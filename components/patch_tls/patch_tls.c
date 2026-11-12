#include "patch_tls.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"

extern const uint8_t s_bundle_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t s_bundle_end[] asm("_binary_x509_crt_bundle_end");

// The bundle's own check (not in its public header, but exported).
int esp_crt_verify_callback(void *buf, mbedtls_x509_crt *crt, int depth, uint32_t *flags);

// Bundle layout (gen_crt_bundle.py): u16 count, then per certificate u16 name
// length, u16 key length, subject DER, SubjectPublicKeyInfo DER.
static bool is_bundled_root(const mbedtls_x509_crt *crt) {
    const uint8_t *p = s_bundle_start + 2;
    const uint8_t *end = s_bundle_end;
    uint16_t count = (uint16_t)(s_bundle_start[0] << 8 | s_bundle_start[1]);
    for (uint16_t i = 0; i < count && p + 4 <= end; i++) {
        size_t name_len = (size_t)(p[0] << 8 | p[1]);
        size_t key_len = (size_t)(p[2] << 8 | p[3]);
        const uint8_t *name = p + 4;
        const uint8_t *key = name + name_len;
        if (key + key_len > end) return false;
        if (name_len == crt->subject_raw.len && key_len == crt->pk_raw.len &&
            memcmp(name, crt->subject_raw.p, name_len) == 0 &&
            memcmp(key, crt->pk_raw.p, key_len) == 0) {
            return true;
        }
        p = key + key_len;
    }
    return false;
}

static int verify(void *ctx, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
    if (*flags != 0 && is_bundled_root(crt)) {
        // A trust anchor: its key is trusted, whoever signed this copy of it.
        *flags = 0;
        return 0;
    }
    return esp_crt_verify_callback(ctx, crt, depth, flags);
}

esp_err_t patch_tls_attach(void *conf) {
    esp_err_t e = esp_crt_bundle_attach(conf);
    if (e == ESP_OK && conf) mbedtls_ssl_conf_verify((mbedtls_ssl_config *)conf, verify, NULL);
    return e;
}
