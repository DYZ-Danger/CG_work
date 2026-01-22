cmake_minimum_required(VERSION 3.15)
project(VdbToRaw LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置vcpkg路径,请根据实际路径修改
set(CMAKE_TOOLCHAIN_FILE "D:/Desktop/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake")

# 强制使用MSVC
if(NOT MSVC)
    message(FATAL_ERROR "VdbToRaw must be built with MSVC!")
endif()

find_package(OpenVDB REQUIRED)

add_executable(VdbToRaw VdbToRaw.cpp)
target_link_libraries(VdbToRaw PRIVATE OpenVDB::openvdb)

# 输出到 ../data 目录
set_target_properties(VdbToRaw PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../data"
)
