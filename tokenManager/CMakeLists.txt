idf_component_register(
    SRCS "src/tokenManager.cpp"
    PRIV_REQUIRES esp_http_client esp_netif mbedtls json
    INCLUDE_DIRS "include"
)
