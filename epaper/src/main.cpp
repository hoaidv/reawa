// @implements [STORY-EP-081] empty reMarkable 2 shell
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>
#include "app.hpp"
#include <QQmlContext>
#include "bridge/epaperbridge.h"

int main(int argc, char *argv[])
{
#ifdef __arm__
    qputenv("QMLSCENE_DEVICE", "epaper");
    qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");
    qputenv("QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS", "rotate=180:invertx");
    qputenv("QT_QPA_GENERIC_PLUGINS", "evdevtablet");
    qputenv("QT_QUICK_BACKEND", "epaper");
#endif

    // QGuiApplication, not QApplication: this is Qt Quick / QML, no Widgets.
    // Construction starts the QPA plugin (window system, here typically xcb →
    // Xwayland in the guest), parses Qt CLI flags out of argc/argv, and is the
    // object that owns the event loop we enter at the bottom.
    QGuiApplication app(argc, argv);

    // QML name of this one C++ object: import epaper 1.0, then EpaperBridgeInstance.
    // The type name must start with an uppercase letter; Qt rejects anything else.
    // Keep the URI "epaper". Registering into "QtLearn" replaces that module and
    // NativeCanvas is no longer a QML type.
    EpaperBridge *bridge = EpaperBridge::instance();
    qmlRegisterSingletonInstance("epaper", 1, 0, "EpaperBridgeInstance", bridge);

    // Process identity for QSettings, QStandardPaths, and D-Bus names.
    // On Linux this lands under ~/.config/epaper/epaper.conf and similar
    // app-specific dirs. Set before anything that reads those paths.
    app.setOrganizationName("epaper");
    app.setApplicationName("epaper");

    // Qt Quick Controls 2 style — MUST be set before any Controls QML loads.
    // "Basic" is the bundled platform-neutral style. Native styles (Fusion,
    // Imagine, …) can fail or look wrong with the software renderer on this VM.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // QQmlApplicationEngine compiles QML and instantiates the root object.
    // CMake `qt_add_qml_module(... NO_RESOURCE_TARGET_PATH)` embeds files at
    // their source paths, so Main.qml is qrc:/src/qml/Main.qml
    App logic;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &logic);
    const QUrl url(QStringLiteral("qrc:/src/qml/Main.qml"));

    // Fail the process if the root QML object cannot be created (syntax error,
    // missing import, bad qrc path). Without this, exec() would spin an empty
    // event loop and look like a hang. Compare objUrl to `url` so a later
    // incidental load does not quit the app. QueuedConnection defers the exit
    // until after this signal has finished delivering.
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, 
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    // Instantiate a ApplicationWindow in the qml.
    engine.load(url);

    // Block here dispatching input, timers, and scene-graph frames until the
    // last window closes or something calls quit() / exit().
    return app.exec();
    
}
