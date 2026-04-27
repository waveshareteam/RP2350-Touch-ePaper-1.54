# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(audio_firmware C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(audio_firmware)

pico_generate_pio_header(audio_firmware ${CMAKE_CURRENT_LIST_DIR}/lib/audio_pio/audio_pio.pio
                        OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/audio_pio)

target_sources(audio_firmware PRIVATE
    lib/audio_pio/audio_pio.c
    lib/audio_data/audio_data.c
    lib/config/DEV_Config.c
    lib/es8311/es8311.c
    main.c
)

# Add compilation subdirectory
add_subdirectory(lib/audio_data)
add_subdirectory(lib/audio_pio)
add_subdirectory(lib/config)
add_subdirectory(lib/es8311)

# Add header file directory
include_directories(./lib/audio_data)
include_directories(./lib/audio_pio)
include_directories(./lib/config)
include_directories(./lib/es8311)

target_link_libraries(audio_firmware PRIVATE 
    audio_pio 
    audio_data
    config
    es8311
    pico_stdlib 
    hardware_spi 
    hardware_i2c 
    hardware_pwm 
    hardware_adc 
    hardware_pio 
    hardware_dma
    pico_multicore
)

# enable usb output, disable uart output
pico_enable_stdio_usb(audio_firmware 1)
pico_enable_stdio_uart(audio_firmware 1)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(audio_firmware)


