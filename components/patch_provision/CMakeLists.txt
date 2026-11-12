idf_component_register(
    SRCS "patch_provision.c"
    INCLUDE_DIRS "include"
    REQUIRES patch_state
    PRIV_REQUIRES nvs_flash esp_wifi log
)
