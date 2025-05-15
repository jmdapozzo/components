idf_component_register(
    SRCS "src/GPS.cpp"
    PRIV_REQUIRES esp_event esp_driver_uart
    INCLUDE_DIRS "include"
)
