#pragma once

#include <QElapsedTimer>
#include <QPainter>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

#include "bridge/epaperbridge.h"

// Pen-to-ink item. Batches dirty rectangles, attaches one panel waveform, and
// draws each segment into a single image. The point list is only enough to
// connect the previous sample to the next; it moves out of this item later.
class NativeCanvas : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Waveform waveform READ waveform WRITE setWaveform NOTIFY waveformChanged)
    Q_PROPERTY(int batchWindowMs READ batchWindowMs WRITE setBatchWindowMs NOTIFY batchWindowMsChanged)
    Q_PROPERTY(qreal penWidth READ penWidth WRITE setPenWidth NOTIFY penWidthChanged)

public:
    // Panel refresh used for this item's damage. ScreenMode leaves the panel
    // on its ordinary waveform. Further EPScreenMode values can join this list.
    enum Waveform { PenMode, MonoMode, ScreenMode };
    Q_ENUM(Waveform)

    // QML assigns the properties above before componentComplete().
    explicit NativeCanvas(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    Q_INVOKABLE void beginStroke(qreal x, qreal y);
    Q_INVOKABLE void extendStroke(qreal x, qreal y);
    Q_INVOKABLE void endStroke();
    Q_INVOKABLE void clear();

    Waveform waveform() const { return m_waveform; }
    void setWaveform(Waveform waveform);

    int batchWindowMs() const { return m_batchWindowMs; }
    void setBatchWindowMs(int batchWindowMs);

    qreal penWidth() const { return m_penWidth; }
    void setPenWidth(qreal penWidth);

signals:
    void waveformChanged();
    void batchWindowMsChanged();
    void penWidthChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void applyWaveform();
    void ensureImage();
    QRect dirtyFor(const QPointF &a, const QPointF &b) const;
    void noteDirty(const QRect &rect);
    void flush();

    Waveform m_waveform = PenMode;
    int m_batchWindowMs = 8;
    qreal m_penWidth = 4;

    QVector<QPointF> m_stroke;
    QImage m_image;
    QRect m_pendingFlush;
    QElapsedTimer m_flushTimer;
};

inline NativeCanvas::NativeCanvas(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setRenderTarget(QQuickPaintedItem::Image);
    setOpaquePainting(true);
    setFillColor(Qt::white);
}

inline void NativeCanvas::setWaveform(Waveform waveform)
{
    if (m_waveform == waveform)
        return;
    m_waveform = waveform;
    applyWaveform();
    emit waveformChanged();
}

inline void NativeCanvas::setBatchWindowMs(int batchWindowMs)
{
    const int window = qMax(0, batchWindowMs);
    if (m_batchWindowMs == window)
        return;
    m_batchWindowMs = window;
    emit batchWindowMsChanged();
}

inline void NativeCanvas::setPenWidth(qreal penWidth)
{
    const qreal width = qMax<qreal>(1, penWidth);
    if (qFuzzyCompare(m_penWidth, width))
        return;
    m_penWidth = width;
    emit penWidthChanged();
}

inline void NativeCanvas::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    applyWaveform();
}

inline void NativeCanvas::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    // A 0x0 region tags nothing, so attach again once the item has a real size.
    if (newGeometry.width() > 1.0 && newGeometry.height() > 1.0
        && newGeometry.size() != oldGeometry.size())
        applyWaveform();
}

inline void NativeCanvas::applyWaveform()
{
    if (width() <= 1.0 || height() <= 1.0)
        return;

    EpaperBridge *bridge = EpaperBridge::instance();
    switch (m_waveform) {
    case PenMode:
        bridge->attachPenModeRegion(this);
        break;
    case MonoMode:
        bridge->attachMonoModeRegion(this);
        break;
    case ScreenMode:
        break;
    }
}

inline void NativeCanvas::ensureImage()
{
    const int w = qCeil(width());
    const int h = qCeil(height());
    if (w < 1 || h < 1)
        return;
    if (m_image.size() == QSize(w, h))
        return;

    m_image = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    m_image.fill(Qt::white);
}

inline QRect NativeCanvas::dirtyFor(const QPointF &a, const QPointF &b) const
{
    const qreal pad = m_penWidth * 0.5 + 8;
    return QRectF(a, b).normalized()
        .adjusted(-pad, -pad, pad, pad)
        .toAlignedRect()
        .intersected(m_image.rect());
}

inline void NativeCanvas::noteDirty(const QRect &rect)
{
    m_pendingFlush = m_pendingFlush.isNull() ? rect : m_pendingFlush.united(rect);

    // First sample of the process has no clock yet. Start it and wait out the window.
    // A later stroke whose clock is already past the window flushes this rect now.
    if (!m_flushTimer.isValid())
        m_flushTimer.start();
    else if (m_flushTimer.elapsed() >= m_batchWindowMs)
        flush();
}

inline void NativeCanvas::flush()
{
    if (m_pendingFlush.isNull())
        return;

    const QRect rect = m_pendingFlush;
    m_pendingFlush = QRect();
    m_flushTimer.restart();
    update(rect);
}

inline void NativeCanvas::beginStroke(qreal x, qreal y)
{
    ensureImage();
    if (m_image.isNull())
        return;

    const QPointF next(x, y);
    QPainter painter(&m_image);
    painter.setPen(QPen(Qt::black, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPoint(next);

    noteDirty(dirtyFor(next, next));
    m_stroke = {next};
}

inline void NativeCanvas::extendStroke(qreal x, qreal y)
{
    if (m_stroke.isEmpty())
        return;

    ensureImage();
    if (m_image.isNull())
        return;

    const QPointF prev = m_stroke.last();
    const QPointF next(x, y);
    QPainter painter(&m_image);
    painter.setPen(QPen(Qt::black, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(prev, next);

    noteDirty(dirtyFor(prev, next));
    m_stroke.append(next);
}

inline void NativeCanvas::endStroke()
{
    flush();
    m_stroke.clear();
}

inline void NativeCanvas::clear()
{
    m_stroke.clear();
    update();
}

inline void NativeCanvas::paint(QPainter *painter)
{
    // The scene graph clips this painter to the rect passed to update().
    // Drawing the whole buffer at the item origin copies only that piece.
    painter->drawImage(0, 0, m_image);
}
