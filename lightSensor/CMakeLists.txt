file(GLOB_RECURSE ICON_SOURCES "src/icons/*.c")

idf_component_register(
    SRCS
        "src/lightSensor.cpp"
        ${ICON_SOURCES}
    PRIV_REQUIRES
        esp_event
        esp_timer
        driver
    INCLUDE_DIRS
        "include")
