add_library(windows STATIC
    windows.c
)

target_include_directories(windows PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(windows PRIVATE $ENV{ZVB_SDK_PATH}/include)
target_link_libraries(windows PRIVATE common)