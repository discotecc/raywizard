cmake_minimum_required(VERSION 3.16)

project(RayWizard
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCES
    src/main.cpp
)

add_executable(rtiow ${SOURCES})

target_include_directories(rtiow PRIVATE src)

set_target_properties(rtiow PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(rtiow PRIVATE /W4 /permissive-)
else()
    target_compile_options(rtiow PRIVATE -Wall -Wextra -Wpedantic)
endif()