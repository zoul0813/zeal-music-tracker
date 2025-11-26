cmake_minimum_required(VERSION 3.16)

# Application Version
set(APP_VERSION_MAJOR 0)
set(APP_VERSION_MINOR 1)
set(APP_VERSION_PATCH 0)
set(APP_NAME "Zeal Music Tracker")
set(target_name "zmt")

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# Ensure dirs exist
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

zos_use(zvb)
find_package(ZVB_SDK REQUIRED)

project(${target_name} C ASM)

# include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories($ENV{ZVB_SDK_PATH}/include)

add_subdirectory(libsrc/libcommon)
add_subdirectory(libsrc/libkeyboard)
add_subdirectory(libsrc/libwindows)

add_executable(${target_name}
    "src/main.c"
    "src/arrange.c"
    "src/pattern.c"
    "src/shared.c"
    "src/tracker.c"

    "src/dialogs/confirm_dialog.c"
    "src/dialogs/file_dialog.c"
    "src/dialogs/help_dialog.c"
)

target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
# target_link_libraries(${target_name} PRIVATE )
target_link_libraries(${target_name} PRIVATE zvb_gfx)
zos_link_libraries(${target_name} PRIVATE common keyboard windows)

target_compile_options(${target_name} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:--nostdlib>
)

zos_add_outputs(${target_name})