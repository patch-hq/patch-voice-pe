#include "patch_ota.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "patch_tls.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "mdns.h"
#include "patch_setup.h"

static const char *TAG = "patch-ota";

#define CHECK_INTERVAL_US ((uint64_t)6 * 60 * 60 * 1000 * 1000)  // every 6 hours
#define FIRST_CHECK_US ((uint64_t)2 * 60 * 1000 * 1000)          // 2 min after boot

static const char *s_version;
static const char *s_update_url;
static const char *s_token;
static esp_timer_handle_t s_timer;
static volatile bool s_busy;

static bool extract_str(const char *body, const char *key, char *out, size_t cap) {
    char needle[48];
    snprintf(needle, sizeof needle, "\"%s\"", key);
    const char *p = strstr(body, needle);
    if (!p) return false;
    p = strchr(p + strlen(needle), '"');
    if (!p) return false;
    const char *q = strchr(p + 1, '"');
    if (!q || (size_t)(q - p - 1) >= cap) return false;
    memcpy(out, p + 1, (size_t)(q - p - 1));
    out[q - p - 1] = '\0';
    return true;
}

static esp_http_client_config_t http_cfg(const char *url) {
    esp_http_client_config_t c = {
        .url = url,
        .timeout_ms = 30000,
        .crt_bundle_attach = patch_tls_attach,
        .max_redirection_count = 5,
        .buffer_size = 2048,
        .buffer_size_tx = 1024,
    };
    return c;
}

typedef struct { char *buf; size_t cap; size_t len; } body_t;

static esp_err_t on_http(esp_http_client_event_t *e) {
    // Collect the body as it arrives: perform() follows redirects (GitHub
    // release downloads) and has consumed the response by the time it returns.
    body_t *b = (body_t *)e->user_data;
    if (e->event_id == HTTP_EVENT_ON_DATA && b && esp_http_client_get_status_code(e->client) == 200) {
        size_t n = (size_t)e->data_len;
        if (b->len + n >= b->cap) n = b->cap - 1 - b->len;
        memcpy(b->buf + b->len, e->data, n);
        b->len += n;
        b->buf[b->len] = '\0';
    }
    return ESP_OK;
}

static bool fetch(const char *url, char *out, size_t cap) {
    body_t body = { .buf = out, .cap = cap, .len = 0 };
    out[0] = '\0';
    esp_http_client_config_t cfg = http_cfg(url);
    cfg.event_handler = on_http;
    cfg.user_data = &body;
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (!c) return false;
    esp_err_t err = esp_http_client_perform(c);
    bool ok = err == ESP_OK && esp_http_client_get_status_code(c) == 200 && body.len > 0;
    esp_http_client_cleanup(c);
    return ok;
}

static void run_check(void) {
    if (!s_update_url || s_update_url[0] == '\0') return;
    static char manifest[1024];
    if (!fetch(s_update_url, manifest, sizeof manifest)) {
        ESP_LOGW(TAG, "update check: could not read %s", s_update_url);
        return;
    }
    char latest[40], url[300];
    if (!extract_str(manifest, "version", latest, sizeof latest) ||
        !extract_str(manifest, "url", url, sizeof url)) {
        ESP_LOGW(TAG, "update check: manifest has no version/url");
        return;
    }
    if (psu_version_cmp(latest, s_version) <= 0) {
        ESP_LOGI(TAG, "up to date (%s, latest %s)", s_version, latest);
        return;
    }
    ESP_LOGI(TAG, "updating %s -> %s from %s", s_version, latest, url);
    esp_http_client_config_t hcfg = http_cfg(url);
    esp_https_ota_config_t ocfg = { .http_config = &hcfg };
    esp_err_t err = esp_https_ota(&ocfg);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "update installed; restarting");
        esp_restart();
    }
    ESP_LOGE(TAG, "update failed: %s", esp_err_to_name(err));
}

static void check_task(void *arg) {
    (void)arg;
    run_check();
    s_busy = false;
    esp_timer_start_once(s_timer, CHECK_INTERVAL_US);
    vTaskDelete(NULL);
}

static void timer_cb(void *arg) {
    (void)arg;
    if (s_busy) return;
    s_busy = true;
    if (xTaskCreate(check_task, "patch-ota", 8192, NULL, 3, NULL) != pdPASS) {
        s_busy = false;
        esp_timer_start_once(s_timer, CHECK_INTERVAL_US);
    }
}

void patch_ota_check_now(void) {
    if (!s_timer) return;
    esp_timer_stop(s_timer);
    timer_cb(NULL);
}

// ---------- push endpoint -----------------------------------------------------

static bool authorised(httpd_req_t *req) {
    char hdr[1100];
    if (httpd_req_get_hdr_value_str(req, "Authorization", hdr, sizeof hdr) != ESP_OK) return false;
    return strncmp(hdr, "Bearer ", 7) == 0 && s_token && strcmp(hdr + 7, s_token) == 0;
}

static esp_err_t info_get(httpd_req_t *req) {
    char body[160];
    snprintf(body, sizeof body, "{\"firmware\":\"patch-voice-pe\",\"version\":\"%s\"}", s_version);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, body);
}

static esp_err_t update_post(httpd_req_t *req) {
    if (!authorised(req)) {
        httpd_resp_set_status(req, "401 Unauthorized");
        return httpd_resp_sendstr(req, "send the device token as Authorization: Bearer <token>\n");
    }
    if (s_busy) {
        httpd_resp_set_status(req, "409 Conflict");
        return httpd_resp_sendstr(req, "an update is already running\n");
    }
    s_busy = true;
    const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
    esp_ota_handle_t h = 0;
    esp_err_t err = part ? esp_ota_begin(part, OTA_WITH_SEQUENTIAL_WRITES, &h) : ESP_FAIL;
    static char buf[4096];
    int left = req->content_len;
    while (err == ESP_OK && left > 0) {
        int n = httpd_req_recv(req, buf, left < (int)sizeof buf ? left : (int)sizeof buf);
        if (n == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (n <= 0) { err = ESP_FAIL; break; }
        err = esp_ota_write(h, buf, (size_t)n);
        left -= n;
    }
    if (err == ESP_OK) err = esp_ota_end(h);
    else if (h) esp_ota_abort(h);
    if (err == ESP_OK) err = esp_ota_set_boot_partition(part);
    if (err != ESP_OK) {
        s_busy = false;
        ESP_LOGE(TAG, "pushed update failed: %s", esp_err_to_name(err));
        httpd_resp_set_status(req, "400 Bad Request");
        char msg[80];
        snprintf(msg, sizeof msg, "update failed: %s\n", esp_err_to_name(err));
        return httpd_resp_sendstr(req, msg);
    }
    httpd_resp_sendstr(req, "installed; restarting\n");
    ESP_LOGI(TAG, "pushed update installed; restarting");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;
}

static void start_server(void) {
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size = 8192;
    cfg.recv_wait_timeout = 30;
    httpd_handle_t srv = NULL;
    if (httpd_start(&srv, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "push endpoint failed to start");
        return;
    }
    httpd_uri_t info = { .uri = "/info", .method = HTTP_GET, .handler = info_get };
    httpd_uri_t upd = { .uri = "/update", .method = HTTP_POST, .handler = update_post };
    httpd_register_uri_handler(srv, &info);
    httpd_register_uri_handler(srv, &upd);
}

void patch_ota_start(const char *running_version, const char *update_url, const char *token,
                     const char *hostname) {
    s_version = running_version;
    s_update_url = update_url;
    s_token = token;
    // A new image that booted this far is good: keep it rather than rolling back.
    esp_ota_mark_app_valid_cancel_rollback();
    if (mdns_init() == ESP_OK) {
        mdns_hostname_set(hostname);
        mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
        ESP_LOGI(TAG, "reachable as http://%s.local", hostname);
    }
    start_server();
    const esp_timer_create_args_t a = { .callback = timer_cb, .name = "patch-ota" };
    esp_timer_create(&a, &s_timer);
    esp_timer_start_once(s_timer, FIRST_CHECK_US);
}
