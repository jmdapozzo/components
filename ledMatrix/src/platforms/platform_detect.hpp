#pragma once

#if defined (ESP_PLATFORM)

 #include <sdkconfig.h>

 #if defined (CONFIG_IDF_TARGET_ESP32S2)

  //#pragma message "Compiling for ESP32-S2"
  #include "esp32/esp32_i2s_parallel_dma.hpp"  
  

 #elif defined (CONFIG_IDF_TARGET_ESP32S3)
  
  //#pragma message "Compiling for ESP32-S3"
  #include "esp32s3/gdma_lcd_parallel16.hpp"
  
  #if defined(SPIRAM_FRAMEBUFFER)
	#pragma message "Use SPIRAM_DMA_BUFFER instead."
  #endif

  #if defined(SPIRAM_DMA_BUFFER) && defined (CONFIG_IDF_TARGET_ESP32S3)       
   #pragma message "Enabling use of PSRAM/SPIRAM based DMA Buffer"
   
   // Disable fast functions because I don't understand the interaction with DMA PSRAM and the CPU->DMA->SPIRAM Cache implications..
   #define NO_FAST_FUNCTIONS 1

  #endif

 #elif defined(CONFIG_IDF_TARGET_ESP32P4)

   #pragma message "You are ahead of your time. ESP32P4 support is planned"

 #elif defined (CONFIG_IDF_TARGET_ESP32) || defined(ESP32)

  // Assume an ESP32 (the original 2015 version)
  // Same include as ESP32S3  
  //#pragma message "Compiling for original ESP32 (released 2016)"  
  
  #define ESP32_THE_ORIG 1	
  #include "esp32/esp32_i2s_parallel_dma.hpp"  

 #elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2)

	#error "ESP32 C2 C3 C6 and H2 devices are not supported by this library."

 
 #else
    #error "Unknown platform."
 #endif


#endif

