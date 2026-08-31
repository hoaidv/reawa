cmake_minimum_required(VERSION 3.16)
project(epaper VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Network)
find_package(Threads REQUIRED)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(epaper_bin
    main.cpp
    drawing/canvas_session.cpp
    drawing/tabletcanvasitem.cpp
    drawing/toolcanvasitem.cpp
    drawing/tools/input_hub.cpp
    drawing/tools/ui/selection_overlay.cpp
    drawing/tools/contexts/tool_context_impl.cpp
    drawing/tools/ui/action_list_model.cpp
    drawing/tools/ui/selection_context_bar.cpp
    drawing/tabletwindow.cpp
    rendering/rendering.cpp
    regionsync/strokesync.cpp
    epaperbridge.cpp
    debug/debug_log_ship.cpp
    debug/ui_stall.cpp
    connectivity/usbgadget.cpp
    connectivity/usb_link.cpp
    input/qtinputfilter.cpp
)
set_source_files_properties(drawing/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(drawing/ToolCanvas.qml PROPERTIES QT_RESOURCE_ALIAS ToolCanvas.qml)
set_source_files_properties(drawing/tools/ui/SelectionContextToolbar.qml PROPERTIES QT_RESOURCE_ALIAS SelectionContextToolbar.qml)
qt_add_qml_module(epaper_bin
    URI epaper
    VERSION 1.0
    QML_FILES drawing/Main.qml drawing/ToolCanvas.qml
              drawing/tools/ui/SelectionContextToolbar.qml
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
        icons/icon-epaper-cut.png
        icons/icon-epaper-copy.png
        icons/icon-epaper-paste.png
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
        icons/icon-epaper-erase-brush.png
        icons/icon-epaper-erase-brush-inv.png
        icons/icon-epaper-erase-area.png
        icons/icon-epaper-erase-area-inv.png
        icons/icon-epaper-erase-object.png
        icons/icon-epaper-erase-object-inv.png
)

target_link_libraries(epaper_bin PRIVATE Qt6::Quick Qt6::Network Threads::Threads ${CMAKE_DL_LIBS})
target_include_directories(epaper_bin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(epaper_bin PRIVATE -Wno-psabi)
set_target_properties(epaper_bin PROPERTIES
    OUTPUT_NAME epaper
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
