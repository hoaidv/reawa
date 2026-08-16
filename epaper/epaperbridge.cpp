#include "epaperbridge.h"

#include <QCoreApplication>
#include <QMetaEnum>
#include <QMetaObject>
#include <QProcess>
#include <QDebug>
#include <QtGlobal>
#include <algorithm>
#include <cstdlib>
#include <dlfcn.h>

namespace {

template <typename T>
T resolveSym(void *lib, const char *name, QByteArray *log)
{
    // Prefer explicit library handle (Qt plugins are often RTLD_LOCAL).
    void *sym = lib ? dlsym(lib, name) : nullptr;
    if (!sym)
        sym = dlsym(RTLD_DEFAULT, name);
    if (!sym && log)
        *log += QByteArray(" missing ") + name;
    return reinterpret_cast<T>(sym);
}

qint64 percentileUs(QVector<qint64> samples, double p)
{
    if (samples.isEmpty())
        return -1;
    std::sort(samples.begin(), samples.end());
    const int idx = qBound(0, int(p * (samples.size() - 1)), samples.size() - 1);
    return samples[idx];
}

void *openQsgepaper()
{
    // Already loaded by QT_QUICK_BACKEND=epaper — NOLOAD avoids double-init.
    if (void *h = dlopen("/usr/lib/plugins/scenegraph/libqsgepaper.so",
                         RTLD_NOW | RTLD_GLOBAL | RTLD_NOLOAD))
        return h;
    if (void *h = dlopen("/usr/lib/plugins/scenegraph/libqsgepaper.so",
                         RTLD_NOW | RTLD_GLOBAL))
        return h;
    if (void *h = dlopen("libqsgepaper.so", RTLD_NOW | RTLD_GLOBAL | RTLD_NOLOAD))
        return h;
    return dlopen("libqsgepaper.so", RTLD_NOW | RTLD_GLOBAL);
}

} // namespace

EpaperBridge *EpaperBridge::instance()
{
    static EpaperBridge *s = new EpaperBridge(qApp);
    return s;
}

EpaperBridge *epaperBridge()
{
    return EpaperBridge::instance();
}

EpaperBridge::EpaperBridge(QObject *parent)
    : QObject(parent)
{
    m_tracing = qEnvironmentVariableIsSet("RM_INK_TRACE");
    if (m_tracing)
        m_clock.start();
    resolve();
}

EpaperBridge::~EpaperBridge()
{
    if (m_blocker && m_blockerStop)
        m_blockerStop(m_blocker);
    // The opaque items are QQuickItems parented to the canvas, so Qt has already
    // destroyed them; freeing the backing storage here would be a double free.
    m_penModeStorage = nullptr;
    m_monoModeStorage = nullptr;
    m_blockerStorage = nullptr;
    m_penModeItem = nullptr;
    m_monoModeItem = nullptr;
    m_blocker = nullptr;
    if (m_lib) {
        // Do not dlclose — Qt still owns the plugin for the process lifetime.
        m_lib = nullptr;
    }
}

void EpaperBridge::setStatus(const QString &s)
{
    if (m_status == s)
        return;
    m_status = s;
    emit statusChanged();
}

void EpaperBridge::resolve()
{
    QByteArray miss;
    m_lib = openQsgepaper();
    if (!m_lib)
        miss += QByteArray(" dlopen(libqsgepaper) failed: ") + (dlerror() ? dlerror() : "?");

    m_instance = resolveSym<InstanceFn>(
        m_lib, "_ZN13EPFramebuffer8instanceEv", &miss);
    // Prefer newer 4-arg signature (SDK); fall back to 3-arg (some device builds).
    m_swapBuffers4 = resolveSym<SwapBuffers4Fn>(
        m_lib,
        "_ZN13EPFramebuffer11swapBuffersE5QRect13EPContentType12EPScreenMode"
        "6QFlagsINS_10UpdateFlagEE",
        nullptr);
    m_swapBuffers3 = resolveSym<SwapBuffers3Fn>(
        m_lib,
        "_ZN13EPFramebuffer11swapBuffersE5QRect12EPScreenMode"
        "6QFlagsINS_10UpdateFlagEE",
        nullptr);
    if (!m_swapBuffers4 && !m_swapBuffers3)
        miss += " missing swapBuffers(3|4-arg)";

    m_screenModeCtor = resolveSym<ScreenModeCtor>(
        m_lib, "_ZN16EPScreenModeItemC1EP10QQuickItem", &miss);
    m_screenModeMeta = resolveSym<const QMetaObject *>(
        m_lib, "_ZN16EPScreenModeItem16staticMetaObjectE", &miss);
    m_blockerCtor = resolveSym<BlockerCtor>(
        m_lib, "_ZN15EPRenderBlockerC1EP10QQuickItem", &miss);
    m_blockerStart = resolveSym<BlockerVoidFn>(
        m_lib, "_ZN15EPRenderBlocker5startEv", &miss);
    m_blockerStop = resolveSym<BlockerVoidFn>(
        m_lib, "_ZN15EPRenderBlocker4stopEv", &miss);
    m_blockerSetDeadline = resolveSym<BlockerSetDeadlineFn>(
        m_lib, "_ZN15EPRenderBlocker11setDeadlineEi", &miss);

    m_available = m_instance && (m_swapBuffers4 || m_swapBuffers3)
                  && m_screenModeCtor && m_screenModeMeta;
    m_penMode = resolvePenModeValue();
    m_monoMode = resolveModeValue("Mono");
    m_contentType = resolveContentTypeValue();

    // Optional overrides for probing waveform/content enums on-device.
    bool ok = false;
    const int modeOverride = qEnvironmentVariableIntValue("RM_EP_SCREEN_MODE", &ok);
    if (ok)
        m_penMode = modeOverride;
    ok = false;
    const int contentOverride = qEnvironmentVariableIntValue("RM_EP_CONTENT_TYPE", &ok);
    if (ok)
        m_contentType = contentOverride;

    if (m_available && m_penMode < 0) {
        m_available = false;
        setStatus(QStringLiteral("EPScreenModeItem::Mode::Pen not found"));
    } else if (m_available) {
        setStatus(QStringLiteral("libqsgepaper Pen OK (mode=%1 content=%2 swap=%3)%4")
                      .arg(m_penMode)
                      .arg(m_contentType)
                      .arg(m_swapBuffers4 ? QStringLiteral("4arg") : QStringLiteral("3arg"))
                      .arg(miss.isEmpty() ? QString() : QString::fromUtf8(miss)));
        qInfo().noquote() << "[epaperbridge]" << m_status;
    } else {
        setStatus(QStringLiteral("libqsgepaper unavailable:%1").arg(QString::fromUtf8(miss)));
        qWarning().noquote() << "[epaperbridge]" << m_status;
    }
    emit availableChanged();
}

int EpaperBridge::resolvePenModeValue() const
{
    return resolveModeValue("Pen");
}

int EpaperBridge::resolveModeValue(const char *key) const
{
    if (!m_screenModeMeta || !key)
        return -1;
    const int idx = m_screenModeMeta->indexOfEnumerator("Mode");
    if (idx < 0)
        return -1;
    const QMetaEnum me = m_screenModeMeta->enumerator(idx);
    bool ok = false;
    const int v = me.keyToValue(key, &ok);
    return ok ? v : -1;
}

int EpaperBridge::resolveContentTypeValue() const
{
    // EPContentType appears as None / Color / FastGrayscale in the plugin
    // string table (adjacent to EPFramebuffer moc data). Default to None (0)
    // for 1-bit pen ink; override with RM_EP_CONTENT_TYPE if needed.
    return 0;
}

bool EpaperBridge::attachScreenMode(QQuickItem *host, int mode, void **storage, QQuickItem **itemOut)
{
    if (!m_available || !host || !storage || !itemOut || mode < 0)
        return false;
    if (*itemOut) {
        (*itemOut)->setParentItem(host);
        (*itemOut)->setSize(host->size());
        return true;
    }

    *storage = calloc(1, kOpaqueBytes);
    if (!*storage)
        return false;

    m_screenModeCtor(*storage, host);
    auto *obj = reinterpret_cast<QObject *>(*storage);
    auto *item = qobject_cast<QQuickItem *>(obj);
    if (!item) {
        free(*storage);
        *storage = nullptr;
        return false;
    }

    item->setParentItem(host);
    item->setX(0);
    item->setY(0);
    item->setWidth(host->width());
    item->setHeight(host->height());
    QObject::connect(host, &QQuickItem::widthChanged, item, [item, host]() {
        item->setWidth(host->width());
    });
    QObject::connect(host, &QQuickItem::heightChanged, item, [item, host]() {
        item->setHeight(host->height());
    });
    if (!obj->setProperty("mode", mode))
        qWarning() << "[epaperbridge] setProperty(mode) failed" << mode;

    *itemOut = item;
    return true;
}

bool EpaperBridge::attachPenModeRegion(QQuickItem *host)
{
    if (!attachScreenMode(host, m_penMode, &m_penModeStorage, &m_penModeItem))
        return false;
    emit penModeAttachedChanged();
    qInfo() << "[epaperbridge] Pen-mode region attached to" << host;

    if (!m_blocker && m_blockerCtor && m_blockerStart && m_blockerStop) {
        m_blockerStorage = calloc(1, kOpaqueBytes);
        if (m_blockerStorage) {
            m_blockerCtor(m_blockerStorage, host);
            m_blocker = reinterpret_cast<QObject *>(m_blockerStorage);
            if (m_blockerSetDeadline)
                m_blockerSetDeadline(m_blocker, 5000);
            auto *blockerItem = qobject_cast<QQuickItem *>(m_blocker);
            if (blockerItem) {
                blockerItem->setParentItem(host);
                blockerItem->setVisible(false);
            }
        }
    }
    return true;
}

bool EpaperBridge::attachMonoModeRegion(QQuickItem *host)
{
    if (m_monoMode < 0) {
        qWarning() << "[epaperbridge] EPScreenModeItem::Mode::Mono not found";
        return false;
    }
    if (!attachScreenMode(host, m_monoMode, &m_monoModeStorage, &m_monoModeItem))
        return false;
    emit monoModeAttachedChanged();
    qInfo() << "[epaperbridge] Mono-mode region attached to" << host << "mode=" << m_monoMode;
    return true;
}

bool EpaperBridge::setOverlayStrokePen(bool pen)
{
    if (!m_monoModeItem)
        return false;
    const int mode = pen ? m_penMode : m_monoMode;
    if (mode < 0)
        return false;
    if (!m_monoModeItem->setProperty("mode", mode)) {
        qWarning() << "[epaperbridge] overlay setProperty(mode) failed" << (pen ? "Pen" : "Mono");
        return false;
    }
    if (m_overlayStrokePen == pen)
        return true;
    m_overlayStrokePen = pen;
    emit overlayStrokePenChanged();
    qInfo() << "[epaperbridge] ToolCanvas waveform" << (pen ? "Pen" : "Mono");
    return true;
}

void EpaperBridge::swapPen(const QRect &rect)
{
    if (!m_available || !m_instance)
        return;
    if (!m_swapBuffers4 && !m_swapBuffers3)
        return;
    if (rect.isEmpty())
        return;

    void *fb = m_instance();
    if (!fb)
        return;

    const QRect r = rect.normalized();
    if (m_swapBuffers4)
        m_swapBuffers4(fb, r, m_contentType, m_penMode, 0);
    else
        m_swapBuffers3(fb, r, m_penMode, 0);

    if (m_tracing)
        tracePostSwap();
}

void EpaperBridge::beginStrokeBlock()
{
    if (m_blocker && m_blockerStart)
        m_blockerStart(m_blocker);
}

void EpaperBridge::endStrokeBlock()
{
    if (m_blocker && m_blockerStop)
        m_blockerStop(m_blocker);
}

void EpaperBridge::traceArrival()
{
    if (!m_tracing)
        return;
    m_arrivalNs = m_clock.nsecsElapsed();
}

void EpaperBridge::traceFlush()
{
    if (!m_tracing || m_arrivalNs <= 0)
        return;
    const qint64 now = m_clock.nsecsElapsed();
    m_arrivalToFlushUs.append((now - m_arrivalNs) / 1000);
    m_arrivalNs = now;
}

void EpaperBridge::tracePostSwap()
{
    if (!m_tracing || m_arrivalNs <= 0)
        return;
    const qint64 now = m_clock.nsecsElapsed();
    m_flushToSwapUs.append((now - m_arrivalNs) / 1000);
    m_arrivalNs = 0;
}

void EpaperBridge::dumpTraceStats() const
{
    if (!m_tracing)
        return;
    auto dump = [](const char *label, QVector<qint64> samples) {
        if (samples.isEmpty()) {
            qInfo().nospace() << "[ink-trace] " << label << ": (no samples)";
            return;
        }
        qInfo().nospace()
            << "[ink-trace] " << label
            << " n=" << samples.size()
            << " p50=" << percentileUs(samples, 0.50) << "us"
            << " p95=" << percentileUs(samples, 0.95) << "us"
            << " p99=" << percentileUs(samples, 0.99) << "us";
    };
    dump("arrival->flush", m_arrivalToFlushUs);
    dump("flush->swap", m_flushToSwapUs);
}

void EpaperBridge::restoreXochitl()
{
    // Detach first so xochitl starts after we release the panel.
    QProcess::startDetached(QStringLiteral("/bin/sh"),
                            {QStringLiteral("-c"),
                             QStringLiteral("sleep 0.5; systemctl start xochitl")});
    QCoreApplication::quit();
}
