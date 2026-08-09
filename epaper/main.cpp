#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtPlugin>

#include "tabletcanvasitem.h"
#include "tabletwindow.h"
#include "tabletappfilter.h"

int main(int argc, char *argv[])
{
#ifdef __arm__
    qputenv("QMLSCENE_DEVICE", "epaper");
    qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");
    qputenv("QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS", "rotate=180:invertx");
    qputenv("QT_QPA_GENERIC_PLUGINS", "evdevtablet");
    qputenv("QT_QUICK_BACKEND", "epaper");
#endif

    QGuiApplication app(argc, argv);

    qmlRegisterType<TabletCanvasItem>("epaper", 1, 0, "TabletCanvas");
    qmlRegisterType<TabletWindow>("epaper", 1, 0, "TabletWindow");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("epaper", "Main");

    auto *filter = new TabletAppFilter(&app);
    app.installEventFilter(filter);
    const auto roots = engine.rootObjects();
    for (QObject *root : roots) {
        if (auto *win = qobject_cast<TabletWindow *>(root)) {
            QObject::connect(win, &TabletWindow::canvasChanged, filter, [filter](TabletCanvasItem *c) {
                filter->setCanvas(c);
            });
            filter->setCanvas(win->canvas());
            break;
        }
    }

    return app.exec();
}
