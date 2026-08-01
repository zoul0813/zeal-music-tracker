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
set(CMAKE_DEBUG_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

zos_use(zvb)
find_package(ZVB_SDK REQUIRED)

zos_use(coreutils)
find_package(COREUTILS REQUIRED)

project(${target_name} C ASM)



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
target_include_directories(${target_name} PRIVATE $ENV{ZVB_SDK_PATH}/include)
target_link_libraries(${target_name} PRIVATE core keyboard windows)

target_compile_options(${target_name} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:--nostdlib>
)

zos_add_outputs(${target_name})
coreutils_add_post_build_artifacts(${target_name}
    DEBUG_DIR ${CMAKE_DEBUG_OUTPUT_DIRECTORY}
)