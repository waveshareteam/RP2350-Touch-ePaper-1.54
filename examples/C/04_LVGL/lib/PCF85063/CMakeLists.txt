# Find all source files in a directory
aux_source_directory(. DIR_PCF85063_SRCS)

# Adds a library target 
add_library(PCF85063 ${DIR_PCF85063_SRCS})

target_include_directories(PCF85063 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(PCF85063 PUBLIC 
    Config
    )