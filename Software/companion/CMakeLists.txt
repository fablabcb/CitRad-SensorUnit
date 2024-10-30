cmake_minimum_required(VERSION 3.16)

project(CitradCompanion
    VERSION 1.0
    DESCRIPTION "Citrad Companion"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(_NAME main_analyzer)

set(SOURCES
    main.cpp
    InputStream.hpp
    InputStream.cpp
    gfx/SpectrumColorizer.hpp
    gfx/SpectrumColorizer.cpp
    gfx/SpectrumVisualizer.hpp
    gfx/SpectrumVisualizer.cpp
    ../sensor/noise_floor.h
    ../sensor/noise_floor.cpp
    ../sensor/SignalAnalyzer.hpp
    ../sensor/SignalAnalyzer.cpp
    ../sensor/statistics.hpp
    ../sensor/statistics.cpp
)

add_executable(${_NAME}
    ${SOURCES}
    )

target_link_libraries(${_NAME}
    cairo
    SDL2)
