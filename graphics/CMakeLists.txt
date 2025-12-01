file(GLOB FONT_SOURCES "src/fonts/*.c")
file(GLOB_RECURSE ICON_SOURCES "src/icons/*.c")
file(GLOB_RECURSE LOGO_SOURCES "src/logos/*.c")

idf_component_register(
    SRCS
        "src/graphics.cpp"
        "src/screenLayout.cpp"
        ${FONT_SOURCES}
        ${ICON_SOURCES}
        ${LOGO_SOURCES}
    PRIV_REQUIRES driver
    INCLUDE_DIRS "include"
)
