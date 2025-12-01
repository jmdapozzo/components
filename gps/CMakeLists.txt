file(GLOB_RECURSE ICON_SOURCES "src/icons/*.c")

idf_component_register(
    SRCS
        "src/GPS.cpp"
        ${ICON_SOURCES}
    PRIV_REQUIRES 
        esp_event
        esp_driver_uart
    INCLUDE_DIRS
        "include"
)
