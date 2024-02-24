# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/pipe/src/pico/pico-sdk")

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(dma-copy-buffer-alignment C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_compile_options(-Werror -Wall -Wextra -Wnull-dereference)
add_executable(dma-copy-buffer-alignment dma-copy-buffer-alignment.c )

pico_set_program_name(dma-copy-buffer-alignment "dma-copy-buffer-alignment")
pico_set_program_version(dma-copy-buffer-alignment "0.1")

pico_enable_stdio_uart(dma-copy-buffer-alignment 1)
pico_enable_stdio_usb(dma-copy-buffer-alignment 0)

# Add the standard library to the build
target_link_libraries(dma-copy-buffer-alignment
        pico_stdlib hardware_dma)

# Add the standard include files to the build
target_include_directories(dma-copy-buffer-alignment PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(dma-copy-buffer-alignment)

