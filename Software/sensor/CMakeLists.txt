cmake_minimum_required(VERSION 3.16)

project(CitradSU
    VERSION 1.0
    DESCRIPTION "Citrad Sensor Unit"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_custom_target(aux
    SOURCES
        AudioSystem.cpp
        AudioSystem.h
        Config.h
        FileWriter.cpp
        FileWriter.hpp
        functions.cpp
        functions.h
        Makefile
        noise_floor.cpp
        noise_floor.h
        sensor.ino
        SerialIO.hpp
        SerialIO.cpp
)
