# Search for all source files in the current directory
# and save the names to the DIR_ES8311_SRCS variable
aux_source_directory(. DIR_ES8311_SRCS)

include_directories(../config)
include_directories(../audio_pio)

# Adds a library target 
add_library(es8311 ${DIR_ES8311_SRCS})
target_link_libraries(es8311 PUBLIC config audio_pio pico_stdlib hardware_i2c)