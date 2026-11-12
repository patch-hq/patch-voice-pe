idf_component_register(
    SRCS "patch_ota.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_http_client esp_http_server esp_https_ota app_update patch_tls log freertos
                  esp_timer espressif__mdns patch_state
)
