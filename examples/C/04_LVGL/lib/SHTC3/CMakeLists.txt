# Find all source files in a directory
aux_source_directory(. DIR_SHTC3_SRCS)

# Adds a library target 
add_library(SHTC3 ${DIR_SHTC3_SRCS})

target_include_directories(SHTC3 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(SHTC3 PUBLIC 
    Config
    )