# ESP/IDF LED Matrix

Led Matrix driver used for MacDap's projects.

See https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA?tab=readme-ov-file
See also https://news.sparkfun.com/2650

https://github.com/2dom/PxMatrix


Set PIXEL_COLOR_DEPTH_BITS_DEFAULT to 6 instead of 8 to free some memory
Defined SPIRAM_FRAMEBUFFER for esp32-s3 but not much difference...

Get component from git
git clone https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA.git components/ESP32-HUB75-MatrixPanel-I2S-DMA

Clock speed at the fastest, may be set at 8M for less memory usage
clk_speed::HZ_20M



See https://blog.davidv.dev/posts/exploring-hub75/ for a HUB75 explanation