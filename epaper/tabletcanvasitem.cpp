#include "tabletcanvasitem.h"
#include "strokesync.h"
#include "epaperbridge.h"

#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWindow>
#include <QtMath>

namespace {

bool envFlag(const char *name, bool fallback)
{
    if (!qEnvironmentVariableIsSet(name))
        return fallback;
    const QByteArray v = qgetenv(name).trimmed().toLower();
    return !(v == "0" || v == "false" || v == "off" || v == "no");
}

/** Fixed screen slots for the render-path beacons (EXP-0001 Round 22). */
constexpr int kStaticBeaconX = 40;
constexpr int kStaticBeaconY = 60;
constexpr int kStaticBeaconSize = 120;
constexpr int kFlushBeaconX = 200;
constexpr int kFlushBeaconY = 60;
constexpr int kFlushBeaconSize = 60;

} // namespace

TabletCanvasItem::TabletCanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_sync(new StrokeSync(this))
{
    m_paintsInk = qgetenv("RM_INK_MODE").trimmed().toLower() != "pool";
    m_beacons = envFlag("RM_INK_BEACON", false);

    setAntialiasing(false);
    setRenderTarget(QQuickPaintedItem::Image);
    setOpaquePainting(m_paintsInk);
    setFillColor(m_paintsInk ? QColor(Qt::white) : QColor(Qt::transparent));
    m_flushClock.start();
    m_sync->connectToMac();
}

void TabletCanvasItem::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    qInfo() << "[ink] componentComplete size" << size()
            << "visible" << isVisible()
            << "opacity" << opacity()
            << "hasContents" << flags().testFlag(ItemHasContents)
            << "parentItem" << parentItem()
            << "parentSize" << (parentItem() ? parentItem()->size() : QSizeF());
    EpaperBridge::instance()->attachPenModeRegion(this);
}

void TabletCanvasItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    qInfo() << "[ink] geometryChange" << oldGeometry << "->" << newGeometry;
    if (newGeometry.size() != oldGeometry.size()
        && newGeometry.width() > 1.0 && newGeometry.height() > 1.0) {
        ensureImage();
        EpaperBridge::instance()->attachPenModeRegion(this);
        update();
    }
}

QSGNode *TabletCanvasItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    QSGNode *node = QQuickPaintedItem::updatePaintNode(oldNode, data);
    if (m_updateNodeLogs < 6) {
        ++m_updateNodeLogs;
        qInfo() << "[ink] updatePaintNode size" << size()
                << "textureSize" << textureSize()
                << "contentsScale" << contentsScale()
                << "old" << oldNode << "new" << node;
    }
    return node;
}

void TabletCanvasItem::ensureImage()
{
    if (!m_paintsInk)
        return;

    const QSize want(qMax(1, int(width())), qMax(1, int(height())));
    if (m_image.size() == want)
        return;

    QImage grown(want, QImage::Format_RGB32);
    grown.fill(Qt::white);
    if (!m_image.isNull()) {
        QPainter p(&grown);
        p.drawImage(0, 0, m_image);
    }
    m_image = grown;
    setTextureSize(want);
    stampStaticBeacon();
}

void TabletCanvasItem::stampStaticBeacon()
{
    if (!m_beacons || m_image.isNull())
        return;

    // Painted once, never touched again: if this square is missing on screen the
    // painter node never reaches the panel at all.
    QPainter p(&m_image);
    p.fillRect(kStaticBeaconX, kStaticBeaconY, kStaticBeaconSize, kStaticBeaconSize, Qt::black);
    p.fillRect(kStaticBeaconX + 30, kStaticBeaconY + 30, kStaticBeaconSize - 60, kStaticBeaconSize - 60, Qt::white);
}

void TabletCanvasItem::stampFlushBeacon()
{
    if (!m_beacons || m_image.isNull())
        return;

    // Toggled on every flush without any geometry change: if the static beacon
    // shows but this one never blinks, content-only damage is being dropped.
    const bool on = (m_flushCount % 2) == 0;
    QPainter p(&m_image);
    p.fillRect(kFlushBeaconX, kFlushBeaconY, kFlushBeaconSize, kFlushBeaconSize,
               on ? Qt::black : Qt::white);

    const QRectF r(kFlushBeaconX, kFlushBeaconY, kFlushBeaconSize, kFlushBeaconSize);
    m_pendingDirty = m_pendingDirty.isNull() ? r : m_pendingDirty.united(r);
}

QPointF TabletCanvasItem::mapInputToCanvas(const QPointF &raw) const
{
    // [SRS-EP-01] Panel is portrait (w x h); device is used in landscape.
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

void TabletCanvasItem::ingestPoint(QEvent::Type type, const QPointF &pos, qreal pressure)
{
    EpaperBridge::instance()->traceArrival();

    const qreal p = qBound<qreal>(0.0, pressure, 1.0);
    const QPointF canvasPos = mapInputToCanvas(pos);
    m_lastPoint = canvasPos;

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
    if (!m_paintsInk)
        return;

    ensureImage();
    m_paintCount.fetchAndAddRelaxed(1);
    painter->drawImage(0, 0, m_image);
}

void TabletCanvasItem::paintSegment(const Point &from, const Point &to, qreal lineWidth)
{
    ensureImage();
    if (m_image.isNull())
        return;

    QPainter p(&m_image);
    p.setRenderHint(QPainter::Antialiasing, false);
    QPen pen(Qt::black);
    pen.setWidthF(lineWidth);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    if (from.pos == to.pos)
        p.drawPoint(from.pos);
    else
        p.drawLine(from.pos, to.pos);
}

void TabletCanvasItem::emitSegment(const Point &from, const Point &to)
{
    const qreal lineW = qMax<qreal>(4.0, 1.0 + from.pressure * 4.0);

    if (m_paintsInk)
        paintSegment(from, to, lineW);
    else
        emit segmentDrawn(from.pos.x(), from.pos.y(), to.pos.x(), to.pos.y(), lineW);

    const qreal pad = lineW * 0.5 + 2.0;
    const QRectF rf = QRectF(from.pos, to.pos).normalized().adjusted(-pad, -pad, pad, pad);
    m_pendingDirty = m_pendingDirty.isNull() ? rf : m_pendingDirty.united(rf);
}

void TabletCanvasItem::flushPending()
{
    if (m_pendingDirty.isNull())
        return;

    EpaperBridge::instance()->traceFlush();
    ++m_flushCount;
    stampFlushBeacon();

    const QRect local = m_pendingDirty.toAlignedRect();
    m_pendingDirty = QRectF();
    m_flushClock.restart();

    if (m_paintsInk)
        update(local);

    if (qEnvironmentVariableIsSet("RM_EP_SWAP")) {
        if (auto *win = window()) {
            const QRect scene = mapRectToScene(QRectF(local)).toAlignedRect();
            QObject::connect(
                win,
                &QQuickWindow::afterRendering,
                this,
                [scene]() { EpaperBridge::instance()->swapPen(scene); },
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
            win->update();
        }
    }
}

void TabletCanvasItem::beginStroke(const QPointF &canvasPos, qreal pressure)
{
    m_current.clear();
    m_activeStrokeId = QStringLiteral("s-%1").arg(++m_strokeSeq);
    m_current.append({canvasPos, pressure});
    m_lastEmitted = m_current.last();
    m_hasEmitted = false;
    m_strokeActive = true;
    m_pendingDirty = QRectF();
    m_flushClock.restart();

    syncBegin();
    syncPoint(m_current.last());

    emitSegment(m_lastEmitted, m_lastEmitted);
    m_hasEmitted = true;
    flushPending();
}

void TabletCanvasItem::appendPoint(const QPointF &canvasPos, qreal pressure)
{
    if (!m_strokeActive || m_current.isEmpty()) {
        beginStroke(canvasPos, pressure);
        return;
    }

    Point next{canvasPos, pressure};
    m_current.append(next);
    syncPoint(next);

    emitSegment(m_lastEmitted, next);
    m_lastEmitted = next;

    if (!m_hasEmitted || m_flushClock.elapsed() >= kFlushIntervalMs) {
        m_hasEmitted = true;
        flushPending();
    }
}

void TabletCanvasItem::endStroke()
{
    if (!m_strokeActive)
        return;

    if (m_current.size() >= 2) {
        const Point &last = m_current.last();
        if (last.pos != m_lastEmitted.pos)
            emitSegment(m_lastEmitted, last);
        ++m_strokeCount;
        emit strokeCountChanged();
        syncEnd();
    }

    flushPending();

    m_current.clear();
    m_hasEmitted = false;
    m_strokeActive = false;

    // Status text is refreshed between strokes only: during a stroke it would
    // add a second damage region per flush.
    m_debugInfo = QStringLiteral("(%1,%2) sz=%3x%4 flush=%5 paint=%6")
                      .arg(int(m_lastPoint.x()))
                      .arg(int(m_lastPoint.y()))
                      .arg(int(width()))
                      .arg(int(height()))
                      .arg(m_flushCount)
                      .arg(m_paintCount.loadRelaxed());
    emit debugChanged();
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
