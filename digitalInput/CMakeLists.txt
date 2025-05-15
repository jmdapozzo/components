idf_component_register(
    SRCS "src/digitalInput.cpp"
    PRIV_REQUIRES esp_driver_gpio esp_event macdap_eventLoop
    INCLUDE_DIRS "include"
)
