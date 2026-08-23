#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQmlContext>
#include <QTimer>
#include <QDebug>
#include <QtPlugin>

#include <csignal>

#include "tabletcanvasitem.h"
#include "toolcanvasitem.h"
#include "tabletwindow.h"
#include "epaperbridge.h"
#include "debuglog/debug_log_ship.h"
#include "usb_link.hpp"
#include "ui_stall.hpp"
#include "latencyprobe/stub_document.hpp"
#include "gesture/tabletgestures.h"
#include "gesture/canvaspointeritem.h"

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

    // Infra: USB gadget stay-up (worker thread). Independent of TCP apps.
    auto *usbLink = new epaper::UsbLink(&app);
    usbLink->start();
    epaper::startUiStallWatchdog();

    // Sidecar :9878 — worker thread; env-gated. Before QML so paint never installs it.
    auto *debugShip = new DebugLogShip(&app);
    debugShip->startIfEnabled();
    QObject::connect(usbLink, &epaper::UsbLink::requestAppReconnect, debugShip,
                     &DebugLogShip::armReconnect);

    // Resolve libqsgepaper symbols once the QPA/epaper plugins are loaded.
    EpaperBridge *bridge = EpaperBridge::instance();

    qmlRegisterType<TabletCanvasItem>("epaper", 1, 0, "TabletCanvas");
    qmlRegisterType<CanvasPointerItem>("epaper", 1, 0, "CanvasPointer");

    qmlRegisterType<ToolCanvasItem>("epaper", 1, 0, "ToolCanvas");
    qmlRegisterType<TabletWindow>("epaper", 1, 0, "TabletWindow");
    qmlRegisterSingletonInstance("epaper", 1, 0, "EpaperBridge", bridge);
    qmlRegisterSingletonInstance("epaper", 1, 0, "UsbHud", usbLink);
    qmlRegisterSingletonInstance("epaper", 1, 0, "DebugLog", debugShip);

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

    auto *gestures = new TabletGestures(&app);
    app.installEventFilter(gestures);
    const auto roots = engine.rootObjects();
    for (QObject *root : roots) {
        if (auto *win = qobject_cast<TabletWindow *>(root)) {
            QObject::connect(win, &TabletWindow::canvasChanged, gestures,
                            [gestures](TabletCanvasItem *c) { gestures->setCanvas(c); });
            gestures->setCanvas(win->canvas());
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
