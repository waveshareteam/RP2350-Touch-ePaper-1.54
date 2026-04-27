# Search for all source files in the current directory
# and save the names to the DIR_LCD_SRCS variable
aux_source_directory(. DIR_LCD_SRCS)

# Generate link library
add_library(LCD ${DIR_LCD_SRCS})

target_include_directories(LCD PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(LCD PUBLIC 
    Config
    )