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
    Q_PROPERTY(bool monoModeAttached READ monoModeAttached NOTIFY monoModeAttachedChanged)
    Q_PROPERTY(bool overlayStrokePen READ overlayStrokePen NOTIFY overlayStrokePenChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    static EpaperBridge *instance();

    explicit EpaperBridge(QObject *parent = nullptr);
    ~EpaperBridge() override;

    bool available() const { return m_available; }
    bool penModeAttached() const { return m_penModeItem != nullptr; }
    bool monoModeAttached() const { return m_monoModeItem != nullptr; }
    bool overlayStrokePen() const { return m_overlayStrokePen; }
    QString status() const { return m_status; }

    /**
     * Switch ToolCanvasLayer's dedicated EPScreenModeItem between Pen (lasso/marquee in flight)
     * and Mono (settled + move/resize). Does **not** steal CanvasLayer's Pen region.
     */
    Q_INVOKABLE bool setOverlayStrokePen(bool pen);

    /** Attach an EPScreenModeItem(mode=Pen) as a child covering @p host. */
    Q_INVOKABLE bool attachPenModeRegion(QQuickItem *host);
    /**
     * Attach EPScreenModeItem(mode=Mono) covering @p host (ToolCanvasLayer).
     * @implements [ADR-0019] Mono waveform for selection chrome
     * Fallback: returns false — caller keeps tight dirty rects (never full-panel GC16).
     */
    Q_INVOKABLE bool attachMonoModeRegion(QQuickItem *host);

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
    void monoModeAttachedChanged();
    void overlayStrokePenChanged();
    void statusChanged();

private:
    void resolve();
    void setStatus(const QString &s);
    int resolvePenModeValue() const;
    int resolveModeValue(const char *key) const;
    int resolveContentTypeValue() const;
    bool attachScreenMode(QQuickItem *host, int mode, void **storage, QQuickItem **item);

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
    int m_monoMode = -1;
    int m_contentType = 0;
    QString m_status;
    bool m_overlayStrokePen = false;

    // Over-allocated storage for opaque QQuickItem subclasses (size unknown).
    static constexpr int kOpaqueBytes = 4096;
    void *m_penModeStorage = nullptr;
    QQuickItem *m_penModeItem = nullptr;
    void *m_monoModeStorage = nullptr;
    QQuickItem *m_monoModeItem = nullptr;
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
