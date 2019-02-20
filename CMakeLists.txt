#=============================================================================#
# Author: Gregory DEPUILLE                                                    #
# Date:   13/05/2016                                                          #
# Description: Arduino CMake Quadratic Reader                                 #
#=============================================================================#
# Contrôle de la version minimale de CMake
cmake_minimum_required(VERSION 3.8.2)

# Définition de la tool chain de build
set(CMAKE_TOOLCHAIN_FILE ../arduino-cmake-ng/cmake/Arduino-Toolchain.cmake)

# Compilateur flags
#set(ARDUINO_C_FLAGS      "-ffunction-sections -fdata-sections")
#set(ARDUINO_CXX_FLAGS    "${ARDUINO_C_FLAGS} -fno-exceptions")
#set(ARDUINO_LINKER_FLAGS "-Wl,--gc-sections")

set(ARDUINO_CXX_FLAGS    "-std=c++11")

#add_definitions(-DDEBUG_MODE)

#====================================================================#
#  Setup Project                                                     #
#====================================================================#

project(QuadradicReader C CXX ASM)

# Récupération de la board
get_board_id(board_id nano atmega328)

# Build
add_arduino_executable(QuadradicReader ${board_id} src/main.cpp src/define.h)
link_platform_library(QuadradicReader Wire ${board_id})
#upload_arduino_target(QuadradicReader "${board_id}" /dev/ttyUSB0)
