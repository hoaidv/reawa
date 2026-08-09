cmake_minimum_required(VERSION 3.16)
project(epaper VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Network)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(epaper_bin
    main.cpp
    tabletcanvasitem.cpp
    tabletwindow.cpp
    tabletappfilter.cpp
    strokesync.cpp
)
qt_add_qml_module(epaper_bin
    URI epaper
    VERSION 1.0
    QML_FILES Main.qml
)

target_link_libraries(epaper_bin PRIVATE Qt6::Quick Qt6::Network)
set_target_properties(epaper_bin PROPERTIES OUTPUT_NAME epaper)
