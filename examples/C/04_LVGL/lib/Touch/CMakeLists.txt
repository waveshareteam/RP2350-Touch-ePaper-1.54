# Search for all source files in the current directory
# and save the names to the DIR_Touch_SRCS variable
aux_source_directory(. DIR_Touch_SRCS)

# Generate link library
add_library(Touch ${DIR_Touch_SRCS})

target_include_directories(Touch PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(Touch PUBLIC 
    Config
    )