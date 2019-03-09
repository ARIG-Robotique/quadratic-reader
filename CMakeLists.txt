#=============================================================================#
# Author: Gregory DEPUILLE                                                    #
# Date:   13/05/2016                                                          #
# Description: Arduino CMake Quadratic Reader                                 #
#=============================================================================#
# Contrôle de la version minimale de CMake
cmake_minimum_required(VERSION 3.8.2)

# Définition de la tool chain de build
set(CMAKE_TOOLCHAIN_FILE ./cmake/Arduino-Toolchain.cmake)

# Fix bug in avrdude.conf included in ArduinoSDK
set(ARDUINO_CMAKE_AVRDUDE_CONFIG_PATH /etc/avrdude/avrdude.conf)

# Compilateur flags
#set(ARDUINO_C_FLAGS      "-ffunction-sections -fdata-sections")
#set(ARDUINO_CXX_FLAGS    "${ARDUINO_C_FLAGS} -fno-exceptions")
#set(ARDUINO_LINKER_FLAGS "-Wl,--gc-sections")

set(ARDUINO_CXX_FLAGS    "-std=c++11")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message(STATUS "Debug mode ON")
    add_definitions(-DDEBUG_MODE)
endif()

#====================================================================#
#  Setup Project                                                     #
#====================================================================#

project(QuadraticReader C CXX ASM)
arduino_cmake_project(QuadraticReader BOARD_NAME nano BOARD_CPU atmega328)

# Build
add_arduino_executable(QuadraticReader src/main.cpp src/define.h)

# Wire lib
find_arduino_library(wire Wire)
link_arduino_library(QuadraticReader wire)

# Upload
set_target_upload_port(QuadraticReader /dev/ttyUSB0)
