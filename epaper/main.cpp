#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQmlContext>
#include <QTimer>
#include <QDebug>
#include <QtPlugin>

#include <csignal>
#include <memory>

#include "drawing/tabletcanvasitem.h"
#include "drawing/toolcanvasitem.h"
#include "drawing/tabletwindow.h"
#include "epaperbridge.h"
#include "debug/debug_log_ship.h"
#include "debug/ui_stall.hpp"
#include "connectivity/usb_link.hpp"
#include "debug/latency_probe.hpp"
#include "input/qtinputfilter.h"

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

    // Pen and touch are routed by QML pointer handlers, which read the real
    // events. A synth mouse on top of that double-handles every gesture — and
    // worse, it hands the exclusive grab to whatever plain item happens to sit
    // under the finger (a Text label accepts LeftButton), so the chrome
    // TapHandler above it then declines the already-grabbed press.
    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);
    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTabletEvents, false);

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

    qmlRegisterType<ToolCanvasItem>("epaper", 1, 0, "ToolCanvas");
    qmlRegisterType<TabletWindow>("epaper", 1, 0, "TabletWindow");
    qmlRegisterSingletonInstance("epaper", 1, 0, "EpaperBridge", bridge);
    qmlRegisterSingletonInstance("epaper", 1, 0, "UsbHud", usbLink);
    qmlRegisterSingletonInstance("epaper", 1, 0, "DebugLog", debugShip);

    // Raw input facts for Main.qml, which owns the routing policy that reads them.
    auto *inputFilter = new QtInputFilter(&app);
    app.installEventFilter(inputFilter);
    qmlRegisterSingletonInstance("epaper", 1, 0, "Input", inputFilter);

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

    // The only wire between raw input and the document: extra pen channels, which
    // a QML HandlerPoint cannot carry. Everything else the filter knows is read as
    // a property in Main.qml.
    const auto roots = engine.rootObjects();
    for (QObject *root : roots) {
        if (auto *win = qobject_cast<TabletWindow *>(root)) {
            auto conn = std::make_shared<QMetaObject::Connection>();
            auto bind = [inputFilter, conn](TabletCanvasItem *c) {
                if (*conn)
                    QObject::disconnect(*conn);
                if (c) {
                    *conn = QObject::connect(inputFilter, &QtInputFilter::penSample, c,
                                             &TabletCanvasItem::stashTabletSample);
                }
            };
            QObject::connect(win, &TabletWindow::canvasChanged, inputFilter, bind);
            bind(win->canvas());
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
