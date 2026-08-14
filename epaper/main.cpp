#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QDebug>
#include <QtPlugin>

#include <csignal>

#include "tabletcanvasitem.h"
#include "toolcanvasitem.h"
#include "tabletwindow.h"
#include "tabletappfilter.h"
#include "epaperbridge.h"
#include "debuglog/debug_log_ship.h"
#include "latencyprobe/stub_document.hpp"

namespace {

volatile std::sig_atomic_t g_termRequested = 0;

void requestTerm(int) { g_termRequested = 1; }

} // namespace

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

    // Sidecar :9878 — worker thread; env-gated. Before QML so paint never installs it.
    auto *debugShip = new DebugLogShip(&app);
    debugShip->startIfEnabled();

    // Resolve libqsgepaper symbols once the QPA/epaper plugins are loaded.
    EpaperBridge *bridge = EpaperBridge::instance();

    qmlRegisterType<TabletCanvasItem>("epaper", 1, 0, "TabletCanvas");
    qmlRegisterType<ToolCanvasItem>("epaper", 1, 0, "ToolCanvas");
    qmlRegisterType<TabletWindow>("epaper", 1, 0, "TabletWindow");
    qmlRegisterSingletonInstance("epaper", 1, 0, "EpaperBridge", bridge);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("epaper", "Main");

    // Exit through the event loop so trace stats are still dumped on killall.
    std::signal(SIGTERM, requestTerm);
    std::signal(SIGINT, requestTerm);
    auto *termPoll = new QTimer(&app);
    termPoll->setInterval(200);
    QObject::connect(termPoll, &QTimer::timeout, &app, [&app]() {
        if (g_termRequested)
            app.quit();
    });
    termPoll->start();

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

    const int rc = app.exec();
    bridge->dumpTraceStats();
    if (epaper::latencyprobe::harness().enabled()) {
        const QString dump = QString::fromStdString(epaper::latencyprobe::harness().dumpText());
        const auto lines = dump.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const QString &line : lines)
            qInfo().noquote() << line;
    }
    for (QObject *root : engine.rootObjects()) {
        if (auto *win = qobject_cast<TabletWindow *>(root)) {
            if (TabletCanvasItem *c = win->canvas()) {
                const QString dump = QString::fromStdString(c->ingestDumpText());
                const auto lines = dump.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                for (const QString &line : lines)
                    qInfo().noquote() << line;
            }
            break;
        }
    }
    return rc;
}
