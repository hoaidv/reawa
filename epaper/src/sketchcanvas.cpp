#include "sketchcanvas.hpp"
#include "bridge/epaperbridge.h"

SketchCanvas::SketchCanvas(QQuickItem * parent): QQuickPaintedItem(parent) {
    // Opaque white RGB. Pen mode is 1-bit; gray or alpha pixels show up dashed.
    setAntialiasing(true);
    setRenderTarget(QQuickPaintedItem::Image);
    setOpaquePainting(true);
    setFillColor(Qt::white);
}

void SketchCanvas::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    // Same object QML calls as EpaperBridgeInstance. A 0x0 region tags nothing,
    // and onCompleted can run before layout, so geometryChange attaches again.
    EpaperBridge::instance()->attachPenModeRegion(this);
}

void SketchCanvas::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.width() > 1.0 && newGeometry.height() > 1.0
        && newGeometry.size() != oldGeometry.size())
        EpaperBridge::instance()->attachPenModeRegion(this);
}

void SketchCanvas::ensureImage() {
    const int w = qCeil(width());
    const int h = qCeil(height());

    if (w < 1 || h < 1) return;
    if (m_image.size() == QSize(w, h)) return;

    m_image = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    m_image.fill(Qt::white);
}


QRect SketchCanvas::dirtyFor(const QPointF &a, const QPointF &b, const QRect &bounds)
{
    return QRectF(a, b).normalized()
        .adjusted(-kPad, -kPad, kPad, kPad)
        .toAlignedRect()
        .intersected(bounds);
}

void SketchCanvas::noteDirty(const QRect &r) {
    m_pendingFlush = m_pendingFlush.isNull() ? r : m_pendingFlush.united(r);
    if (m_pendingFlushTimer.isValid() && m_pendingFlushTimer.elapsed() >= kFlushMs) {
        flush();
    }
}

void SketchCanvas::beginStroke(qreal x, qreal y) {

    ensureImage();
    
    const QPointF next(x, y);
    // drawing
    QPainter p(&m_image);
    p.setPen(QPen(Qt::black, kPenWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPoint(next);

    // Flushing logic
    m_pendingFlush = QRect();
    m_pendingFlushTimer.start();

    noteDirty(dirtyFor(next, next, m_image.rect()));

    // update state
    m_strokes.append(QVector<QPointF>{next});    
}

void SketchCanvas::extendStroke(qreal x, qreal y){
    // Safe-guard corrupted state, with no stroke but trying to extend
    if (m_strokes.isEmpty()) return;

    QVector<QPointF> lastStroke = m_strokes.last();
    // Safe-guard currupted state, stroke with empty data
    if (m_strokes.last().isEmpty()) { 
        m_strokes.last().append(QPointF(x, y));
        return;
    }

    ensureImage();
    // Safe-guard non-existing buffer
    if (m_image.isNull()) return;

    const QPointF prev = m_strokes.last().last();
    const QPointF next(x, y);
    
    // drawing on buffer
    QPainter p(&m_image);
    p.setPen(QPen(Qt::black, kPenWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(prev, next);
    
    // Mark the dirty region
    noteDirty(dirtyFor(prev, next, m_image.rect()));

    // ------------------------------------------------------------------
    // update state
    m_strokes.last().append(next);
}

void SketchCanvas::endStroke()
{
    flush();
}

void SketchCanvas::clear()
{
    m_strokes.clear();
    update();
}

void SketchCanvas::paint(QPainter *painter) {
    // The scene graph already clips this painter to the rect passed to update().
    // Drawing the whole buffer at item origin copies only that clipped piece.
    painter->drawImage(0, 0, m_image);
}
