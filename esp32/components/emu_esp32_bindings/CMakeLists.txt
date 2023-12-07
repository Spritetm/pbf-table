#Note: this is for the ESP32 build.

idf_build_get_property(target IDF_TARGET)

if (${target} STREQUAL esp32)

idf_component_register(SRCS gfx_esp32.c lcd.c mmap_file_esp32.c audio_esp32.c hiscore.c haptics.c prefs.c
        REQUIRES emu log spi_flash esp_partition driver nvs_flash esp_timer
        INCLUDE_DIRS "." )

target_compile_options(${COMPONENT_LIB} PRIVATE -DPROJECT_NAME="${CMAKE_PROJECT_NAME}")
endif()
