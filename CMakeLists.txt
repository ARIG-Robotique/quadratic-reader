#=============================================================================#
# Author: Gregory DEPUILLE                                                    #
# Date:   13/05/2016                                                          #
# Description: Arduino CMake Quadratic Reader                                 #
#=============================================================================#

# Génération du programme par défaut
generate_arduino_firmware(QuadraticNano
    SRCS src/main.cpp
    HDRS src/define.h
    BOARD nano328
    PORT /dev/ttyUSB0
)
