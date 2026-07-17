# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize pico_sdk from installed location
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

project(zdi C CXX ASM)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(zdi)

# by default the header is generated into the build dir
pico_generate_pio_header(zdi ${CMAKE_CURRENT_LIST_DIR}/zdi.pio)
# however, alternatively you can choose to generate it somewhere else (in this case in the source tree for check in)
#pico_generate_pio_header(zdi ${CMAKE_CURRENT_LIST_DIR}/zdi.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_sources(zdi PRIVATE main.cpp usb_descriptors.c circular_buffer.c src/cmd.cpp src/cmd_set.cpp src/cmd_status.cpp src/cmd_read.cpp
  src/cmd_write.cpp src/cmd_break.cpp src/cmd_reg.cpp src/cmd_runstop.cpp src/zdis.c src/cmd_disassemble.cpp src/cmd_version.cpp)

add_subdirectory(src)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(zdi PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(zdi PRIVATE pico_stdlib hardware_pio hardware_dma tinyusb_device tinyusb_board)
pico_add_extra_outputs(zdi)

#if (TARGET tinyusb_device)
  # enable USB output, disable UART output
  pico_enable_stdio_usb(zdi 0)
  pico_enable_stdio_uart(zdi 1)
#endif()

set(PROJECT_VERSION  0.1.0)
target_compile_definitions(zdi PRIVATE
  PICO_PROGRAM_VERSION="${PROJECT_VERSION}"
)

# add url via pico_set_program_url