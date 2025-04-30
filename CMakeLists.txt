# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(LOCAL_BUILD "Build without Raspberry Pi Pico SDK" OFF)

if (NOT LOCAL_BUILD)
# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pixelblit C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)

# Create the executable
add_executable(pixelblit)

# Enable USB serial output
pico_enable_stdio_usb(pixelblit 1)
pico_enable_stdio_uart(pixelblit 0)

# Generate PIO header
pico_generate_pio_header(pixelblit ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

# Add source files
target_sources(pixelblit PRIVATE 
    pixelblit.c 
    lib/utils.c 
    lib/libpixelblit.c 
    ir_control.c
)

# Add compile definitions
target_compile_definitions(pixelblit PRIVATE
    PIN_DBG1=3
)

# Link libraries
target_link_libraries(pixelblit PRIVATE 
    pico_stdlib 
    pico_multicore 
    hardware_pio 
    hardware_dma
)# Add extra outputs
pico_add_extra_outputs(pixelblit)

# Additionally generate python and hex pioasm outputs for inclusion in the RP2040 datasheet
add_custom_target(pio_ws2812_datasheet DEPENDS ${CMAKE_CURRENT_LIST_DIR}/generated/ws2812.py)
add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/ws2812.py
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio
    COMMAND pioasm -o python ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio ${CMAKE_CURRENT_LIST_DIR}/generated/ws2812.py
    VERBATIM)
add_dependencies(pixelblit pio_ws2812_datasheet)
else()
    project(test C CXX ASM)
    add_executable(test test.c lib/utils.c)
    add_definitions(-DLOCAL_BUILD=1)
endif()


