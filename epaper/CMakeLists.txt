cmake_minimum_required(VERSION 3.16)
project(epaper VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Gui)

qt_add_executable(epaper_bin main.cpp)
target_link_libraries(epaper_bin PRIVATE Qt6::Gui)
target_compile_options(epaper_bin PRIVATE -Wno-psabi)
set_target_properties(epaper_bin PROPERTIES
    OUTPUT_NAME epaper
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
