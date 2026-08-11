#include "tabletcanvasitem.h"
#include "strokesync.h"
#include "epaperbridge.h"

#include <QPainter>
#include <QPainterPath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QQuickWindow>
#include <QtMath>
#include <QByteArray>
#include <QTransform>
#include <QTimer>
#include <QPen>

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

QString normalizeOrientation(const QString &raw)
{
    if (raw == QLatin1String("portrait") || raw == QLatin1String("gutToLeft"))
        return QStringLiteral("gutToLeft");
    if (raw == QLatin1String("landscape") || raw == QLatin1String("gutOnTop"))
        return QStringLiteral("gutOnTop");
    if (raw == QLatin1String("gutAtBottom") || raw == QLatin1String("gutToRight"))
        return raw;
    return QStringLiteral("gutToLeft");
}

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
    m_refreshClock.start();
    connect(m_sync, &StrokeSync::hostMessage, this, &TabletCanvasItem::onHostMessage);
    m_sync->connectToMac();
    updateToolChipRect();
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
        updateToolChipRect();
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
    // Verified Round 19 (RENDERING.md): panel framebuffer is portrait; digitizer
    // reports landscape-oriented raw coords. Always apply this for local ink —
    // Infini orientation only changes the sync-frame aspect / world UV, not this.
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
    m_lastRaw = pos;

    // Pen on ToolChip — not ink; may arm (pen-on-chip fallback). Handled in QML taps primarily.
    if (pointInToolChip(canvasPos)
        && (type == QEvent::TabletPress || type == QEvent::MouseButtonPress)) {
        return;
    }

    switch (type) {
    case QEvent::TabletPress:
    case QEvent::MouseButtonPress:
        if (m_toolMode == QLatin1String("selection"))
            beginSelectionGesture(canvasPos);
        else
            beginStroke(canvasPos, p);
        break;
    case QEvent::TabletMove:
    case QEvent::MouseMove:
        if (m_selectionGesture)
            updateSelectionGesture(canvasPos);
        else
            appendPoint(canvasPos, p);
        break;
    case QEvent::TabletRelease:
    case QEvent::MouseButtonRelease:
        if (m_selectionGesture)
            endSelectionGesture();
        else
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
    // ADR-0012: live ink uses world width × current panel scale (matches zoomed vectors).
    const qreal worldW = worldStrokeWidth(from.pressure);
    const qreal lineW = qMax<qreal>(1.0, worldW * panelScale());

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
    m_activeWorldStrokeWidth = worldStrokeWidth(pressure);
    m_current.append({canvasPos, pressure, m_lastRaw});
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

    Point next{canvasPos, pressure, m_lastRaw};
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

    appendLocalStrokeAsWorldPath();

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
        // World units — Infini stores as-is (ADR-0012).
        {"brush", QJsonObject{{"width", m_activeWorldStrokeWidth}}},
        {"cw", width()},
        {"ch", height()},
    };
    // SRS-EP-04 / SRS-IN-13 — intent only; tool mode stays device-local.
    if (m_toolMode == QLatin1String("ink_box"))
        obj.insert(QStringLiteral("intent"), QStringLiteral("enclose"));
    else
        obj.insert(QStringLiteral("intent"), QStringLiteral("ink"));
    m_sync->sendLine(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void TabletCanvasItem::syncPoint(const Point &pt)
{
    // Wire uses panel-framebuffer coords (after digitizer map). Infini UV is then
    // orientation-aware against the sync frame — do not send pre-map raw (double-rotate).
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

void TabletCanvasItem::onHostMessage(const QJsonObject &obj)
{
    const QString type = obj.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("viewport")) {
        applyViewport(obj);
        return;
    }
    if (type == QLatin1String("doc_snapshot")) {
        applyDocSnapshot(obj);
        return;
    }
    // Legacy PNG path ignored — vectors only (ADR-0009 / human: no bitmap sync).
    if (type == QLatin1String("region_refresh")) {
        qInfo() << "[sync] ignoring region_refresh bitmap; use doc_snapshot";
        return;
    }
}

void TabletCanvasItem::applyViewport(const QJsonObject &obj)
{
    m_viewportSeq = obj.value(QStringLiteral("seq")).toInt(m_viewportSeq);
    const QString orient = obj.value(QStringLiteral("orientation")).toString();
    if (!orient.isEmpty()) {
        m_orientation = normalizeOrientation(orient);
        updateToolChipRect();
    }

    const QJsonObject dr = obj.value(QStringLiteral("drawingRegion")).toObject();
    if (!dr.isEmpty()) {
        m_drawingRegion.minX = dr.value(QStringLiteral("minX")).toDouble();
        m_drawingRegion.minY = dr.value(QStringLiteral("minY")).toDouble();
        m_drawingRegion.maxX = dr.value(QStringLiteral("maxX")).toDouble();
        m_drawingRegion.maxY = dr.value(QStringLiteral("maxY")).toDouble();
        m_drawingRegion.valid = m_drawingRegion.maxX > m_drawingRegion.minX
            && m_drawingRegion.maxY > m_drawingRegion.minY;
    }

    const bool settle = obj.value(QStringLiteral("settle")).toBool(false);
    qInfo() << "[sync] viewport seq" << m_viewportSeq << "orientation" << m_orientation
            << "settle" << settle << "nodes" << m_vectorNodes.size();
    scheduleVectorRasterize(settle);
}

void TabletCanvasItem::applyDocSnapshot(const QJsonObject &obj)
{
    m_vectorNodes = obj.value(QStringLiteral("nodes")).toArray();
    m_pickables = obj.value(QStringLiteral("pickables")).toArray();
    // Ghost discarded — authoritative geometry (SRS-EP-04 / SRS-IN-13).
    m_selectionGesture = false;
    m_gesturePickableId.clear();
    qInfo() << "[sync] doc_snapshot nodes" << m_vectorNodes.size()
            << "pickables" << m_pickables.size();
    emit pickablesChanged();
    scheduleVectorRasterize(true);
}

void TabletCanvasItem::setToolMode(const QString &mode)
{
    QString next = mode;
    if (next != QLatin1String("pen") && next != QLatin1String("ink_box")
        && next != QLatin1String("selection")) {
        next = QStringLiteral("pen");
    }
    if (m_toolMode == next)
        return;
    m_toolMode = next;
    // Selection inert without pickables — still allow arming; UI shows count.
    emit toolModeChanged();
    m_debugInfo = QStringLiteral("tool=%1 pickables=%2")
                      .arg(m_toolMode)
                      .arg(m_pickables.size());
    emit debugChanged();
}

void TabletCanvasItem::armTool(const QString &mode)
{
    setToolMode(mode);
}

void TabletCanvasItem::updateToolChipRect()
{
    // UI-EP-01 amended (human verify 2026-08-11): ≥64px tiles (was 32).
    // @implements [SRS-EP-05] floating ToolChip hit bounds
    const qreal chipH = 64.0;
    const qreal chipW = 64.0 * 3.0;
    const qreal inset = 8.0;
    qreal top = inset;
    // gutOnTop → oriented top is opposite short edge (bottom of panel coords).
    if (m_orientation == QLatin1String("gutOnTop"))
        top = height() - inset - chipH;
    const qreal left = (width() - chipW) * 0.5;
    const QRectF next(left, top, chipW, chipH);
    if (next == m_toolChipRect)
        return;
    m_toolChipRect = next;
    emit toolChipRectChanged();
}

bool TabletCanvasItem::pointInToolChip(const QPointF &canvasPos) const
{
    return m_toolChipRect.contains(canvasPos);
}

QString TabletCanvasItem::hitPickable(const QPointF &world) const
{
    // Topmost = last in array (SRS-EP-04).
    for (int i = m_pickables.size() - 1; i >= 0; --i) {
        const QJsonObject p = m_pickables.at(i).toObject();
        const QJsonObject b = p.value(QStringLiteral("bounds")).toObject();
        const double minX = b.value(QStringLiteral("minX")).toDouble();
        const double minY = b.value(QStringLiteral("minY")).toDouble();
        const double maxX = b.value(QStringLiteral("maxX")).toDouble();
        const double maxY = b.value(QStringLiteral("maxY")).toDouble();
        if (world.x() >= minX && world.x() <= maxX && world.y() >= minY && world.y() <= maxY)
            return p.value(QStringLiteral("id")).toString();
    }
    return {};
}

void TabletCanvasItem::beginSelectionGesture(const QPointF &canvasPos)
{
    if (m_pickables.isEmpty())
        return;
    const QPointF world = panelToWorld(canvasPos);
    const QString id = hitPickable(world);
    if (id.isEmpty()) {
        m_selectedPickableId.clear();
        return;
    }
    m_selectedPickableId = id;
    m_gesturePickableId = id;
    m_gestureStartWorld = world;
    m_gestureLastWorld = world;
    m_selectionGesture = true;
}

void TabletCanvasItem::updateSelectionGesture(const QPointF &canvasPos)
{
    if (!m_selectionGesture)
        return;
    m_gestureLastWorld = panelToWorld(canvasPos);
}

void TabletCanvasItem::endSelectionGesture()
{
    if (!m_selectionGesture)
        return;
    m_selectionGesture = false;
    const double dx = m_gestureLastWorld.x() - m_gestureStartWorld.x();
    const double dy = m_gestureLastWorld.y() - m_gestureStartWorld.y();
    ++m_toolIntentSeq;
    QJsonObject obj{{"type", "tool_intent"},
                    {"nodeId", m_gesturePickableId},
                    {"seq", m_toolIntentSeq}};
    if (qHypot(dx, dy) < 2.0) {
        obj.insert(QStringLiteral("action"), QStringLiteral("select"));
    } else {
        obj.insert(QStringLiteral("action"), QStringLiteral("move"));
        obj.insert(QStringLiteral("delta"), QJsonObject{{"dx", dx}, {"dy", dy}});
    }
    syncToolIntent(obj);
    m_gesturePickableId.clear();
}

void TabletCanvasItem::syncToolIntent(const QJsonObject &obj)
{
    m_sync->sendLine(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void TabletCanvasItem::scheduleVectorRasterize(bool sharp)
{
    if (sharp)
        m_rasterizeSharp = true;
    if (m_rasterizePending && !sharp) {
        // Already scheduled; keep pending soft refresh.
        return;
    }
    if (sharp) {
        // Settle: paint now for sharp GC / full redraw.
        m_rasterizePending = false;
        rasterizeVectors(true);
        m_rasterizeSharp = false;
        return;
    }
    m_rasterizePending = true;
    QTimer::singleShot(int(kRefreshMinIntervalMs), this, [this]() {
        if (!m_rasterizePending)
            return;
        m_rasterizePending = false;
        const bool sharp = m_rasterizeSharp;
        m_rasterizeSharp = false;
        rasterizeVectors(sharp);
    });
}

bool TabletCanvasItem::orientationLandscape() const
{
    return m_orientation == QLatin1String("gutOnTop")
        || m_orientation == QLatin1String("gutAtBottom")
        || m_orientation == QLatin1String("landscape");
}

bool TabletCanvasItem::orientationInvertX() const
{
    return m_orientation == QLatin1String("gutAtBottom")
        || m_orientation == QLatin1String("gutToRight");
}

bool TabletCanvasItem::orientationInvertY() const
{
    return m_orientation == QLatin1String("gutAtBottom")
        || m_orientation == QLatin1String("gutToRight");
}

double TabletCanvasItem::panelScale() const
{
    if (!m_drawingRegion.valid)
        return 1.0;
    const double rw = m_drawingRegion.maxX - m_drawingRegion.minX;
    if (rw <= 0.0)
        return 1.0;
    return width() / rw;
}

qreal TabletCanvasItem::worldStrokeWidth(qreal pressure) const
{
    const qreal p = qBound<qreal>(0.0, pressure, 1.0);
    // Match Infini demo ink (~2.5 world) with light pressure modulation.
    return kBaseWorldStroke * (0.7 + 0.3 * p);
}

void TabletCanvasItem::panelToFrameUv(double localX, double localY, double *u, double *v) const
{
    const qreal pw = qMax<qreal>(1.0, width());
    const qreal ph = qMax<qreal>(1.0, height());
    double nx = 0;
    double ny = 0;
    if (orientationLandscape()) {
        nx = 1.0 - localY / ph;
        ny = localX / pw;
    } else {
        nx = localX / pw;
        ny = localY / ph;
    }
    if (orientationInvertX())
        nx = 1.0 - nx;
    if (orientationInvertY())
        ny = 1.0 - ny;
    *u = nx;
    *v = ny;
}

void TabletCanvasItem::frameUvToPanel(double u, double v, double *x, double *y) const
{
    const qreal pw = qMax<qreal>(1.0, width());
    const qreal ph = qMax<qreal>(1.0, height());
    double nx = u;
    double ny = v;
    if (orientationInvertX())
        nx = 1.0 - nx;
    if (orientationInvertY())
        ny = 1.0 - ny;
    if (orientationLandscape()) {
        *x = ny * pw;
        *y = (1.0 - nx) * ph;
    } else {
        *x = nx * pw;
        *y = ny * ph;
    }
}

QPointF TabletCanvasItem::worldToPanel(double wx, double wy) const
{
    if (!m_drawingRegion.valid)
        return QPointF(wx, wy);
    const double rw = m_drawingRegion.maxX - m_drawingRegion.minX;
    const double rh = m_drawingRegion.maxY - m_drawingRegion.minY;
    if (rw <= 0 || rh <= 0)
        return QPointF();
    const double u = (wx - m_drawingRegion.minX) / rw;
    const double v = (wy - m_drawingRegion.minY) / rh;
    double x = 0;
    double y = 0;
    frameUvToPanel(u, v, &x, &y);
    return QPointF(x, y);
}

QPointF TabletCanvasItem::panelToWorld(const QPointF &panel) const
{
    if (!m_drawingRegion.valid)
        return panel;
    double u = 0;
    double v = 0;
    panelToFrameUv(panel.x(), panel.y(), &u, &v);
    const double rw = m_drawingRegion.maxX - m_drawingRegion.minX;
    const double rh = m_drawingRegion.maxY - m_drawingRegion.minY;
    return QPointF(m_drawingRegion.minX + u * rw, m_drawingRegion.minY + v * rh);
}

void TabletCanvasItem::appendLocalStrokeAsWorldPath()
{
    if (m_current.size() < 2 || !m_drawingRegion.valid)
        return;
    QJsonArray pts;
    for (const Point &pt : m_current) {
        const QPointF w = panelToWorld(pt.pos);
        pts.append(QJsonObject{{"x", w.x()}, {"y", w.y()}});
    }
    m_vectorNodes.append(QJsonObject{
        {"kind", "path"},
        {"id", m_activeStrokeId},
        {"points", pts},
        {"strokeWidth", m_activeWorldStrokeWidth},
    });
}

void TabletCanvasItem::drawVectorNode(QPainter &p, const QJsonObject &node)
{
    const QString kind = node.value(QStringLiteral("kind")).toString();
    const qreal worldSw = node.value(QStringLiteral("strokeWidth")).toDouble(2.0);
    const double rw = m_drawingRegion.maxX - m_drawingRegion.minX;
    const double sPanel = width() / qMax(1e-6, rw);
    const qreal lineW = qMax<qreal>(1.0, worldSw * sPanel);

    QPen pen(Qt::black);
    pen.setWidthF(lineW);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    if (kind == QLatin1String("line")) {
        const QPointF a = worldToPanel(node.value(QStringLiteral("x1")).toDouble(),
                                       node.value(QStringLiteral("y1")).toDouble());
        const QPointF b = worldToPanel(node.value(QStringLiteral("x2")).toDouble(),
                                       node.value(QStringLiteral("y2")).toDouble());
        p.drawLine(a, b);
        return;
    }
    if (kind == QLatin1String("rect")) {
        const QPointF tl = worldToPanel(node.value(QStringLiteral("x")).toDouble(),
                                        node.value(QStringLiteral("y")).toDouble());
        const QPointF br = worldToPanel(
            node.value(QStringLiteral("x")).toDouble() + node.value(QStringLiteral("w")).toDouble(),
            node.value(QStringLiteral("y")).toDouble() + node.value(QStringLiteral("h")).toDouble());
        p.drawRect(QRectF(tl, br).normalized());
        return;
    }
    if (kind == QLatin1String("ellipse")) {
        const double cx = node.value(QStringLiteral("cx")).toDouble();
        const double cy = node.value(QStringLiteral("cy")).toDouble();
        const double rx = node.value(QStringLiteral("rx")).toDouble();
        const double ry = node.value(QStringLiteral("ry")).toDouble();
        const QPointF c = worldToPanel(cx, cy);
        const QPointF e = worldToPanel(cx + rx, cy + ry);
        const qreal prx = qAbs(e.x() - c.x());
        const qreal pry = qAbs(e.y() - c.y());
        p.drawEllipse(c, prx, pry);
        return;
    }
    if (kind == QLatin1String("path")) {
        const QJsonArray pts = node.value(QStringLiteral("points")).toArray();
        if (pts.size() < 2)
            return;
        QPainterPath path;
        const QJsonObject p0 = pts.at(0).toObject();
        path.moveTo(worldToPanel(p0.value(QStringLiteral("x")).toDouble(),
                                 p0.value(QStringLiteral("y")).toDouble()));
        for (int i = 1; i < pts.size(); ++i) {
            const QJsonObject pi = pts.at(i).toObject();
            path.lineTo(worldToPanel(pi.value(QStringLiteral("x")).toDouble(),
                                     pi.value(QStringLiteral("y")).toDouble()));
        }
        p.drawPath(path);
    }
}

void TabletCanvasItem::rasterizeVectors(bool sharp)
{
    if (!m_paintsInk || !m_drawingRegion.valid)
        return;
    ensureImage();
    if (m_image.isNull())
        return;

    QPainter p(&m_image);
    // Full white clear + vector redraw → sharp when settle (sharp=true).
    p.fillRect(m_image.rect(), Qt::white);
    p.setRenderHint(QPainter::Antialiasing, sharp);
    for (const QJsonValue &v : m_vectorNodes) {
        if (v.isObject())
            drawVectorNode(p, v.toObject());
    }
    p.end();

    stampStaticBeacon();
    m_refreshClock.restart();
    // Full-rect update for settle sharpness; soft path still full redraw but may
    // look faded on e-ink until a later settle refresh.
    update(m_image.rect());
    if (sharp && qEnvironmentVariableIsSet("RM_EP_SWAP")) {
        if (auto *win = window()) {
            const QRect scene = mapRectToScene(QRectF(m_image.rect())).toAlignedRect();
            QObject::connect(
                win,
                &QQuickWindow::afterRendering,
                this,
                [scene]() { EpaperBridge::instance()->swapPen(scene); },
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
            win->update();
        }
    }
    qInfo() << "[sync] vector rasterize nodes" << m_vectorNodes.size() << "sharp" << sharp
            << "seq" << m_viewportSeq;
}
