cmake_minimum_required(VERSION 3.5)

project(MSM LANGUAGES CXX)

if(BUILD_X86)
    add_definitions(-DBUILD_X86)
elseif(BUILD_V3S)
    add_definitions(-DBUILD_V3S)
endif()

set(BUILD_INFO_HEADER "${CMAKE_BINARY_DIR}/buildinfo.h")

add_custom_command(
    OUTPUT ${BUILD_INFO_HEADER}
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE=${BUILD_INFO_HEADER}
        -P ${CMAKE_SOURCE_DIR}/GenerateBuildInfo.cmake
    COMMENT "Generating build info..."
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS -fuse-ld=gold)

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

target_link_libraries(MSM PRIVATE Qt5::Core Qt5::Widgets Qt5::SerialPort Qt5::Network Qt5::Mqtt)

file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR})


add_custom_target(generate_build_info DEPENDS ${BUILD_INFO_HEADER})
add_dependencies(MSM generate_build_info)
target_include_directories(MSM PRIVATE ${CMAKE_BINARY_DIR})