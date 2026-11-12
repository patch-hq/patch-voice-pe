#include "patch_provision.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "patch_setup.h"
#include "sdkconfig.h"

static const char *TAG = "patch-provision";
#define NVS_NS "patch.cred"

static bool nvs_load_str(nvs_handle_t h, const char *k, char *out, size_t cap) {
    size_t len = cap;
    return nvs_get_str(h, k, out, &len) == ESP_OK;
}

bool patch_provision_load(patch_provision_state_t *out) {
    if (!out) return false;
    memset(out, 0, sizeof *out);
    snprintf(out->update_url, sizeof out->update_url, "%s", CONFIG_PATCH_UPDATE_URL);
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    bool have_server = nvs_load_str(h, "server", out->server_url, sizeof out->server_url);
    bool have_token = nvs_load_str(h, "token", out->token, sizeof out->token);
    char update[sizeof out->update_url];
    if (nvs_load_str(h, "updateUrl", update, sizeof update) && update[0] != '\0') {
        snprintf(out->update_url, sizeof out->update_url, "%s", update);
    }
    nvs_close(h);
    if (!have_server || !have_token) return false;
    if (!psu_parse_server_url(out->server_url, out->server_host, sizeof out->server_host,
                              &out->server_port, &out->tls)) {
        ESP_LOGE(TAG, "stored server address is not valid: %s", out->server_url);
        return false;
    }
    if (!psu_token_device_id(out->token, out->device_id, sizeof out->device_id)) {
        ESP_LOGE(TAG, "stored token is not a Patch device token");
        return false;
    }
    return true;
}

bool patch_provision_set(const char *key, const char *value) {
    const char *nvs_key;
    if (strcmp(key, "server") == 0) {
        char host[128]; int port; bool tls;
        if (!psu_parse_server_url(value, host, sizeof host, &port, &tls)) return false;
        nvs_key = "server";
    } else if (strcmp(key, "token") == 0) {
        char id[64];
        if (!psu_token_device_id(value, id, sizeof id)) return false;
        nvs_key = "token";
    } else if (strcmp(key, "update-url") == 0) {
        if (strncmp(value, "https://", 8) != 0 && strncmp(value, "http://", 7) != 0) return false;
        nvs_key = "updateUrl";
    } else {
        return false;
    }
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return false;
    esp_err_t e = nvs_set_str(h, nvs_key, value);
    if (e == ESP_OK) e = nvs_commit(h);
    nvs_close(h);
    return e == ESP_OK;
}

bool patch_provision_set_wifi(const char *ssid, const char *password) {
    wifi_config_t c = {0};
    size_t sl = strlen(ssid), pl = strlen(password);
    if (sl == 0 || sl > sizeof c.sta.ssid || pl > sizeof c.sta.password - 1) return false;
    memcpy(c.sta.ssid, ssid, sl);
    memcpy(c.sta.password, password, pl);
    c.sta.threshold.authmode = pl ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    c.sta.pmf_cfg.capable = true;
    if (esp_wifi_set_storage(WIFI_STORAGE_FLASH) != ESP_OK) return false;
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) return false;
    return esp_wifi_set_config(WIFI_IF_STA, &c) == ESP_OK;
}

bool patch_provision_wifi_configured(void) {
    wifi_config_t c = {0};
    if (esp_wifi_get_config(WIFI_IF_STA, &c) != ESP_OK) return false;
    return c.sta.ssid[0] != '\0';
}

void patch_provision_factory_reset(void) {
    ESP_LOGW(TAG, "forgetting the server and token");
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_erase_all(h);
    nvs_commit(h);
    nvs_close(h);
}
