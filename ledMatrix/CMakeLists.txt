idf_component_register(
  SRCS
    "src/ledMatrix.cpp"
  PRIV_REQUIRES
    driver esp_lcd
  INCLUDE_DIRS
    "include"
)

# target_compile_options(${COMPONENT_TARGET} PUBLIC -DSPIRAM_DMA_BUFFER)
