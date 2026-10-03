#include "sketchcanvas.hpp"

SketchCanvas::SketchCanvas(QQuickItem * parent): QQuickPaintedItem(parent) {
    setRenderTarget(QQuickPaintedItem::Image),
    setAntialiasing(false);
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

void SketchCanvas::beginStroke(qreal x, qreal y) {

    m_strokes.append(QVector<QPointF>{QPointF(x, y)});

    QRect dirtyRect = dirtyFor(QPointF(x, y), QPointF(x, y), m_image.rect());

    update(dirtyRect);
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
    
    QPainter p(&m_image);
    // antialiasing adds gray pixels that pen mode cannot show.
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(Qt::black, kPenWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(prev, next);

    // update state
    m_strokes.last().append(next);

    // Mark the dirty region. Though this might not be the final repaint region.
    // Final repaint region is decided by Qt
    update(dirtyFor(prev, next, m_image.rect()));
}

void SketchCanvas::clear()
{
    m_strokes.clear();
    update();
}

/**
 * Final repaint region, decided by Qt, base on our dirty rect
 * They differs in 3 cases
 *
 * 1/ first paint, resize or update with no rectangle -> paints whole item 
 * 2/ several update(dirtyRect) land on one paint(), clip = bounding box of all dirtyRect
 * 3/ qt aligns region to pixels, so the clip can be a pixel wider than the rect you passed.
 */

void SketchCanvas::paint(QPainter *painter) {
    
    // A. Whole buffer
    painter->drawImage(QPoint(0, 0), m_image);

    // B. Partial buffer
    // const QRect dirtyRect = painter->clipBoundingRect().toAlignedRect()
    //     .intersected(m_image.rect());
    // painter->drawImage(dirtyRect, m_image, dirtyRect);
}
