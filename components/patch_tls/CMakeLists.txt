idf_component_register(
    SRCS "patch_tls.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_common
    PRIV_REQUIRES mbedtls
)
