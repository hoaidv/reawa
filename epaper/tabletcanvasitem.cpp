#include "tabletcanvasitem.h"
#include "strokesync.h"
#include "epaperbridge.h"
#include "latencyprobe/stub_document.hpp"
#include "document/ingest_stroke.hpp"
#include "document/recognize_enclose.hpp"
#include "document/membership.hpp"
#include "debuglog/debug_log_format.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QQuickWindow>
#include <QCoreApplication>
#include <QtMath>
#include <QByteArray>
#include <QTransform>
#include <QTimer>
#include <QPen>
#include <cstdio>
#include <string>
#include <vector>

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

/** RM_DOC_PROBE=1 — ingest-path stub only; never read from paint(). */
bool g_docProbe = false;

void armDocProbeFromEnv()
{
    const bool synth = envFlag("RM_DOC_PROBE_SYNTH", false);
    if (!envFlag("RM_DOC_PROBE", false) && !synth)
        return;
    auto &h = epaper::latencyprobe::harness();
    if (envFlag("RM_DOC_PROBE_EVERY_SAMPLE", false))
        h.setEverySample(true);
    h.enable();
    g_docProbe = true;
    qInfo() << "[doc-probe] enabled nodes" << h.document().nodeCount()
            << "samples" << h.document().sampleCount()
            << "every_sample" << h.everySample()
            << "synth" << synth;
}

void runDocProbeSynth(TabletCanvasItem *canvas)
{
    auto stroke = [canvas](qreal x0, qreal y0, int n) {
        canvas->ingestPoint(QEvent::TabletPress, QPointF(x0, y0), 0.6);
        for (int i = 1; i < n; ++i) {
            canvas->ingestPoint(QEvent::TabletMove, QPointF(x0 + i * 12.0, y0 + i * 4.0), 0.55);
        }
        canvas->ingestPoint(QEvent::TabletRelease, QPointF(x0 + n * 12.0, y0 + n * 4.0), 0.0);
    };
    for (int s = 0; s < 40; ++s)
        stroke(200.0 + s * 30.0, 400.0 + (s % 8) * 80.0, 20);
}

} // namespace

TabletCanvasItem::TabletCanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_sync(new StrokeSync(this))
{
    m_paintsInk = qgetenv("RM_INK_MODE").trimmed().toLower() != "pool";
    m_beacons = envFlag("RM_INK_BEACON", false);
    armDocProbeFromEnv();

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
    if (envFlag("RM_DOC_PROBE_SYNTH", false)) {
        QTimer::singleShot(400, this, [this]() {
            qInfo() << "[doc-probe] synth ingest start";
            runDocProbeSynth(this);
            qInfo() << "[doc-probe] synth ingest done";
            QTimer::singleShot(250, qApp, &QCoreApplication::quit);
        });
    }
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
    IngestChannels ch;
    ch.pressure = pressure;
    ingestPoint(type, pos, ch);
}

void TabletCanvasItem::ingestPoint(QEvent::Type type, const QPointF &pos, const IngestChannels &ch)
{
    EpaperBridge::instance()->traceArrival();

    const qreal p = qBound<qreal>(0.0, ch.pressure, 1.0);
    IngestChannels bounded = ch;
    bounded.pressure = p;
    const QPointF canvasPos = mapInputToCanvas(pos);
    m_lastPoint = canvasPos;
    m_lastRaw = pos;

    // @implements [SRS-EP-13] hit-test probe on ingest, not in paint()
    if (g_docProbe) {
        const bool press = (type == QEvent::TabletPress || type == QEvent::MouseButtonPress);
        epaper::latencyprobe::harness().onIngest(float(canvasPos.x()), float(canvasPos.y()), press);
    }

    // Pen on ToolChip — not ink; arm via tile hit-test (pen-on-chip fallback).
    if (pointInToolChip(canvasPos)
        && (type == QEvent::TabletPress || type == QEvent::MouseButtonPress)) {
        tryArmToolAtCanvasPos(canvasPos);
        return;
    }

    switch (type) {
    case QEvent::TabletPress:
    case QEvent::MouseButtonPress:
        if (m_toolMode == QLatin1String("selection"))
            beginSelectionGesture(canvasPos);
        else
            beginStroke(canvasPos, bounded);
        break;
    case QEvent::TabletMove:
    case QEvent::MouseMove:
        // @implements [SRS-EP-10] mid-stroke tool switch does not change the latch
        // @implements [SRS-EP-04] selection mode never inks (STORY-EP-007)
        // @implements [SRS-EP-07] Selection armed → no stroke, no Ink node
        if (m_strokeActive)
            appendPoint(canvasPos, bounded);
        else if (m_selectionGesture)
            updateSelectionGesture(canvasPos);
        break;
    case QEvent::TabletRelease:
    case QEvent::MouseButtonRelease:
        if (m_strokeActive)
            endStroke();
        else if (m_selectionGesture)
            endSelectionGesture();
        break;
    default:
        break;
    }
}

int TabletCanvasItem::documentInkCount() const
{
    return m_document.inkCount();
}

std::string TabletCanvasItem::ingestDumpText() const
{
    using epaper::latencyprobe::nsToUs;
    using epaper::latencyprobe::summarizeNs;
    const auto p = summarizeNs(m_ingestNs);
    char buf[512];
    std::snprintf(buf, sizeof(buf),
                  "[doc-ingest] ink_nodes=%d applied=%d rejected=%d n=%d p50=%lldns p95=%lldns "
                  "p99=%lldns (p95=%lldus)\n",
                  m_document.inkCount(), m_ingestApplied, m_ingestRejected, p.n,
                  static_cast<long long>(p.p50Ns), static_cast<long long>(p.p95Ns),
                  static_cast<long long>(p.p99Ns), static_cast<long long>(nsToUs(p.p95Ns)));
    return std::string(buf);
}

void TabletCanvasItem::paint(QPainter *painter)
{
    if (!m_paintsInk)
        return;

    // SRS-EP-13: do not hit-test or time the stub document here.
    ensureImage();
    m_paintCount.fetchAndAddRelaxed(1);
    painter->drawImage(0, 0, m_image);
    // Selection chrome is composited (not baked) so move ghosts don't full-clear ink.
    paintSelectionChrome(painter);
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

TabletCanvasItem::Point TabletCanvasItem::makePoint(const QPointF &canvasPos, const IngestChannels &ch) const
{
    Point pt;
    pt.pos = canvasPos;
    pt.pressure = ch.pressure;
    pt.raw = m_lastRaw;
    pt.hasTilt = ch.hasTilt;
    pt.tiltX = ch.tiltX;
    pt.tiltY = ch.tiltY;
    pt.hasDistance = ch.hasDistance;
    pt.distance = ch.distance;
    pt.hasTimestamp = ch.hasTimestamp;
    pt.timestamp = ch.timestamp;
    pt.hasRotation = ch.hasRotation;
    pt.rotation = ch.rotation;
    pt.hasTangential = ch.hasTangential;
    pt.tangential = ch.tangential;
    return pt;
}

void TabletCanvasItem::beginStroke(const QPointF &canvasPos, const IngestChannels &ch)
{
    // Cancel pending settle follow-up so it cannot white-clear mid-stroke.
    ++m_settleFollowUpToken;
    // @implements [SRS-EP-07] stroke is one undo gesture from pen-down
    m_document.beginGesture();
    // @implements [SRS-EP-10] latch armed tool at pen-down
    m_strokeArmedTool = m_toolMode;

    m_current.clear();
    m_activeStrokeId = QStringLiteral("s-%1").arg(++m_strokeSeq);
    m_activeWorldStrokeWidth = worldStrokeWidth(ch.pressure);
    m_current.append(makePoint(canvasPos, ch));
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

void TabletCanvasItem::appendPoint(const QPointF &canvasPos, const IngestChannels &ch)
{
    if (!m_strokeActive || m_current.isEmpty()) {
        beginStroke(canvasPos, ch);
        return;
    }

    Point next = makePoint(canvasPos, ch);
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

    // Pixels first — ingestion must not sit between a sample and its pixel (SRS-EP-07 / EP-13).
    flushPending();

    if (m_current.size() >= 2)
        ingestCurrentStroke();
    // Safety net: commitOp already ended a successful gesture; this aborts an empty one.
    m_document.abortGesture();

    m_current.clear();
    m_hasEmitted = false;
    m_strokeActive = false;

    // Rasterize after enclose so group-local ink is visible (not in paint()).
    if (m_needEncloseRasterize) {
        m_needEncloseRasterize = false;
        rasterizeVectors(true);
    }

    // Run any viewport/snapshot refresh that was deferred while the pen was down.
    if (m_rasterizeDeferredSharp) {
        m_rasterizeDeferredSharp = false;
        scheduleVectorRasterize(true);
    } else if (m_rasterizePending) {
        m_rasterizePending = false;
        scheduleVectorRasterize(false);
    }

    // Status text is refreshed between strokes only: during a stroke it would
    // add a second damage region per flush.
    m_debugInfo = QStringLiteral("(%1,%2) sz=%3x%4 flush=%5 paint=%6 ink=%7")
                      .arg(int(m_lastPoint.x()))
                      .arg(int(m_lastPoint.y()))
                      .arg(int(width()))
                      .arg(int(height()))
                      .arg(m_flushCount)
                      .arg(m_paintCount.loadRelaxed())
                      .arg(m_document.inkCount());
    emit debugChanged();
}

/** @implements [SRS-EP-07] finished stroke → append_ink at pen-up */
/** @implements [SRS-EP-10] Ink-box latch → enclose; Pen → ordinary ink (no enclose path) */
/** @implements [SRS-EP-15] [ink] / [enclose] log sources after ingest */
void TabletCanvasItem::ingestCurrentStroke()
{
    using namespace epaper::document;
    FinishedStroke stroke;
    stroke.id = m_activeStrokeId.toStdString();
    stroke.strokeWidthWorld = double(m_activeWorldStrokeWidth);
    stroke.samples.reserve(size_t(m_current.size()));
    for (int i = 0; i < m_current.size(); ++i) {
        const Point &pt = m_current.at(i);
        DigitizerSample d;
        d.panelX = pt.pos.x();
        d.panelY = pt.pos.y();
        d.pressure = pt.pressure;
        if (pt.hasTilt) {
            d.tiltX = pt.tiltX;
            d.tiltY = pt.tiltY;
        }
        if (pt.hasDistance)
            d.distance = pt.distance;
        if (pt.hasTimestamp)
            d.timestamp = pt.timestamp;
        d.t = double(i);
        if (pt.hasRotation)
            d.extras.emplace("rotation", JsonValue::number(double(pt.rotation)));
        if (pt.hasTangential)
            d.extras.emplace("tangentialPressure", JsonValue::number(double(pt.tangential)));
        stroke.samples.push_back(std::move(d));
    }
    const PanelToWorld map = [this](double px, double py, double *wx, double *wy) {
        const QPointF w = panelToWorld(QPointF(px, py));
        *wx = w.x();
        *wy = w.y();
    };

    // Pen (and any non-ink_box latch): ordinary ingest + draw-into membership.
    // Never recognize_enclose on this path (SRS-EP-10).
    if (m_strokeArmedTool != QLatin1String("ink_box")) {
        const IngestTiming t = ingestFinishedStrokeTimed(m_document, stroke, map);
        m_ingestNs.push_back(t.ns);
        if (t.result.applied) {
            ++m_ingestApplied;
            tryDrawIntoMembership(m_document, stroke.id);
            qInfo().noquote() << QString::fromStdString(epaper::debuglog::formatInkLog(stroke.id));
        } else {
            ++m_ingestRejected;
        }
        return;
    }

    const EncloseIngestTiming t =
        ingestStrokeAtPenUp(m_document, stroke, map, StrokeArmedTool::InkBox);
    {
        std::string kindName = "Skipped";
        if (t.result.kind == EncloseKind::Created)
            kindName = "Created";
        else if (t.result.kind == EncloseKind::OrdinaryInk)
            kindName = "OrdinaryInk";
        // Skip too_few_samples / Skipped — no enclose evaluation worth a line.
        if (t.result.kind != EncloseKind::Skipped) {
            const std::string line = epaper::debuglog::formatEncloseLog(
                kindName, t.result.reason, t.result.smartGroupId, t.result.childIds);
            qInfo().noquote() << QString::fromStdString(line);
        }
    }
    m_ingestNs.push_back(t.ns);
    if (t.apply.applied)
        ++m_ingestApplied;
    else
        ++m_ingestRejected;
    if (t.result.kind == EncloseKind::Created)
        m_needEncloseRasterize = true;
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
    // Legacy PNG path ignored — paint stays on the local tree (SRS-EP-07 / ADR-0014 §2).
    if (type == QLatin1String("region_refresh")) {
        qInfo() << "[sync] ignoring region_refresh bitmap; paint stays on local tree";
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
            << "settle" << settle << "ink" << m_document.inkCount();
    scheduleVectorRasterize(settle);
}

void TabletCanvasItem::applyDocSnapshot(const QJsonObject &obj)
{
    Q_UNUSED(obj);
    // @implements [SRS-EP-07] 0 inbound peer pictures as a paint source
    // Full inbound classification / handshake is STORY-EP-020; this story must not
    // rasterize a peer snapshot over the local tree.
    if (!m_loggedRetiredSnapshot) {
        m_loggedRetiredSnapshot = true;
        qInfo() << "[sync] reject retired doc_snapshot; paint stays on local tree ink"
                << m_document.inkCount();
    }
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

QString TabletCanvasItem::toolModeAtChipPos(const QPointF &canvasPos) const
{
    if (!pointInToolChip(canvasPos))
        return {};
    const qreal tileW = m_toolChipRect.height();
    if (tileW <= 0.0)
        return {};
    const qreal relX = canvasPos.x() - m_toolChipRect.x();
    const int tile = qBound(0, int(relX / tileW), 2);
    static const char *const kModes[] = {"selection", "pen", "ink_box"};
    return QString::fromLatin1(kModes[tile]);
}

bool TabletCanvasItem::tryArmToolAtCanvasPos(const QPointF &canvasPos)
{
    const QString mode = toolModeAtChipPos(canvasPos);
    if (mode.isEmpty())
        return false;
    armTool(mode);
    return true;
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
        if (!m_selectedPickableId.isEmpty()) {
            m_selectedPickableId.clear();
            m_selectionChromeDirty = QRectF();
            update(); // clear chrome
        }
        return;
    }
    m_selectedPickableId = id;
    m_gesturePickableId = id;
    m_gestureStartWorld = world;
    m_gestureLastWorld = world;
    m_selectionGesture = true;
    m_selectionGhostClock.restart();
    m_selectionChromeDirty = pickablePanelRect(id).adjusted(-8, -8, 8, 8);
    update(m_selectionChromeDirty.toAlignedRect());
}

void TabletCanvasItem::updateSelectionGesture(const QPointF &canvasPos)
{
    if (!m_selectionGesture)
        return;
    m_gestureLastWorld = panelToWorld(canvasPos);
    // Soft region refresh ≥20 Hz — chrome only, not full vector redraw (SRS-EP-04 ghost).
    if (m_selectionGhostClock.isValid()
        && m_selectionGhostClock.elapsed() < kSelectionGhostMinIntervalMs)
        return;
    m_selectionGhostClock.restart();

    const double dx = m_gestureLastWorld.x() - m_gestureStartWorld.x();
    const double dy = m_gestureLastWorld.y() - m_gestureStartWorld.y();
    const QRectF next = pickablePanelRect(m_gesturePickableId, dx, dy).adjusted(-8, -8, 8, 8);
    const QRectF dirty = m_selectionChromeDirty.isNull() ? next : m_selectionChromeDirty.united(next);
    m_selectionChromeDirty = next;
    update(dirty.toAlignedRect());
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
        // Keep selection chrome visible after tap-select.
        m_selectedPickableId = m_gesturePickableId;
    } else {
        obj.insert(QStringLiteral("action"), QStringLiteral("move"));
        obj.insert(QStringLiteral("delta"), QJsonObject{{"dx", dx}, {"dy", dy}});
        // Keep selected; chrome stays until next snapshot (authority).
        m_selectedPickableId = m_gesturePickableId;
    }
    syncToolIntent(obj);
    m_gesturePickableId.clear();
    m_selectionChromeDirty = pickablePanelRect(m_selectedPickableId).adjusted(-8, -8, 8, 8);
    update(m_selectionChromeDirty.toAlignedRect());
}

QRectF TabletCanvasItem::pickablePanelRect(const QString &id, double dxWorld, double dyWorld) const
{
    if (id.isEmpty() || !m_drawingRegion.valid)
        return {};
    for (const QJsonValue &v : m_pickables) {
        const QJsonObject p = v.toObject();
        if (p.value(QStringLiteral("id")).toString() != id)
            continue;
        const QJsonObject b = p.value(QStringLiteral("bounds")).toObject();
        const double minX = b.value(QStringLiteral("minX")).toDouble() + dxWorld;
        const double minY = b.value(QStringLiteral("minY")).toDouble() + dyWorld;
        const double maxX = b.value(QStringLiteral("maxX")).toDouble() + dxWorld;
        const double maxY = b.value(QStringLiteral("maxY")).toDouble() + dyWorld;
        const QPointF tl = worldToPanel(minX, minY);
        const QPointF br = worldToPanel(maxX, maxY);
        return QRectF(tl, br).normalized();
    }
    return {};
}

void TabletCanvasItem::paintSelectionChrome(QPainter *painter) const
{
    if (m_toolMode != QLatin1String("selection"))
        return;
    if (m_selectedPickableId.isEmpty() && !m_selectionGesture)
        return;

    const QString id = m_selectionGesture ? m_gesturePickableId : m_selectedPickableId;
    if (id.isEmpty())
        return;

    double dx = 0;
    double dy = 0;
    if (m_selectionGesture) {
        dx = m_gestureLastWorld.x() - m_gestureStartWorld.x();
        dy = m_gestureLastWorld.y() - m_gestureStartWorld.y();
    }
    const QRectF r = pickablePanelRect(id, dx, dy);
    if (r.isEmpty())
        return;

    painter->save();
    QPen pen(Qt::black);
    pen.setWidthF(2.0);
    if (m_selectionGesture && (qAbs(dx) > 0.5 || qAbs(dy) > 0.5))
        pen.setStyle(Qt::DashLine);
    else
        pen.setStyle(Qt::SolidLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(r);

    // Corner + edge anchors (8 handles) — feedback only; resize stays Infini for now.
    const qreal h = 10.0;
    const QPointF pts[8] = {
        r.topLeft(),
        QPointF(r.center().x(), r.top()),
        r.topRight(),
        QPointF(r.right(), r.center().y()),
        r.bottomRight(),
        QPointF(r.center().x(), r.bottom()),
        r.bottomLeft(),
        QPointF(r.left(), r.center().y()),
    };
    painter->setBrush(Qt::white);
    pen.setStyle(Qt::SolidLine);
    painter->setPen(pen);
    for (const QPointF &c : pts)
        painter->drawRect(QRectF(c.x() - h * 0.5, c.y() - h * 0.5, h, h));
    painter->restore();
}

void TabletCanvasItem::syncToolIntent(const QJsonObject &obj)
{
    m_sync->sendLine(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void TabletCanvasItem::scheduleVectorRasterize(bool sharp)
{
    // Never full-redraw while the pen is down — white clear would erase live ink
    // and stall the GUI thread so later strokes miss the panel.
    if (m_strokeActive) {
        if (sharp)
            m_rasterizeDeferredSharp = true;
        else if (!m_rasterizeDeferredSharp)
            m_rasterizePending = true; // soft after stroke ends
        return;
    }

    if (sharp)
        m_rasterizeSharp = true;
    if (m_rasterizePending && !sharp) {
        // Already scheduled; keep pending soft refresh (region already updated).
        return;
    }
    if (sharp) {
        // Settle: one sharp redraw now. Optional light follow-up cancels if another
        // settle/stroke arrives — no per-frame swapPen (that starved ink).
        m_rasterizePending = false;
        m_rasterizeDeferredSharp = false;
        rasterizeVectors(true);
        m_rasterizeSharp = false;
        const int token = ++m_settleFollowUpToken;
        QTimer::singleShot(int(kSettleFollowUpMs), this, [this, token]() {
            if (token != m_settleFollowUpToken || m_strokeActive)
                return;
            rasterizeVectors(true);
        });
        return;
    }
    m_rasterizePending = true;
    QTimer::singleShot(int(kRefreshMinIntervalMs), this, [this]() {
        if (!m_rasterizePending || m_strokeActive)
            return;
        m_rasterizePending = false;
        const bool doSharp = m_rasterizeSharp || m_rasterizeDeferredSharp;
        m_rasterizeSharp = false;
        m_rasterizeDeferredSharp = false;
        rasterizeVectors(doSharp);
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

void TabletCanvasItem::drawDocNode(QPainter &p, const epaper::document::DocNode &node,
                                   const epaper::document::DocNode *smartParent)
{
    using epaper::document::NodeKind;
    using epaper::document::PrimitiveKind;
    using epaper::document::inkSamplesCentroid;
    using epaper::document::smartLocalToWorld;
    if (node.kind != NodeKind::Ink && node.kind != NodeKind::Primitive)
        return;

    const qreal worldSw = node.style.strokeWidth;
    const double rw = m_drawingRegion.valid ? (m_drawingRegion.maxX - m_drawingRegion.minX)
                                            : qMax(1.0, double(width()));
    const double sPanel = width() / qMax(1e-6, rw);
    const qreal lineW = qMax<qreal>(1.0, worldSw * sPanel);

    QPen pen(Qt::black);
    pen.setWidthF(lineW);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    if (node.kind == NodeKind::Ink) {
        if (int(node.samples.size()) < 2)
            return;
        const std::string role = node.role ? *node.role : std::string("content");
        epaper::document::Vec2 centroid{};
        const epaper::document::Vec2 *centroidPtr = nullptr;
        if (smartParent && role == "content" && smartParent->inkScaleMode == "fixedInk") {
            centroid = inkSamplesCentroid(node.samples);
            centroidPtr = &centroid;
        }
        auto toPanel = [&](double x, double y) {
            if (smartParent) {
                const auto w = smartLocalToWorld(x, y, *smartParent, role, node.layoutOffset, centroidPtr);
                return worldToPanel(w.x, w.y);
            }
            return worldToPanel(x, y);
        };
        QPainterPath path;
        path.moveTo(toPanel(node.samples[0].x, node.samples[0].y));
        for (size_t i = 1; i < node.samples.size(); ++i)
            path.lineTo(toPanel(node.samples[i].x, node.samples[i].y));
        p.drawPath(path);
        return;
    }

    if (node.geomKind == PrimitiveKind::Line) {
        p.drawLine(worldToPanel(node.x1, node.y1), worldToPanel(node.x2, node.y2));
        return;
    }
    if (node.geomKind == PrimitiveKind::Rect) {
        const QPointF tl = worldToPanel(node.gx, node.gy);
        const QPointF br = worldToPanel(node.gx + node.gw, node.gy + node.gh);
        p.drawRect(QRectF(tl, br).normalized());
        return;
    }
    if (node.geomKind == PrimitiveKind::Ellipse) {
        const QPointF c = worldToPanel(node.cx, node.cy);
        const QPointF e = worldToPanel(node.cx + node.rx, node.cy + node.ry);
        p.drawEllipse(c, qAbs(e.x() - c.x()), qAbs(e.y() - c.y()));
    }
}

void TabletCanvasItem::drawTree(QPainter &p, const std::vector<epaper::document::DocNode> &nodes,
                                const epaper::document::DocNode *smartParent)
{
    using epaper::document::NodeKind;
    for (const auto &node : nodes) {
        if (node.kind == NodeKind::SmartGroup)
            drawTree(p, node.children, &node);
        else if (node.kind == NodeKind::Frame || node.kind == NodeKind::Group)
            drawTree(p, node.children, nullptr);
        else
            drawDocNode(p, node, smartParent);
    }
}

void TabletCanvasItem::rasterizeVectors(bool sharp)
{
    if (!m_paintsInk)
        return;
    ensureImage();
    if (m_image.isNull())
        return;

    QPainter p(&m_image);
    // Full white clear + local-tree redraw. Never an inbound peer picture (SRS-EP-07).
    p.fillRect(m_image.rect(), Qt::white);
    p.setRenderHint(QPainter::Antialiasing, sharp);
    drawTree(p, m_document.rootChildren, nullptr);
    p.end();

    stampStaticBeacon();
    m_refreshClock.restart();
    // Full-rect update. Pen-mode swap only when explicitly requested (RM_EP_SWAP) —
    // unconditional swap after every settle starved the stroke path on device.
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
    qInfo() << "[sync] vector rasterize ink" << m_document.inkCount() << "nodes"
            << m_document.nodeCount() << "sharp" << sharp << "seq" << m_viewportSeq;
}
