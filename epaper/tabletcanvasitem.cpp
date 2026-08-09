#include "tabletcanvasitem.h"
#include "strokesync.h"

#include <QPainter>
#include <QPen>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWindow>

TabletCanvasItem::TabletCanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_sync(new StrokeSync(this))
{
    setAntialiasing(false);
    setOpaquePainting(false);
    setFillColor(Qt::transparent);
    m_sync->connectToMac();
}

void TabletCanvasItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        ensurePixmap();
}

void TabletCanvasItem::ensurePixmap()
{
    const int w = qMax(1, int(width()));
    const int h = qMax(1, int(height()));
    setTextureSize(QSize(w, h));
    m_pixmap = QPixmap(w, h);
    m_pixmap.fill(Qt::white);

    QPainter p(&m_pixmap);
    p.setRenderHint(QPainter::Antialiasing, false);
    QPen pen(Qt::black);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    for (const auto &stroke : m_strokes) {
        for (int i = 1; i < stroke.size(); ++i) {
            const auto &a = stroke[i - 1];
            const auto &b = stroke[i];
            pen.setWidthF(1.0 + a.pressure * 4.0);
            p.setPen(pen);
            p.drawLine(a.pos, b.pos);
        }
    }
}

QPointF TabletCanvasItem::mapInputToCanvas(const QPointF &raw) const
{
    // [SRS-EP-01] Panel is portrait (w x h); device is used in landscape.
    // Qt reports pen coords with digitizer axes swapped + unevenly scaled.
    // Rotate into an isotropic landscape frame that still fits the portrait scene:
    //   renderX = penY * (w/h)
    //   renderY = h - penX * (h/w)
    qreal w = width();
    qreal h = height();
    if (w < 2.0 && window())
        w = window()->width();
    if (h < 2.0 && window())
        h = window()->height();
    w = qMax<qreal>(1.0, w);
    h = qMax<qreal>(1.0, h);

    const qreal rx = raw.y() * (w / h);
    const qreal ry = h - raw.x() * (h / w);
    return QPointF(rx, ry);
}

void TabletCanvasItem::setDebug(const QPointF &raw, const QPointF &mapped)
{
    m_lastPoint = mapped;
    m_debugInfo = QStringLiteral("raw(%1,%2) map(%3,%4) sz(%5x%6)")
                      .arg(int(raw.x()))
                      .arg(int(raw.y()))
                      .arg(int(mapped.x()))
                      .arg(int(mapped.y()))
                      .arg(int(width()))
                      .arg(int(height()));
    // NB: no emit here. Debug text is refreshed at the throttled cadence
    // (see appendPoint / begin / end) to avoid flooding the e-ink queue.
}

void TabletCanvasItem::ingestPoint(QEvent::Type type, const QPointF &pos, qreal pressure)
{
    const qreal p = qBound<qreal>(0.0, pressure, 1.0);
    const QPointF canvasPos = mapInputToCanvas(pos);
    setDebug(pos, canvasPos);

    switch (type) {
    case QEvent::TabletPress:
    case QEvent::MouseButtonPress:
        beginStroke(canvasPos, p);
        break;
    case QEvent::TabletMove:
    case QEvent::MouseMove:
        appendPoint(canvasPos, p);
        break;
    case QEvent::TabletRelease:
    case QEvent::MouseButtonRelease:
        endStroke();
        break;
    default:
        break;
    }
}

void TabletCanvasItem::paint(QPainter *painter)
{
    // Ink is rendered by the QML node pool; this item exists only to receive
    // input and provide a full-window coordinate frame. Draw nothing.
    Q_UNUSED(painter);
}

void TabletCanvasItem::drawSegment(const Point &from, const Point &to)
{
    if (m_pixmap.isNull())
        ensurePixmap();

    QPainter p(&m_pixmap);
    p.setRenderHint(QPainter::Antialiasing, false);
    QPen pen(Qt::black);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setWidthF(1.0 + from.pressure * 4.0);
    p.setPen(pen);
    p.drawLine(from.pos, to.pos);
}

void TabletCanvasItem::forceRefresh()
{
    update();
    if (auto *win = window())
        win->update();
}

void TabletCanvasItem::syncBegin()
{
    QJsonObject obj{
        {"type", "stroke_begin"},
        {"id", m_activeStrokeId},
        {"brush", QJsonObject{{"width", 2.0}}},
        {"cw", width()},
        {"ch", height()},
    };
    m_sync->sendLine(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void TabletCanvasItem::syncPoint(const Point &pt)
{
    QJsonObject obj{
        {"type", "stroke_point"},
        {"id", m_activeStrokeId},
        {"x", pt.pos.x()},
        {"y", pt.pos.y()},
        {"p", pt.pressure},
    };
    m_sync->sendLine(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void TabletCanvasItem::syncEnd()
{
    QJsonObject obj{
        {"type", "stroke_end"},
        {"id", m_activeStrokeId},
    };
    m_sync->sendLine(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void TabletCanvasItem::beginStroke(const QPointF &canvasPos, qreal pressure)
{
    m_current.clear();
    m_activeStrokeId = QStringLiteral("s-%1").arg(++m_strokeSeq);
    m_current.append({canvasPos, pressure});
    m_lastEmitted = canvasPos;
    m_hasEmitted = false;
    syncBegin();
    syncPoint(m_current.last());
    emit debugChanged();
    // No forced repaint: QML renders the ink layer when the model changes.
}

void TabletCanvasItem::appendPoint(const QPointF &canvasPos, qreal pressure)
{
    if (m_current.isEmpty()) {
        beginStroke(canvasPos, pressure);
        return;
    }
    Point next{canvasPos, pressure};
    m_current.append(next);
    syncPoint(next);

    // Throttle QML segment creation: only spawn a delegate once the pen has
    // moved a meaningful distance, so an input burst can't flood the scene
    // graph with thousands of rectangles and starve the event loop.
    const qreal dx = canvasPos.x() - m_lastEmitted.x();
    const qreal dy = canvasPos.y() - m_lastEmitted.y();
    if (!m_hasEmitted || (dx * dx + dy * dy) >= 9.0) {
        emit segmentDrawn(m_lastEmitted.x(), m_lastEmitted.y(),
                          canvasPos.x(), canvasPos.y(),
                          1.0 + pressure * 4.0);
        m_lastEmitted = canvasPos;
        m_hasEmitted = true;
        emit debugChanged();
    }
}

void TabletCanvasItem::endStroke()
{
    if (m_current.size() >= 2) {
        // Emit the tail so the last bit of the stroke is drawn.
        const Point &last = m_current.last();
        if (last.pos != m_lastEmitted)
            emit segmentDrawn(m_lastEmitted.x(), m_lastEmitted.y(),
                              last.pos.x(), last.pos.y(),
                              1.0 + last.pressure * 4.0);
        m_strokes.append(m_current);
        ++m_strokeCount;
        emit strokeCountChanged();
        syncEnd();
    }
    m_current.clear();
    m_hasEmitted = false;
}
