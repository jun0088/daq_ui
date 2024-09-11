cmake_minimum_required(VERSION 3.15)
project(daq_ui)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")

include("build/generators/conan_toolchain.cmake")
include("build/generators/cmakedeps_macros.cmake")
message("include ${CMAKE_INCLUDE_PATH}")

find_package(ZeroMQ REQUIRED)
find_package(imgui REQUIRED)


if(ZeroMQ_FOUND)
    include_directories(${ZeroMQ_INCLUDE_DIR})
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE MY_SOURCES "src/lib/*.c" "src/lib/*.h" "src/lib/*.cpp")
include_directories(src/lib)
include_directories(external/Collections-C/src/include)
add_subdirectory(external/Collections-C)

add_executable(daq_ui  src/main.cpp ${MY_SOURCES})
# add_executable(daq_ui  src/example_implot.cpp ${MY_SOURCES})

message("linking to ${ZeroMQ_LIBRARIES}")
target_link_libraries(${PROJECT_NAME} libzmq-static)
target_link_libraries(${PROJECT_NAME} imgui::imgui)
target_link_libraries(${PROJECT_NAME} d3d11)
target_link_libraries(${PROJECT_NAME} collectc)