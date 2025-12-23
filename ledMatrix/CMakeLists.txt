idf_component_register(
  SRCS
    "src/ledMatrix.cpp"
    "src/ESP32-HUB75-MatrixPanel-I2S-DMA.cpp"
    "src/ESP32-HUB75-MatrixPanel-leddrivers.cpp"
    "src/gdma_lcd_parallel16.cpp"
  PRIV_REQUIRES
    driver esp_lcd
  INCLUDE_DIRS
    "include"
)

target_compile_options(${COMPONENT_TARGET} PUBLIC -DSPIRAM_DMA_BUFFER)
