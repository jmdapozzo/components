idf_component_register(
    SRCS
        "src/graphics.cpp"
        "src/font/nothingFont5x7.c"
        "src/logo/bwLogoNoText16x16.c"
        "src/logo/bwLogoNoText32x32.c"
        "src/logo/bwLogoNoText48x48.c"
        "src/logo/bwLogoNoText64x64.c"
        "src/logo/bwLogoText32x32.c"
        "src/logo/bwLogoText48x48.c"
        "src/logo/bwLogoText64x64.c"
        "src/logo/colorLogoNoText16x16.c"
        "src/logo/colorLogoNoText32x32.c"
        "src/logo/colorLogoNoText64x64.c"
        "src/logo/colorLogoNoText128x128.c"
    PRIV_REQUIRES driver
    INCLUDE_DIRS "include"
)
