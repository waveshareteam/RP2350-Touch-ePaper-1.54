# Search for all source files in the current directory
# and save the names to the DIR_GUI_SRCS variable
aux_source_directory(. DIR_GUI_SRCS)

# Generate link library
add_library(GUI ${DIR_GUI_SRCS})

target_include_directories(GUI PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(GUI PUBLIC 
    Config
    )