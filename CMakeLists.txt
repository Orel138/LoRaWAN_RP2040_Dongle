# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()

set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)

if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(LoRaWAN_RP2040_Dongle C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(LoRaWAN_RP2040_Dongle
    Src/main.c
    Drivers/st7789/src/st7789.c
)

# Add the standard library to the build
target_link_libraries(LoRaWAN_RP2040_Dongle
    pico_stdlib
    hardware_spi
)

# Include directories
target_include_directories(LoRaWAN_RP2040_Dongle PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/Includes
    ${CMAKE_CURRENT_LIST_DIR}/Drivers/st7789/src/include
)

# Force l'inclusion de pico/time.h pour le driver st7789 (submodule Git non modifié)
target_compile_options(LoRaWAN_RP2040_Dongle PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-include pico/time.h>
)

pico_set_program_name(LoRaWAN_RP2040_Dongle "LoRaWAN_RP2040_Dongle")
pico_set_program_version(LoRaWAN_RP2040_Dongle "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(LoRaWAN_RP2040_Dongle 0)
pico_enable_stdio_usb(LoRaWAN_RP2040_Dongle 1)

pico_add_extra_outputs(LoRaWAN_RP2040_Dongle)
