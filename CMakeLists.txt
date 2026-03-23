cmake_minimum_required(VERSION 3.16)

project(DeviceLink VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort Test)

set(DEVICE_LINK_CORE_SOURCES
    src/mainwindow.cpp
    src/mainwindow.h
    src/serialportmanager.cpp
    src/serialportmanager.h
    src/protocolparser.cpp
    src/protocolparser.h
    src/protocolcodec.cpp
    src/protocolcodec.h
    src/devicestate.cpp
    src/devicestate.h
)

qt_add_library(DeviceLinkCore STATIC
    ${DEVICE_LINK_CORE_SOURCES}
)

target_include_directories(DeviceLinkCore PUBLIC src)

target_link_libraries(DeviceLinkCore PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
)

qt_add_executable(DeviceLink
    src/main.cpp
)

target_link_libraries(DeviceLink PRIVATE
    DeviceLinkCore
)

qt_add_executable(DeviceLinkTests
    tests/test_protocol.cpp
)

target_link_libraries(DeviceLinkTests PRIVATE
    DeviceLinkCore
    Qt6::Test
)

add_test(NAME DeviceLinkTests COMMAND DeviceLinkTests)
