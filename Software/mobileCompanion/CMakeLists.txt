cmake_minimum_required(VERSION 3.16)

set(_NAME citrad.mobileCompanion)
set(_ANDROID_NAME "citrad.mobileCompanion")

project(${_NAME} VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fsanitize=address -fsanitize=leak -fsanitize=undefined")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -fsanitize=address -fsanitize=leak -fsanitize=undefined")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=leak")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address -fsanitize=leak")
endif()

# android section start
if(ANDROID)
    message("Configuring for Android Build")
    # because otherwise the android-toolchain file will set it to ONLY and break everything
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE)
    set(QT_ANDROID_APPLICATION_BINARY ${_ANDROID_NAME})
    message("Adding qt-android-cmake funtionality")
    include(external/qt-android-cmake/CMakeLists.txt)
endif()
# android section end

find_package(Qt5 REQUIRED COMPONENTS Core Quick Qml SerialPort)

set(SOURCES
    src/main.cpp
    src/AndroidTest.cpp
    src/devices/AndroidSerialDeviceManager.cpp
    src/DevicesController.cpp
    src/devices/InputStream.cpp
    src/LogModel.cpp
    src/gfx/ImageDisplay.cpp
    src/gfx/ResultVisualizer.cpp
    src/gfx/SpectrumVisualizer.cpp
    src/gfx/SpectrumColorizer.cpp
    src/QmlApi.cpp
    src/SpectrumController.cpp
    ../shared/statistics.cpp
    ../shared/SignalAnalyzer.cpp
    ../shared/noise_floor.cpp
    src/ResultModel.cpp
    src/debug.cpp
    src/gfx/ImageProvider.cpp
)
set(HEADERS
    src/AndroidTest.hpp
    src/DevicesController.hpp
    src/LogModel.hpp
    src/devices/AndroidSerialDeviceManager.hpp
    src/devices/IDeviceManager.hpp
    src/devices/InputStream.hpp
    src/gfx/ImageDisplay.hpp
    src/gfx/ResultVisualizer.hpp
    src/gfx/SpectrumVisualizer.hpp
    src/gfx/SpectrumColorizer.hpp
    src/QmlApi.hpp
    src/SpectrumController.hpp
    ../shared/FiFo.hpp
    ../shared/statistics.hpp
    ../shared/remap.hpp
    ../shared/RingBuffer.hpp
    ../shared/SignalAnalyzer.hpp
    ../shared/noise_floor.h
    src/ResultModel.hpp
    src/debug.hpp
    src/gfx/ImageProvider.hpp
)

qt5_add_resources(RESOURCES qml/qml.qrc)

set(QML_IMPORT_PATH ${CMAKE_BINARY_DIR} CACHE STRING "" FORCE)

if(ANDROID)
    find_package(Qt5 REQUIRED COMPONENTS AndroidExtras)

    add_library(${_NAME} SHARED
        ${SOURCES}
        ${HEADERS}
        ${RESOURCES}
    )

    set_target_properties(${_NAME} PROPERTIES
        AUTOMOC ON
    )

    add_qt_android_apk(my_app_apk ${_NAME}
        PACKAGE_NAME de.citrad.mobileCompanion
        PACKAGE_SOURCES ${CMAKE_CURRENT_LIST_DIR}/android_sources
    )

    target_link_libraries(${_NAME}
        PRIVATE log
        PRIVATE Qt5::AndroidExtras
    )
else()
    add_executable(${_NAME}
        ${SOURCES}
        ${HEADERS}
        ${RESOURCES}
        src/devices/QtSerialDeviceManager.cpp
        src/devices/QtSerialDeviceManager.hpp
    )

    set_target_properties(${_NAME} PROPERTIES
        OUTPUT_NAME ${_NAME}.bin
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        AUTOMOC ON
    )

    target_link_libraries(${_NAME}
        PRIVATE Qt5::SerialPort
    )
endif()

include(GNUInstallDirs)
install(TARGETS ${_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_include_directories(${_NAME} PRIVATE ../)
target_link_libraries(${_NAME}
    PRIVATE Qt5::Quick
)

if(NO_TESTS)
    message("Tests disabled")
else()
    add_subdirectory(tests)
endif()
