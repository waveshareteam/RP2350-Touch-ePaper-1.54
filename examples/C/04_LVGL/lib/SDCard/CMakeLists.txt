# Find all source files in a directory
aux_source_directory(. DIR_SDCARD_SRCS)

# Adds a library target 
add_library(SDCard ${DIR_SDCARD_SRCS})

target_include_directories(SDCard PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(SDCard PUBLIC 
    Config 
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    )