#pragma once

#include <QObject>
#include <QQuickItem>
#include <QRect>
#include <QByteArray>
#include <QVector>
#include <QElapsedTimer>

class QMetaObject;

/**
 * Runtime bridge to reMarkable's libqsgepaper.so (SWTCON / Pen-mode APIs).
 * @implements [SRS-EP-01]
 *
 * All symbols resolved via dlsym(RTLD_DEFAULT). Missing symbols => available()==false
 * and callers fall back to stock Qt Quick epaper refreshes.
 */
class EpaperBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool penModeAttached READ penModeAttached NOTIFY penModeAttachedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    static EpaperBridge *instance();

    explicit EpaperBridge(QObject *parent = nullptr);
    ~EpaperBridge() override;

    bool available() const { return m_available; }
    bool penModeAttached() const { return m_penModeItem != nullptr; }
    QString status() const { return m_status; }

    /** Attach an EPScreenModeItem(mode=Pen) as a child covering @p host. */
    Q_INVOKABLE bool attachPenModeRegion(QQuickItem *host);

    /** Partial update @p rect with Pen waveform (no-op if unavailable). */
    Q_INVOKABLE void swapPen(const QRect &rect);

    /** Suppress scene-graph-driven updates during a stroke burst. */
    Q_INVOKABLE void beginStrokeBlock();
    Q_INVOKABLE void endStrokeBlock();

    // --- Ink latency tracing (RM_INK_TRACE=1) ---
    bool tracing() const { return m_tracing; }
    void traceArrival();
    void traceFlush();
    void tracePostSwap();
    void dumpTraceStats() const;

signals:
    void availableChanged();
    void penModeAttachedChanged();
    void statusChanged();

private:
    void resolve();
    void setStatus(const QString &s);
    int resolvePenModeValue() const;
    int resolveContentTypeValue() const;

    using InstanceFn = void *(*)();
    /** Newer SDK: swapBuffers(QRect, EPContentType, EPScreenMode, flags) */
    using SwapBuffers4Fn = void (*)(void *self, QRect rect, int contentType, int mode, int flags);
    /** Older device: swapBuffers(QRect, EPScreenMode, flags) */
    using SwapBuffers3Fn = void (*)(void *self, QRect rect, int mode, int flags);
    using ScreenModeCtor = void (*)(void *self, QQuickItem *parent);
    using BlockerCtor = void (*)(void *self, QQuickItem *parent);
    using BlockerVoidFn = void (*)(void *self);
    using BlockerSetDeadlineFn = void (*)(void *self, int deadline);

    InstanceFn m_instance = nullptr;
    SwapBuffers4Fn m_swapBuffers4 = nullptr;
    SwapBuffers3Fn m_swapBuffers3 = nullptr;
    ScreenModeCtor m_screenModeCtor = nullptr;
    const QMetaObject *m_screenModeMeta = nullptr;
    BlockerCtor m_blockerCtor = nullptr;
    BlockerVoidFn m_blockerStart = nullptr;
    BlockerVoidFn m_blockerStop = nullptr;
    BlockerSetDeadlineFn m_blockerSetDeadline = nullptr;

    void *m_lib = nullptr;
    bool m_available = false;
    int m_penMode = -1;
    int m_contentType = 0;
    QString m_status;

    // Over-allocated storage for opaque QQuickItem subclasses (size unknown).
    static constexpr int kOpaqueBytes = 4096;
    void *m_penModeStorage = nullptr;
    QQuickItem *m_penModeItem = nullptr;
    void *m_blockerStorage = nullptr;
    QObject *m_blocker = nullptr;

    bool m_tracing = false;
    QElapsedTimer m_clock;
    qint64 m_arrivalNs = 0;
    QVector<qint64> m_arrivalToFlushUs;
    QVector<qint64> m_flushToSwapUs;
};

/**
 * Singleton accessor registered into QML as "EpaperBridge".
 * @implements [SRS-EP-01]
 */
EpaperBridge *epaperBridge();
