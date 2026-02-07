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
cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards )
set(PICO_PLATFORM "rp2350" CACHE STRING "Chip type")
set(PICO_BOARD "tufty2350" CACHE STRING "Board type")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(nes C CXX ASM)

# Reduce stack to fit in 4KB SCRATCH_Y alongside other scratch data
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPICO_STACK_SIZE=0x600")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPICO_STACK_SIZE=0x600")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -DPICO_STACK_SIZE=0x600")

set(PICO_PROGRAM_VERSION_STRING "305")

option(I2S "Enable I2S sound" OFF)
option(I2S_CS4334 "Enable I2S CS4334 sound" OFF)
option(VGA "Enable VGA" OFF)
option(TFT "Enable TFT display" OFF)
option(TFT_PARALLEL "Enable TFT parallel mode" OFF)
option(ILI9341 "Enable TFT ILI9341 display" OFF)
option(INVERSION "Enable TFT display INVERSION" OFF)
option(HDMI "Enable HDMI display" OFF)
option(TV "Enable TV composite output" OFF)
option(SOFTTV "Enable TV soft composite output" OFF)

# Tufty 2350 config: TFT parallel, no audio, embedded ROM
set(TFT ON)
set(TFT_PARALLEL ON)
set(INVERSION ON)

SET(PROJECT_NAME nes)
set(FLASH_SIZE 16384)
SET(BUILD_NAME "tufty-${PROJECT_NAME}")

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(infones)
add_subdirectory(drivers/audio)
add_subdirectory(drivers/ps2kbd)
add_subdirectory(drivers/nespad)
add_subdirectory(drivers/fatfs)
add_subdirectory(drivers/sdcard)
add_subdirectory(drivers/vga-nextgen)
add_subdirectory(drivers/st7789)
add_subdirectory(drivers/hdmi)
add_subdirectory(drivers/tv)
add_subdirectory(drivers/tv-software)
add_subdirectory(drivers/graphics)

# INCLUDE FILES THAT SHOULD BE COMPILED:
file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")

message(STATUS "Add source files:")
foreach (SRC_FILE IN LISTS SRC)
    message(STATUS "${SRC_FILE}")
endforeach ()
message(STATUS "")

add_executable(${PROJECT_NAME} ${SRC})

pico_set_program_name(nes "NES emulator for Pimoroni Tufty 2350")

# pull in common dependencies
target_link_libraries(nes PRIVATE
    infones
    audio
    graphics

    ps2kbd
    nespad
    sdcard
    fatfs

    pico_runtime
    pico_stdlib
    pico_multicore
    hardware_dma
    hardware_pio
    hardware_i2c
    hardware_interp
    hardware_timer
    hardware_clocks
    hardware_pwm
    hardware_flash
    stdc++

    tinyusb_host tinyusb_board
)

target_include_directories(${PROJECT_NAME} PRIVATE
    infones
    src
)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(nes pico_cyw43_arch_none)
endif()

pico_enable_stdio_uart(${PROJECT_NAME} 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

# add url via pico_set_program_url
target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage)
target_compile_definitions(${PROJECT_NAME} PRIVATE FLASH_SIZE=${FLASH_SIZE})

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
target_compile_options(${PROJECT_NAME} PUBLIC
        -Wno-error=suggest-attribute=format
        -Wno-error=cast-qual
        -Wno-error=unused-parameter
        -Wno-error=conversion
        -Wno-error=format=
        -Wno-error=sign-compare
        -Wno-error=missing-field-initializers
        -Wno-error=switch
        -Wno-error=implicit-fallthrough=
        -Wno-error=stringop-truncation
        -Wno-error=restrict
        -w
)
endif ()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        # Tufty 2350 specific
        TUFTY2350

        PICO_PROGRAM_VERSION_STRING="${PICO_PROGRAM_VERSION_STRING}"
)

# TFT parallel display
target_link_libraries(${PROJECT_NAME} PRIVATE st7789)
target_compile_definitions(${PROJECT_NAME} PRIVATE TFT)
target_compile_definitions(${PROJECT_NAME} PRIVATE TFT_PARALLEL)
target_compile_definitions(${PROJECT_NAME} PRIVATE INVERSION)
SET(BUILD_NAME "${BUILD_NAME}-TFT-PARALLEL")

# No real audio - PWM on dummy pin
target_compile_definitions(${PROJECT_NAME} PRIVATE AUDIO_PWM)
SET(BUILD_NAME "${BUILD_NAME}-PWM")

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}-${PICO_PROGRAM_VERSION_STRING}")
