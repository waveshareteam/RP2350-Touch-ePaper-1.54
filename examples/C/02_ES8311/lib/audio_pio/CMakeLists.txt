# Search for all source files in the current directory
# and save the names to the DIR_audio_pio_SRCS variable
aux_source_directory(. DIR_audio_pio_SRCS)

include_directories(../audio_data)
include_directories(../config)

# Generate link library
add_library(audio_pio ${DIR_audio_pio_SRCS} )
target_link_libraries(audio_pio PUBLIC audio_data config pico_stdlib hardware_spi hardware_i2c hardware_pwm hardware_adc hardware_pio hardware_dma)
