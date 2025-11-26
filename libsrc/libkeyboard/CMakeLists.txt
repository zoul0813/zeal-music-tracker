add_library(keyboard STATIC
    keyboard.c
)

target_include_directories(keyboard PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(keyboard PRIVATE $ENV{ZVB_SDK_PATH}/include)
