cmake_minimum_required(VERSION 3.16)

project(CitradSU
    VERSION 1.0
    DESCRIPTION "Citrad Sensor Unit"
    LANGUAGES CXX)

add_definitions(-DTEENSYDUINO=120 -D__AVR__)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR avr)

set(TEENSY_ROOT ~/.arduino15/packages/teensy/tools/teensy-compile/11.3.1/avr)
set(CMAKE_SYSROOT ${TEENSY_ROOT})

set(CMAKE_C_COMPILER ${TEENSY_ROOT}/bin/avr-gcc)
set(CMAKE_CXX_COMPILER ${TEENSY_ROOT}/bin/avr-g++)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    SYSTEM
    ~/.arduino15/packages/teensy/hardware/avr/1.59.0/libraries/Audio
    ~/.arduino15/packages/teensy/hardware/avr/1.59.0/cores/teensy4
)

add_executable(sensor
    AudioSystem.cpp
    AudioSystem.h
    Config.hpp
    Config.cpp
    FileIO.cpp
    FileIO.hpp
    functions.cpp
    functions.h
    Makefile
    noise_floor.cpp
    noise_floor.h
    statistics.hpp
    statistics.cpp
    sensor.ino
    SerialIO.hpp
    SerialIO.cpp
    SignalAnalyzer.hpp
    SignalAnalyzer.cpp
)
