idf_component_register(
    SRCS "src/statusLed.cpp"
    REQUIRES led_indicator
    PRIV_REQUIRES driver led_strip
    INCLUDE_DIRS "include"
)
