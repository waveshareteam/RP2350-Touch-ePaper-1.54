# Search for all source files in the current directory
# and save the names to the DIR_audio_pio_SRCS variable
aux_source_directory(. DIR_audio_pio_SRCS)

# Generate link library
add_library(audio_pio ${DIR_audio_pio_SRCS} )

pico_generate_pio_header(audio_pio ${CMAKE_CURRENT_LIST_DIR}/audio_pio.pio
                         OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_include_directories(audio_pio PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(audio_pio PUBLIC 
    audio_data 
    pico_stdlib 
    hardware_spi 
    hardware_i2c 
    hardware_pwm 
    hardware_adc 
    hardware_pio 
    hardware_dma
    )
