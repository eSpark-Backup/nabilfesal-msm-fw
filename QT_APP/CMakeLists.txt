cmake_minimum_required(VERSION 3.5)

project(MSM LANGUAGES CXX)

if(BUILD_X86)
    add_definitions(-DBUILD_X86)
elseif(BUILD_V3S)
    add_definitions(-DBUILD_V3S)
endif()

# Build date and time
string(TIMESTAMP BUILD_DATE "%b %d %Y")
string(TIMESTAMP BUILD_TIME "%H:%M:%S")

# Get git author email of latest commit
execute_process(
    COMMAND git log -1 --pretty=format:%ae
    OUTPUT_VARIABLE BUILD_AUTHOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get git commit subject/title
execute_process(
    COMMAND git log -1 --pretty=format:%s
    OUTPUT_VARIABLE GIT_COMMIT_TITLE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get git commit short hash
execute_process(
    COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Combine: "title [hash]"
set(GIT_COMMIT_FULL "${GIT_COMMIT_TITLE} [${GIT_COMMIT_HASH}]")

# Pass definitions to the compiler
add_definitions(-DBUILD_DATE=\"${BUILD_DATE}\")
add_definitions(-DBUILD_TIME=\"${BUILD_TIME}\")
add_definitions(-DBUILD_AUTHOR=\"${BUILD_AUTHOR}\")
add_definitions(-DGIT_COMMIT_FULL=\"${GIT_COMMIT_FULL}\")


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
