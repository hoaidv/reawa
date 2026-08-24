cmake_minimum_required(VERSION 3.16)
project(epaper VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Network)
find_package(Threads REQUIRED)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(epaper_bin
    main.cpp
    tabletcanvasitem.cpp
    toolcanvasitem.cpp
    tabletwindow.cpp
    regionsync/strokesync.cpp
    epaperbridge.cpp
    debuglog/debug_log_ship.cpp
    debuglog/ui_stall.cpp
    connectivity/usbgadget.cpp
    connectivity/usb_link.cpp
    input/qtinputfilter.cpp
)
qt_add_qml_module(epaper_bin
    URI epaper
    VERSION 1.0
    QML_FILES Main.qml ResizeKnob.qml
)

# Designer icons (rasterized from .plan/.../system/assets/icon-epaper-*.svg)
qt_add_resources(epaper_bin "epaper_tool_icons"
    PREFIX "/icons"
    FILES
        icons/icon-epaper-selection.png
        icons/icon-epaper-selection-inv.png
        icons/icon-epaper-sel-rect.png
        icons/icon-epaper-sel-rect-inv.png
        icons/icon-epaper-sel-freeform.png
        icons/icon-epaper-sel-freeform-inv.png
        icons/icon-epaper-pen.png
        icons/icon-epaper-pen-inv.png
        icons/icon-epaper-ink-box.png
        icons/icon-epaper-ink-box-inv.png
        icons/icon-epaper-enclose.png
        icons/icon-epaper-enclose-inv.png
        icons/icon-epaper-undo.png
        icons/icon-epaper-redo.png
        icons/icon-epaper-switch.png
        icons/icon-epaper-usb-unplugged.png
        icons/icon-epaper-usb-plugged.png
        icons/icon-epaper-usb-connected.png
        icons/icon-epaper-recog-ink-box.png
        icons/icon-epaper-recog-ink-box-inv.png
        icons/icon-epaper-recog-connector.png
        icons/icon-epaper-recog-connector-inv.png
        icons/icon-epaper-viewport-follow.png
        icons/icon-epaper-viewport-follow-inv.png
        icons/icon-epaper-hand.png
        icons/icon-epaper-hand-inv.png
)

target_link_libraries(epaper_bin PRIVATE Qt6::Quick Qt6::Network Threads::Threads ${CMAKE_DL_LIBS})
set_target_properties(epaper_bin PROPERTIES
    OUTPUT_NAME epaper
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
