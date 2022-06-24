#Note: this is for the ESP32 build.

idf_build_get_property(target IDF_TARGET)

if (${target} STREQUAL esp32s3)

idf_component_register(SRCS gfx_esp32.c lcd.c mmap_file_esp32.c audio_esp32s3.c haptic.c lcdbackboard.c hiscore.c io.c
        REQUIRES emu log spi_flash driver esp_lcd nvs_flash esp_timer
        EMBED_FILES img/loading.rgb img/pf.rgb img/table1.rgb img/table2.rgb img/table3.rgb img/table4.rgb
        INCLUDE_DIRS "." )

target_compile_options(${COMPONENT_LIB} PRIVATE -DPROJECT_NAME="${CMAKE_PROJECT_NAME}")

endif()