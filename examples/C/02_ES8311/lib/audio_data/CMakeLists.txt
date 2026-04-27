# Search for all source files in the current directory
# and save the names to the DIR_audio_data_SRCS variable
aux_source_directory(. DIR_audio_data_SRCS)

# Generate link library
add_library(audio_data ${DIR_audio_data_SRCS})
target_link_libraries(audio_data PUBLIC pico_stdlib)