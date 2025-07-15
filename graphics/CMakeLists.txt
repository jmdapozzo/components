idf_component_register(
    SRCS
        "src/font/nothingFont5x7.c"
        "src/logo/colorLogo32x32.c"
        "src/logo/colorLogo64x64.c"
        "src/logo/colorLogo128x128.c"
        "src/logo/logo16x16.c"
        "src/logo/logo32x32.c"
        "src/logo/logo48x48.c"
        "src/logo/logo64x64.c"
        "src/logo/logoText32x32.c"
        "src/logo/logoText48x48.c"
        "src/logo/logoText64x64.c"
    PRIV_REQUIRES driver
    INCLUDE_DIRS "include"
)
