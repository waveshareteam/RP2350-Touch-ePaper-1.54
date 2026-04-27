# Search for all source files in the current directory
# and save the names to the DIR_Fonts_SRCS variable
aux_source_directory(. DIR_Fonts_SRCS)

# Generate link library
add_library(ADCBattery ${DIR_Fonts_SRCS})

target_include_directories(ADCBattery PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(ADCBattery PUBLIC 
    Config
    hardware_adc 
    )
