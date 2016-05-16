#=============================================================================#
# Author: Gregory DEPUILLE                                                    #
# Date:   13/05/2016                                                          #
#                                                                             #
# Description: Arduino CMake Quadratic Reader                                 #
#                                                                             #
#=============================================================================#
set(CMAKE_TOOLCHAIN_FILE ../arduino-cmake/cmake/ArduinoToolchain.cmake) # Arduino Toolchain

cmake_minimum_required(VERSION 2.8)
#====================================================================#
#  Setup Project                                                     #
#====================================================================#
project(QuadraticReader C CXX)

print_board_list()
print_programmer_list()

set(ARDUINO_DEFAULT_BOARD nano328) # Default Board ID, when not specified
set(ARDUINO_DEFAULT_PORT /dev/ttyUSB0) # Default Port, when not specified

# Ajout des libs Arduino
link_directories(${ARDUINO_SDK_PATH}/libraries)

# Génération du programme par défaut
generate_arduino_firmware(quadNano
    SRCS src/main.cpp
    HDRS src/define.h
)
