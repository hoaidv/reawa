cmake_minimum_required(VERSION 3.16)
project(epaper VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_add_executable(epaper_bin
    src/main.cpp
    src/counter.hpp
    src/app.hpp
    src/app.cpp
    src/bridge/epaperbridge.cpp
)

qt_add_qml_module(epaper_bin
    URI QtLearn
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
    SOURCES
        src/native_canvas.hpp
        src/window/native_window.hpp
        src/window/native_window.cpp
        src/window/stylus_handler.hpp
        src/window/stylus_handler.cpp
    NO_RESOURCE_TARGET_PATH
)

target_include_directories(epaper_bin PRIVATE src src/window)

target_link_libraries(epaper_bin PRIVATE 
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    ${CMAKE_DL_LIBS}
)
target_compile_options(epaper_bin PRIVATE -Wno-psabi)
set_target_properties(epaper_bin PROPERTIES
    OUTPUT_NAME epaper
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
