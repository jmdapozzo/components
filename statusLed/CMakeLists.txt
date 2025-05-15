idf_component_register(
    SRCS "src/statusLed.cpp"
    PRIV_REQUIRES driver led_strip
    INCLUDE_DIRS "include"
)
