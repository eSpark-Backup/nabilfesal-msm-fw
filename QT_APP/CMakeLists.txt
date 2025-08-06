cmake_minimum_required(VERSION 3.5)

project(MSN LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets SerialPort VirtualKeyboard REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core)

qt5_add_resources(RESOURCES icons.qrc)

file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

set(GUI
    mainwindow.ui
    login.ui
    channels.ui
)

add_executable(MSN
    ${SOURCES}
    ${HEADERS}
    ${GUI}
    ${RESOURCES}
)

target_link_libraries(MSN PRIVATE Qt5::Widgets Qt5::SerialPort Qt::VirtualKeyboard)
target_link_libraries(MSN PRIVATE Qt5::Core)
