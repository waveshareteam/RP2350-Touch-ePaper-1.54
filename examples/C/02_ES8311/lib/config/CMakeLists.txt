# Search for all source files in the current directory
# and save the names to the DIR_Config_SRCS variable
aux_source_directory(. DIR_Config_SRCS)

# Adds a library target 
add_library(config ${DIR_Config_SRCS})
target_link_libraries(config PUBLIC pico_stdlib hardware_spi hardware_i2c hardware_pwm hardware_adc hardware_dma)