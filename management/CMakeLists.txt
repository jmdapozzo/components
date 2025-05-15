idf_component_register(
    SRCS "src/management.cpp"
    PRIV_REQUIRES esp_http_client esp_netif json esp_app_format esp_https_ota esp_partition mbedtls macdap_tokenManager
    INCLUDE_DIRS "include"
)
