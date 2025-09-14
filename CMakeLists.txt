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
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")
set(PICOCALC_FROTZ_VERSION "1.0" CACHE STRING "PicoCalc Frotz version")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(picocalc-frotz C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(picocalc-frotz
        picocalc/init.c
        picocalc/input.c
        picocalc/output.c
        picocalc/pic.c
        modules/frotz/src/blorb/blorb.h
        modules/frotz/src/blorb/blorblib.c
        modules/frotz/src/blorb/blorblow.h
        modules/frotz/src/common/frotz.h
        modules/frotz/src/common/setup.h
        modules/frotz/src/common/unused.h
        modules/frotz/src/common/buffer.c
        modules/frotz/src/common/err.c
        modules/frotz/src/common/fastmem.c
        modules/frotz/src/common/files.c
        modules/frotz/src/common/getopt.c
        modules/frotz/src/common/hotkey.c
        modules/frotz/src/common/input.c
        modules/frotz/src/common/main.c
        modules/frotz/src/common/math.c
        modules/frotz/src/common/missing.c
        modules/frotz/src/common/object.c
        modules/frotz/src/common/process.c
        modules/frotz/src/common/quetzal.c
        modules/frotz/src/common/random.c
        modules/frotz/src/common/redirect.c
        modules/frotz/src/common/screen.c
        modules/frotz/src/common/sound.c
        modules/frotz/src/common/stream.c
        modules/frotz/src/common/table.c
        modules/frotz/src/common/text.c
        modules/frotz/src/common/variable.c
        modules/inih/ini.c
        modules/picocalc-text-starter/drivers/audio.c
        modules/picocalc-text-starter/drivers/audio.h
        modules/picocalc-text-starter/drivers/clib.c
        modules/picocalc-text-starter/drivers/display.c
        modules/picocalc-text-starter/drivers/display.h
        modules/picocalc-text-starter/drivers/fat32.c
        modules/picocalc-text-starter/drivers/fat32.h
        modules/picocalc-text-starter/drivers/font-5x10.c
        modules/picocalc-text-starter/drivers/font-8x10.c
        modules/picocalc-text-starter/drivers/font.h
        modules/picocalc-text-starter/drivers/keyboard.c
        modules/picocalc-text-starter/drivers/keyboard.h
        modules/picocalc-text-starter/drivers/lcd.c
        modules/picocalc-text-starter/drivers/lcd.h
        modules/picocalc-text-starter/drivers/sdcard.c
        modules/picocalc-text-starter/drivers/sdcard.h
        modules/picocalc-text-starter/drivers/southbridge.c
        modules/picocalc-text-starter/drivers/southbridge.h
)

pico_set_program_name(picocalc-frotz "picocalc-frotz")
pico_set_program_version(picocalc-frotz "${PICOCALC_FROTZ_VERSION}")
pico_set_program_url(picocalc-frotz "https://github.com/BlairLeduc/picocalc-frotz")
pico_set_program_description(picocalc-frotz "An unofficial port of Frotz to the PicoCalc")

# Generate PIO header
pico_generate_pio_header(picocalc-frotz ${CMAKE_CURRENT_LIST_DIR}/modules/picocalc-text-starter/drivers/audio.pio)

# Enable the use of the ROSC for random number generation
target_compile_definitions(picocalc-frotz PRIVATE
    PICO_STACK_SIZE=2048 # 8 KB stack size
    )

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(picocalc-frotz 0)
pico_enable_stdio_usb(picocalc-frotz 0)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    @ONLY
)

# Add the standard library to the build
target_link_libraries(picocalc-frotz
        pico_stdlib
        pico_printf
        pico_float
        pico_rand
        hardware_gpio
        hardware_i2c
        hardware_spi
        hardware_pio
        hardware_clocks)

# Add the standard include files to the build
target_include_directories(picocalc-frotz PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/modules/frotz/src/common
        ${CMAKE_CURRENT_LIST_DIR}/modules/inih
        ${CMAKE_CURRENT_LIST_DIR}/modules/picocalc-text-starter/drivers
)

pico_add_extra_outputs(picocalc-frotz)

