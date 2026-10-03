#include "sketchcanvas.hpp"

SketchCanvas::SketchCanvas(QQuickItem * parent): QQuickPaintedItem(parent) {
    setRenderTarget(QQuickPaintedItem::Image);
    setAntialiasing(true);
}

void SketchCanvas::paint(QPainter *painter) {
    painter->setRenderHint(QPainter::Antialiasing);
    painter->fillRect(boundingRect(), Qt::white);
    QPen pen(Qt::black, 2);

    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pen);
    for(const auto &stroke: m_strokes) {
        if (stroke.size() >= 2) {
            painter->drawPolyline(stroke.constData(), stroke.size());
        }
    }

}

void SketchCanvas::beginStroke(qreal x, qreal y) {

    m_strokes.append(QVector<QPointF>{QPointF(x, y)});
    update();
}

void SketchCanvas::extendStroke(qreal x, qreal y){
    if (m_strokes.isEmpty()) 
        return;

    m_strokes.last().append(QPointF(x, y));
    update();
}

void SketchCanvas::clear()
{
    m_strokes.clear();
    update();
}