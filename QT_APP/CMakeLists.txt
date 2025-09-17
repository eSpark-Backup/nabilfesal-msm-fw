cmake_minimum_required(VERSION 3.5)

project(MSM LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS -fuse-ld=gold)

# FIX: Use correct casing and combine components
find_package(Qt5 REQUIRED COMPONENTS Core Widgets SerialPort Network Mqtt)

qt5_add_resources(RESOURCES icons.qrc)

file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")
file(GLOB GUI "*.ui")

add_executable(MSM
    ${SOURCES}
    ${HEADERS}
    ${GUI}
    ${RESOURCES}
)

# FIX: Correct target names
target_link_libraries(MSM PRIVATE Qt5::Core Qt5::Widgets Qt5::SerialPort Qt5::Network Qt5::Mqtt)

file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR})
