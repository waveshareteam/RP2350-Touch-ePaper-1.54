# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

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

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(main C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(main 
    main.c
    ./lv_port/lv_port_disp.c
    ./lv_port/lv_port_indev.c
    lv_app_hwtest.c 
    )

pico_set_program_name(main "main")
pico_set_program_version(main "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(main 0)
pico_enable_stdio_usb(main 1)

target_compile_definitions(main PRIVATE 
    PICO_DEFAULT_UART=1
	PICO_DEFAULT_UART_TX_PIN=26
	PICO_DEFAULT_UART_RX_PIN=27
    PICO_DEFAULT_UART_BAUD_RATE=115200
)

# Add compilation subdirectory
add_subdirectory(./lib/Config)
add_subdirectory(./lib/LCD)
add_subdirectory(./lib/GUI)
add_subdirectory(./lib/Fonts)
add_subdirectory(./lib/lvgl)
add_subdirectory(./lib/battery/ADCBattery)
add_subdirectory(./lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src)
add_subdirectory(./lib/SDCard)
add_subdirectory(./lib/SHTC3)
add_subdirectory(./lib/PCF85063)
add_subdirectory(./lib/audio/audio_data)
add_subdirectory(./lib/audio/audio_pio)
add_subdirectory(./lib/audio/es8311)
add_subdirectory(./lib/Touch)

# Add header file directory
include_directories(./lib/Config)
include_directories(./lib/LCD)
include_directories(./lib/GUI)
include_directories(./lib/Fonts)
include_directories(./lib/battery/ADCBattery)
include_directories(./lib/SDCard)
include_directories(./lib/SHTC3)
include_directories(./lib/PCF85063)
include_directories(./lib/audio/audio_data)
include_directories(./lib/audio/audio_pio)
include_directories(./lib/audio/es8311)
include_directories(./lib/Touch)

# Add the standard library to the build
target_link_libraries(main
        pico_stdlib
        hardware_flash
        hardware_adc
        Config
        LCD
        GUI
        Fonts
        lvgl
        ADCBattery
        SDCard
        audio_pio
        es8311         
        PCF85063 
        SHTC3 
        Touch
    )

# Add the standard include files to the build
target_include_directories(main PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lv_port
)

pico_add_extra_outputs(main)

