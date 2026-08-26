#include "tabletcanvasitem.h"
#include "debug/ui_stall.hpp"
#include "regionsync/strokesync.h"
#include "epaperbridge.h"
#include "debug/latency_probe.hpp"
#include "document/connector_warp.hpp"
#include "document/recognizer_dispatch.hpp"
#include "document/recognize_enclose.hpp"
#include "document/membership.hpp"
#include "document/manipulate.hpp"
#include "debug/debug_log_format.hpp"
#include "primary_toolbar.hpp"
#include "ingest_origin_guard.hpp"

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
#include <QDebug>
#include <QSizeF>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

bool envFlag(const char *name, bool fallback)
{
    if (!qEnvironmentVariableIsSet(name))
        return fallback;
    const QByteArray v = qgetenv(name).trimmed().toLower();
    return !(v == "0" || v == "false" || v == "off" || v == "no");
}

static QRectF panelToQ(const epaper::follow::PanelRect &r)
{
    return QRectF(r.x, r.y, r.w, r.h);
}

/** Context toolbar under the box — south of the bottom handle (28 du visual). */

/** Fixed screen slots for the render-path beacons (EXP-0001 Round 22). */
constexpr int kStaticBeaconX = 40;
constexpr int kStaticBeaconY = 60;
constexpr int kStaticBeaconSize = 120;
constexpr int kFlushBeaconX = 200;
constexpr int kFlushBeaconY = 60;
constexpr int kFlushBeaconSize = 60;

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

/** Wire shape for a drawing region, shared by the viewport and follow-cache paths. */
epaper::handtouch::WorldAabb aabbFromJson(const QJsonObject &dr)
{
    epaper::handtouch::WorldAabb a;
    a.minX = dr.value(QStringLiteral("minX")).toDouble();
    a.minY = dr.value(QStringLiteral("minY")).toDouble();
    a.maxX = dr.value(QStringLiteral("maxX")).toDouble();
    a.maxY = dr.value(QStringLiteral("maxY")).toDouble();
    return a;
}

} // namespace

/**
 * =================================================================================================
 * Construction and Qt item lifecycle
 *
 * Ctor wires StrokeSync, CanvasSession NOTIFY → rasterize/chrome, and ToolChip layout.
 * geometryChange keeps panel size and Pen-mode region in sync with the QQuickItem.
 * =================================================================================================
 */

/** Owns session, one-way sync, and session→rasterize connections. */
TabletCanvasItem::TabletCanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_sync(new StrokeSync(this))
    , m_oneWay(m_session.document)
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
    connect(m_sync, &StrokeSync::socketConnected, this, [this]() {
        epaper::UiStallSection stall("onLinkUp-hello");
        m_oneWay.onLinkUp();
        m_session.follow.onReconnect();
        emit followChanged();
        flushOneWayWire();
    });
    connect(m_sync, &StrokeSync::socketDisconnected, this, [this]() {
        m_oneWay.onLinkDown();
        // @implements [SRS-EP-49] disconnect forces follow none
        m_session.follow.onDisconnect();
        m_session.setFollowDirection(QStringLiteral("none"));
        emit followChanged();
    });
    auto *helloRetry = new QTimer(this);
    helloRetry->setInterval(5000);
    connect(helloRetry, &QTimer::timeout, this, [this]() {
        if (!m_sync->isConnected() || m_oneWay.epochLive())
            return;
        m_oneWay.retransmitHelloIfWaiting();
        flushOneWayWire();
    });
    helloRetry->start();
    m_sync->connectToMac();
    updateToolChipRect();

    connect(&m_session, &CanvasSession::cameraChanged, this, [this]() {
        scheduleVectorRasterize(false);
    });
    connect(&m_session, &CanvasSession::documentMutated, this, [this]() {
        scheduleVectorRasterize(true);
    });
    connect(&m_session, &CanvasSession::exclusiveToolChanged, this, [this]() {
        const bool hadHighlight = !m_highlightInkIds.empty();
        clearMembershipHighlight();
        if (hadHighlight && !m_stroke.active)
            scheduleVectorRasterize(true);
        emit toolModeChanged();
        m_debugInfo = QStringLiteral("tool=%1").arg(toolMode());
        emit debugChanged();
    });
    connect(&m_session, &CanvasSession::followChanged, this, &TabletCanvasItem::followChanged);
    connect(&m_session, &CanvasSession::recogChanged, this, &TabletCanvasItem::recogChanged);
}

/** On resize: sync frame panel size, ToolChip layout, and Pen-mode region. */
void TabletCanvasItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    syncFramePanelSize();
    qInfo() << "[ink] geometryChange" << oldGeometry << "->" << newGeometry;
    if (newGeometry.size() != oldGeometry.size()
        && newGeometry.width() > 1.0 && newGeometry.height() > 1.0) {
        ensureImage();
        updateToolChipRect();
        EpaperBridge::instance()->attachPenModeRegion(this);
        update();
    }
}

/** First layout pass + optional RM_DOC_PROBE synth ingest. */
void TabletCanvasItem::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    syncFramePanelSize();
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

/** Logs first few scene-graph updates for ink bring-up. */
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


/**
 * =================================================================================================
 * Canvas frame — orientation, camera region, transforms
 *
 * All panel↔world math goes through m_session.frame. applyFrameIntent is the only place
 * FrameIntent becomes ToolChip re-layout (and related Qt effects).
 * =================================================================================================
 */

/** Digitizer raw → panel via frame orientation. */
TabletCanvasItem::PanelPt TabletCanvasItem::mapInputToCanvas(RawPt raw) const
{
    // Shared with the raw input filter, which maps before Qt delivers the point.
    qreal w = width();
    qreal h = height();
    if (w < 2.0 && window())
        w = window()->width();
    if (h < 2.0 && window())
        h = window()->height();
    return epaper::input::mapPanel(QPointF(raw.x, raw.y), w, h);
}

/** Panel pixel → document world through the current camera. */
TabletCanvasItem::WorldPt TabletCanvasItem::panelToWorld(const PanelPt &panel) const
{
    syncFramePanelSize();
    return m_session.frame.panelToWorld({panel.x(), panel.y()});
}

/** Panel height used by StrokeCapture sample spacing. */
qreal TabletCanvasItem::ingestPanelHeight() const
{
    qreal h = height();
    if (h < 2.0 && window())
        h = window()->height();
    return qMax<qreal>(1.0, h);
}

/** Push QQuickItem size into CanvasFrame before transforms. */
void TabletCanvasItem::syncFramePanelSize() const
{
    qreal w = width();
    qreal h = height();
    if (w < 2.0 && window())
        w = window()->width();
    if (h < 2.0 && window())
        h = window()->height();
    m_session.frame.setPanelSize(double(w), double(h));
}

/** Camera/orientation intent → ToolChip + session cameraChanged (Tool listens). */
void TabletCanvasItem::applyFrameIntent(epaper::canvasframe::FrameIntent intent)
{
    using epaper::canvasframe::FrameIntent;
    using epaper::canvasframe::has;
    if (has(intent, FrameIntent::OrientationChanged))
        updateToolChipRect();
    // Emit for Tool chrome; callers may still schedule settle-sharp rasterize.
    if (has(intent, FrameIntent::CameraChanged) || has(intent, FrameIntent::OrientationChanged))
        m_session.noteCameraChanged();
}

/** Panel → normalized UV inside the sync frame. */
TabletCanvasItem::FrameUv TabletCanvasItem::panelToFrameUv(const PanelPt &panel) const
{
    syncFramePanelSize();
    return m_session.frame.panelToFrameUv({panel.x(), panel.y()});
}

/** UV → panel (follow / two-finger helpers). */
TabletCanvasItem::PanelPt TabletCanvasItem::frameUvToPanel(FrameUv uv) const
{
    syncFramePanelSize();
    const auto p = m_session.frame.frameUvToPanel(uv);
    return PanelPt(p.x, p.y);
}

/** Document world → panel pixel. */
TabletCanvasItem::PanelPt TabletCanvasItem::worldToPanel(double wx, double wy) const
{
    syncFramePanelSize();
    const auto p = m_session.frame.worldToPanel(wx, wy);
    return PanelPt(p.x, p.y);
}

/** World units per panel pixel (stroke width, LOD). */
double TabletCanvasItem::panelScale() const
{
    syncFramePanelSize();
    return m_session.frame.panelScale();
}

/** SmartBounds AABB → panel QRectF. */
QRectF TabletCanvasItem::worldBoundsToPanel(const epaper::document::SmartBounds &wb) const
{
    syncFramePanelSize();
    epaper::canvasframe::PanelPt tl;
    epaper::canvasframe::PanelPt br;
    m_session.frame.worldBoundsToPanel(wb.x, wb.y, wb.width, wb.height, &tl, &br);
    return QRectF(QPointF(tl.x, tl.y), QPointF(br.x, br.y)).normalized();
}

/** True when LOD may refuse manip. */
bool TabletCanvasItem::viewportZoomedOut() const
{
    syncFramePanelSize();
    return m_session.frame.viewportZoomedOut();
}

/** Whether a world AABB is large enough on panel for manip. */
bool TabletCanvasItem::lodOkPanel(const epaper::document::SmartBounds &wb) const
{
    // @implements [SRS-EP-11] LOD only when zoomed out; scale ≥ 1.0 always manipulable
    syncFramePanelSize();
    return m_session.frame.lodOkPanel(wb.x, wb.y, wb.width, wb.height);
}


/**
 * =================================================================================================
 * Panel raster surface
 *
 * Live ink stamps into m_image; paint() blits it and, during live manip, punches the
 * origin hole from CanvasSession::liveManipOrigin. flushPending coalesces dirty rects to the
 * e-ink path.
 * =================================================================================================
 */

/** Blit ink buffer; white-hole punch from session liveManipOrigin during live manip. */
void TabletCanvasItem::paint(QPainter *painter)
{
    if (!m_paintsInk)
        return;

    // SRS-EP-13: do not hit-test or time the stub document here.
    ensureImage();
    m_paintCount.fetchAndAddRelaxed(1);
    painter->drawImage(0, 0, m_image);
    const auto &origin = m_session.liveManipOrigin();
    if (origin && !origin->panelRect.isEmpty()) {
        // Punch only the original box — not origin∪live (that wipes a vertical strip).
        painter->fillRect(origin->panelRect, Qt::white);
        // Connector origin hole: thick white stroke along the rest-pose polyline.
        // [SRS-EP-18] [CHL-0018]
        if (!origin->connStrokes.isEmpty()) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->setBrush(Qt::NoBrush);
            for (const OriginConnStroke &st : origin->connStrokes) {
                if (st.panel.size() < 2)
                    continue;
                QPen erase(Qt::white);
                erase.setWidthF(st.width + 16.0);
                erase.setCapStyle(Qt::RoundCap);
                erase.setJoinStyle(Qt::RoundJoin);
                painter->setPen(erase);
                painter->drawPolyline(st.panel.constData(), st.panel.size());
            }
            painter->restore();
        }
    }
}

/** Allocate/clear the panel QImage to item size. */
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

/** Draw one ink segment into m_image. */
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

/** Paint or emit segmentDrawn (pool mode); dirty + flush. */
void TabletCanvasItem::emitSegment(const Point &from, const Point &to)
{
    // ADR-0012: live ink uses world width × current panel scale (matches zoomed vectors).
    const qreal worldW = worldStrokeWidth(from.pressure);
    const qreal lineW = qMax<qreal>(1.0, worldW * panelScale());

    if (m_paintsInk)
        paintSegment(from, to, lineW);
    else
        emit segmentDrawn(from.pos.x(), from.pos.y(), to.pos.x(), to.pos.y(), lineW);

    const qreal pad = lineW * 0.5 + 8.0;
    const QRectF rf = QRectF(from.pos, to.pos).normalized().adjusted(-pad, -pad, pad, pad);
    m_pendingDirty = m_pendingDirty.isNull() ? rf : m_pendingDirty.united(rf);
}

/** Coalesce dirty rects and swapBuffers / beacon. */
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

/** EXP beacon: fixed rect for render-path timing. */
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

/** EXP beacon: marks a flush cycle. */
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


/**
 * =================================================================================================
 * Device document and undo history
 *
 * Undo/redo mutate m_session.document then noteDocumentMutated (Tablet rasterize +
 * Tool prune/chrome via documentMutated). notifyHistory is also on the Surface API
 * so Tool can emit after its own commits.
 * =================================================================================================
 */

/** QML undo control → applyHistoryRestore(true). */
void TabletCanvasItem::requestUndo()
{
    applyHistoryRestore(true);
}

/** QML redo control → applyHistoryRestore(false). */
void TabletCanvasItem::requestRedo()
{
    applyHistoryRestore(false);
}

/** Undo/redo op, membership clear, documentMutated, flush wire. */
void TabletCanvasItem::applyHistoryRestore(bool isUndo)
{
    using namespace epaper::document;
    const UndoResult r = isUndo ? m_session.document.undo() : m_session.document.redo();
    (void)r;
    clearMembershipHighlight();
    notifyHistory();
    m_session.noteDocumentMutated();
    flushOneWayWire();
}


/**
 * =================================================================================================
 * Pen stroke capture and ingest
 *
 * Digitizer samples enter via ingestPoint / ingestMappedTablet (also Surface ingestPen).
 * StrokeCapture owns the stroke. Selection/handle presses are decided on Tool before
 * ingestPen; this path is ink only.
 * =================================================================================================
 */

/** Q_INVOKABLE raw point with pressure only. */
void TabletCanvasItem::ingestPoint(QEvent::Type type, const QPointF &pos, qreal pressure)
{
    IngestChannels ch;
    ch.pressure = pressure;
    ingestPoint(type, pos, ch);
}

/** Raw point + full PenSample channels. */
void TabletCanvasItem::ingestPoint(QEvent::Type type, const QPointF &pos, const IngestChannels &ch)
{
    EpaperBridge::instance()->traceArrival();

    const qreal p = qBound<qreal>(0.0, ch.pressure, 1.0);
    IngestChannels bounded = ch;
    bounded.pressure = p;
    // Q_INVOKABLE, so the wire type stays QPointF; it is raw and named so here.
    const RawPt raw{pos.x(), pos.y()};
    const PanelPt canvasPos = mapInputToCanvas(raw);
    m_stroke.setPanelHeight(double(ingestPanelHeight()));
    m_stroke.noteContact(canvasPos.x(), canvasPos.y(), raw.x, raw.y);

    const bool isPress = (type == QEvent::TabletPress || type == QEvent::MouseButtonPress);
    const bool isMove = (type == QEvent::TabletMove || type == QEvent::MouseMove);
    const bool isRelease = (type == QEvent::TabletRelease || type == QEvent::MouseButtonRelease);
    const bool stale = m_stroke.sampleStale(canvasPos.x(), canvasPos.y(), raw.x, raw.y);
    // @fix [STORY-EP-033] reject origin/stale first sample on pen-down
    const auto guard = m_stroke.guardContact(isPress, isMove, isRelease, stale);
    if (guard == epaper::ingest::OriginGuardAction::Discard
        || guard == epaper::ingest::OriginGuardAction::DropContact) {
        return;
    }
    const bool treatAsPress =
        isPress || guard == epaper::ingest::OriginGuardAction::PromoteToPress;

    // @implements [SRS-EP-13] hit-test probe on ingest, not in paint()
    if (g_docProbe) {
        epaper::latencyprobe::harness().onIngest(float(canvasPos.x()), float(canvasPos.y()),
                                                 treatAsPress);
    }

    if (treatAsPress) {
        applyContactPress(canvasPos, bounded);
        return;
    }

    switch (type) {
    case QEvent::TabletMove:
    case QEvent::MouseMove:
        // @implements [SRS-EP-10] mid-stroke tool switch does not change the latch
        if (m_stroke.active)
            appendPoint(canvasPos, bounded);
        break;
    case QEvent::TabletRelease:
    case QEvent::MouseButtonRelease:
        if (m_stroke.active)
            endStroke();
        break;
    default:
        break;
    }
}

/** Already-mapped panel sample: guard → press/move/release. */
void TabletCanvasItem::ingestMappedTablet(QEvent::Type type, const PanelPt &canvasPos,
                                          RawPt rawPos, const IngestChannels &ch)
{
    EpaperBridge::instance()->traceArrival();

    const qreal p = qBound<qreal>(0.0, ch.pressure, 1.0);
    IngestChannels bounded = ch;
    bounded.pressure = p;
    m_stroke.setPanelHeight(double(ingestPanelHeight()));
    m_stroke.noteContact(canvasPos.x(), canvasPos.y(), rawPos.x, rawPos.y);

    const bool isPress = (type == QEvent::TabletPress);
    const bool isMove = (type == QEvent::TabletMove);
    const bool isRelease = (type == QEvent::TabletRelease);
    const bool stale = m_stroke.sampleStale(canvasPos.x(), canvasPos.y(), rawPos.x, rawPos.y);
    const auto guard = m_stroke.guardContact(isPress, isMove, isRelease, stale);
    if (guard == epaper::ingest::OriginGuardAction::Discard
        || guard == epaper::ingest::OriginGuardAction::DropContact) {
        return;
    }
    const bool treatAsPress =
        isPress || guard == epaper::ingest::OriginGuardAction::PromoteToPress;

    if (g_docProbe) {
        epaper::latencyprobe::harness().onIngest(float(canvasPos.x()), float(canvasPos.y()),
                                                 treatAsPress);
    }

    if (treatAsPress) {
        applyContactPress(canvasPos, bounded);
        return;
    }

    switch (type) {
    case QEvent::TabletMove:
        if (m_stroke.active)
            appendPoint(canvasPos, bounded);
        break;
    case QEvent::TabletRelease:
        if (m_stroke.active)
            endStroke();
        break;
    default:
        break;
    }
}

/** Latency probe summary for doc-ingest harness. */
std::string TabletCanvasItem::ingestDumpText() const
{
    using epaper::latencyprobe::nsToUs;
    using epaper::latencyprobe::summarizeNs;
    const auto p = summarizeNs(m_ingestNs);
    char buf[512];
    std::snprintf(buf, sizeof(buf),
                  "[doc-ingest] ink_nodes=%d applied=%d rejected=%d n=%d p50=%lldns p95=%lldns "
                  "p99=%lldns (p95=%lldus)\n",
                  m_session.document.inkCount(), m_ingestApplied, m_ingestRejected, p.n,
                  static_cast<long long>(p.p50Ns), static_cast<long long>(p.p95Ns),
                  static_cast<long long>(p.p99Ns), static_cast<long long>(nsToUs(p.p95Ns)));
    return std::string(buf);
}

/** StrokeCapture sample → local Point for sync/paint. */
TabletCanvasItem::Point TabletCanvasItem::makePoint(const epaper::strokecapture::Sample &s) const
{
    Point pt;
    pt.pos = PanelPt(s.panelX, s.panelY);
    pt.pressure = s.ch.pressure;
    pt.raw = RawPt{qreal(s.rawX), qreal(s.rawY)};
    pt.hasTilt = s.ch.hasTilt;
    pt.tiltX = s.ch.tiltX;
    pt.tiltY = s.ch.tiltY;
    pt.hasDistance = s.ch.hasDistance;
    pt.distance = s.ch.distance;
    pt.hasTimestamp = s.ch.hasTimestamp;
    pt.timestamp = s.ch.timestamp;
    pt.hasRotation = s.ch.hasRotation;
    pt.rotation = s.ch.rotation;
    pt.hasTangential = s.ch.hasTangential;
    pt.tangential = s.ch.tangential;
    return pt;
}

/** PenSample → StrokeCapture channel POD. */
epaper::strokecapture::Channels TabletCanvasItem::toChannels(const IngestChannels &ch) const
{
    epaper::strokecapture::Channels c;
    c.pressure = ch.pressure;
    c.hasTilt = ch.hasTilt;
    c.tiltX = ch.tiltX;
    c.tiltY = ch.tiltY;
    c.hasDistance = ch.hasDistance;
    c.distance = ch.distance;
    c.hasTimestamp = ch.hasTimestamp;
    c.timestamp = ch.timestamp;
    c.hasRotation = ch.hasRotation;
    c.rotation = ch.rotation;
    c.hasTangential = ch.hasTangential;
    c.tangential = ch.tangential;
    return c;
}

/** StrokeCapture result → begin/append/end + latch/debug. */
void TabletCanvasItem::applyStrokeIntent(const epaper::strokecapture::StrokeResult &r)
{
    using epaper::strokecapture::StrokeIntent;
    using epaper::strokecapture::has;
    if (has(r.intent, StrokeIntent::CancelSettle))
        ++m_settleFollowUpToken;
    if (has(r.intent, StrokeIntent::BeginGesture))
        m_session.document.beginGesture();
    if (has(r.intent, StrokeIntent::LatchChip)) {
        m_session.chip.latchPenDown();
        const QString latch = QString::fromStdString(m_session.chip.dispatchTuple());
        if (m_lastStrokeLatch != latch) {
            m_lastStrokeLatch = latch;
            emit lastStrokeLatchChanged();
        }
    }
    if (has(r.intent, StrokeIntent::PreviewBegin))
        syncBegin();
    if (has(r.intent, StrokeIntent::PreviewPoint) && r.hasPreviewSample)
        syncPoint(makePoint(r.previewSample));
    if (has(r.intent, StrokeIntent::EmitSegment) && r.hasSegment)
        emitSegment(makePoint(r.segmentFrom), makePoint(r.segmentTo));
    if (has(r.intent, StrokeIntent::PreviewEnd))
        syncEnd();
    if (has(r.intent, StrokeIntent::StrokeCountChanged))
        emit strokeCountChanged();
    // Pixels before ingest — SRS-EP-07 / EP-13.
    if (has(r.intent, StrokeIntent::FlushInk))
        flushPending();
    if (has(r.intent, StrokeIntent::IngestReady) && r.hasFinished)
        ingestCurrentStroke(r.finished);
    if (has(r.intent, StrokeIntent::AbortGesture))
        m_session.document.abortGesture();
    if (has(r.intent, StrokeIntent::NotifyHistory))
        notifyHistory();
    if (has(r.intent, StrokeIntent::FlushWire))
        flushOneWayWire();
    if (has(r.intent, StrokeIntent::ChipPenUp))
        m_session.chip.penUp();
}

/** Pen-down for ink only — Tool already branched selection/handle away. */
void TabletCanvasItem::applyContactPress(const PanelPt &canvasPos, const IngestChannels &ch)
{
    beginStroke(canvasPos, ch);
}

/** Start StrokeCapture + syncBegin. */
void TabletCanvasItem::beginStroke(const PanelPt &canvasPos, const IngestChannels &ch)
{
    m_stroke.setPanelHeight(double(ingestPanelHeight()));
    m_pendingDirty = QRectF();
    m_flushClock.restart();
    applyStrokeIntent(m_stroke.begin(canvasPos.x(), canvasPos.y(), toChannels(ch)));
}

/** Mid-stroke sample + syncPoint. */
void TabletCanvasItem::appendPoint(const PanelPt &canvasPos, const IngestChannels &ch)
{
    if (!m_stroke.active || m_stroke.current.empty()) {
        beginStroke(canvasPos, ch);
        return;
    }
    const bool flushDue = m_flushClock.elapsed() >= kFlushIntervalMs;
    applyStrokeIntent(
        m_stroke.append(canvasPos.x(), canvasPos.y(), toChannels(ch), flushDue));
}

/** Finish stroke, ingestCurrentStroke, deferred rasterize. */
void TabletCanvasItem::endStroke()
{
    if (!m_stroke.active)
        return;

    epaper::UiStallSection stall("endStroke");
    applyStrokeIntent(m_stroke.end());

    // Rasterize after enclose so group-local ink is visible (not in paint()).
    if (m_needEncloseRasterize) {
        m_needEncloseRasterize = false;
        rasterizeVectors(true);
        m_rasterizeDeferredSharp = false;
        m_rasterizePending = false;
        ++m_settleFollowUpToken;
    } else if (m_rasterizeDeferredSharp) {
        m_rasterizeDeferredSharp = false;
        scheduleVectorRasterize(true);
    } else if (m_rasterizePending) {
        m_rasterizePending = false;
        scheduleVectorRasterize(false);
    }

    // Status text is refreshed between strokes only: during a stroke it would
    // add a second damage region per flush.
    m_debugInfo = QStringLiteral("(%1,%2) sz=%3x%4 flush=%5 paint=%6 ink=%7")
                      .arg(int(m_stroke.lastPanelX))
                      .arg(int(m_stroke.lastPanelY))
                      .arg(int(width()))
                      .arg(int(height()))
                      .arg(m_flushCount)
                      .arg(m_paintCount.loadRelaxed())
                      .arg(m_session.document.inkCount());
    emit debugChanged();
}

/** append_ink + recog dispatch; may schedule enclose rasterize. */
void TabletCanvasItem::ingestCurrentStroke(const epaper::document::FinishedStroke &stroke)
{
    using namespace epaper::document;
    if (stroke.samples.size() < 2)
        return;
    const PanelToWorld map = [this](double px, double py, double *wx, double *wy) {
        const WorldPt w = panelToWorld(PanelPt(px, py));
        *wx = w.x;
        *wy = w.y;
    };

    const std::string tool = m_session.chip.latchedTool;
    if (tool == "sel_rect" || tool == "sel_freeform")
        return;

    RecogLatch latch;
    latch.inkBox = m_session.chip.latchedInkBox;
    latch.connector = m_session.chip.latchedConnector;
    const RecogDispatchResult d = dispatchFinishedStroke(m_session.document, stroke, map, latch);
    m_ingestNs.push_back(d.ns);
    // Recognizers walk the whole document per stroke; name it when it hitches.
    if (d.ns > 100'000'000LL) {
        qInfo().noquote() << QStringLiteral("[perf] recog ms=%1 samples=%2 nodes=%3 ink=%4")
                                 .arg(d.ns / 1'000'000LL)
                                 .arg(stroke.samples.size())
                                 .arg(m_session.document.nodeCount())
                                 .arg(m_session.document.inkCount());
    }
    if (d.apply.applied)
        ++m_ingestApplied;
    else
        ++m_ingestRejected;
    qInfo().noquote() << QString::fromStdString(
        epaper::debuglog::formatRecogLog(d.outcomeName(), d.guard, d.encloseWhy));
    if (!d.connector.diag.empty()) {
        qInfo().noquote() << QString::fromStdString(
            epaper::debuglog::formatConnLog(d.connector.diag, d.connector.reason));
    }
    if (d.outcome == RecogOutcome::Enclose && d.enclose.kind == EncloseKind::Created) {
        const std::string line = epaper::debuglog::formatEncloseLog(
            "Created", d.enclose.reason, d.enclose.smartGroupId, d.enclose.childIds);
        qInfo().noquote() << QString::fromStdString(line);
        m_needEncloseRasterize = true;
        if (const DocNode *sg = m_session.document.find(d.enclose.smartGroupId)) {
            std::vector<std::string> ids;
            collectSmartGroupInkIds(*sg, false, &ids);
            beginRecogWidthBlink(ids);
        }
        clearMembershipHighlight();
    } else if (d.outcome == RecogOutcome::Membership) {
        bool highlightChanged = false;
        if (const DocNode *sg = m_session.document.find(d.membership.smartGroupId)) {
            std::vector<std::string> ids;
            collectSmartGroupInkIds(*sg, true, &ids);
            highlightChanged = setMembershipHighlight(ids);
        } else {
            highlightChanged = !m_highlightInkIds.empty();
            clearMembershipHighlight();
        }
        // Same parent already highlighted: live pixels are the new ink. A full
        // white-clear rasterize here is what lagged Pen every few draw-intos.
        if (highlightChanged)
            m_needEncloseRasterize = true;
    } else if (d.outcome == RecogOutcome::Connector) {
        // ovl.conn_blink — width pulse on connector body + both bound nodes (UI-EP-05).
        m_needEncloseRasterize = true;
        std::vector<std::string> ids = d.connector.bodyIds;
        if (const DocNode *sg = m_session.document.find(d.connector.fromId))
            collectSmartGroupInkIds(*sg, false, &ids);
        if (const DocNode *sg = m_session.document.find(d.connector.toId))
            collectSmartGroupInkIds(*sg, false, &ids);
        beginRecogWidthBlink(ids);
        clearMembershipHighlight();
    } else {
        // Failed empty enclose stays live ink. Do not white-clear the panel.
        const std::string &why = d.enclose.reason;
        const bool failedEncloseStayInk =
            why.find("not_primitive") != std::string::npos
            || why.find("too_small") != std::string::npos;
        if (failedEncloseStayInk) {
            clearMembershipHighlight();
            m_rasterizeDeferredSharp = false;
            m_rasterizePending = false;
            ++m_settleFollowUpToken;
        } else {
            if (!m_highlightInkIds.empty())
                m_needEncloseRasterize = true;
            clearMembershipHighlight();
        }
    }
}

/** Pressure → world stroke width. */
qreal TabletCanvasItem::worldStrokeWidth(qreal pressure) const
{
    return qreal(epaper::strokecapture::worldStrokeWidth(double(pressure),
                                                         epaper::strokecapture::StrokeCapture::kBaseWorldStroke));
}


/**
 * =================================================================================================
 * Rasterize scheduling and document tree paint
 *
 * scheduleVectorRasterize queues a soft/sharp redraw of vectors into m_image. Skipped
 * while a stroke is active so live ink is not wiped.
 * =================================================================================================
 */

/** Queue soft/sharp vector refresh (defer if pen down). */
void TabletCanvasItem::scheduleVectorRasterize(bool sharp)
{
    // Never full-redraw while the pen is down — white clear would erase live ink
    // and stall the GUI thread so later strokes miss the panel.
    if (m_stroke.active) {
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
            if (token != m_settleFollowUpToken || m_stroke.active)
                return;
            rasterizeVectors(true);
        });
        return;
    }
    m_rasterizePending = true;
    QTimer::singleShot(int(kRefreshMinIntervalMs), this, [this]() {
        if (!m_rasterizePending || m_stroke.active)
            return;
        m_rasterizePending = false;
        const bool doSharp = m_rasterizeSharp || m_rasterizeDeferredSharp;
        m_rasterizeSharp = false;
        m_rasterizeDeferredSharp = false;
        rasterizeVectors(doSharp);
    });
}

/** Clear image and paint the local doc tree ∩ camera. */
void TabletCanvasItem::rasterizeVectors(bool sharp)
{
    epaper::UiStallSection stall("rasterizeVectors");
    using epaper::document::refreshAllConnectorWarps;
    refreshAllConnectorWarps(m_session.document);
    if (!m_paintsInk)
        return;
    ensureImage();
    if (m_image.isNull())
        return;

    QPainter p(&m_image);
    // Full white clear + local-tree redraw. Never an inbound peer picture (SRS-EP-07).
    p.fillRect(m_image.rect(), Qt::white);
    p.setRenderHint(QPainter::Antialiasing, sharp);
    drawTree(p, m_session.document.rootChildren, nullptr);
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
    qInfo() << "[sync] vector rasterize ink" << m_session.document.inkCount() << "nodes"
            << m_session.document.nodeCount() << "sharp" << sharp << "seq" << m_viewportSeq;
}

/** Recursive paint of DocNode children. */
void TabletCanvasItem::drawTree(QPainter &p, const std::vector<epaper::document::DocNode> &nodes,
                                const epaper::document::DocNode *smartParent)
{
    using epaper::document::NodeKind;
    for (const auto &node : nodes) {
        if (node.kind == NodeKind::SmartGroup)
            drawTree(p, node.children, &node);
        else if (node.kind == NodeKind::Frame || node.kind == NodeKind::Group)
            drawTree(p, node.children, nullptr);
        else if (node.kind == NodeKind::Connector)
            drawWarpedConnector(p, node);
        else
            drawDocNode(p, node, smartParent);
    }
}

/** Paint one ink / SmartGroup / connector node. */
void TabletCanvasItem::drawDocNode(QPainter &p, const epaper::document::DocNode &node,
                                   const epaper::document::DocNode *smartParent)
{
    using epaper::document::NodeKind;
    using epaper::document::PrimitiveKind;
    using epaper::document::inkSamplesMin;
    using epaper::document::smartLocalToWorld;
    if (node.kind != NodeKind::Ink && node.kind != NodeKind::Primitive)
        return;

    const qreal worldSw = node.style.strokeWidth;
    const double rw = m_session.frame.drawingRegion.valid ? (m_session.frame.drawingRegion.maxX - m_session.frame.drawingRegion.minX)
                                            : qMax(1.0, double(width()));
    const double sPanel = width() / qMax(1e-6, rw);
    qreal mul = 1.0;
    if ((m_blinkWidthMul > 1.0 && m_blinkInkIds.count(node.id)) || m_highlightInkIds.count(node.id))
        mul = 2.0;
    const qreal lineW = qMax<qreal>(1.0, worldSw * sPanel * mul);

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
        epaper::document::Vec2 contentMin{};
        const epaper::document::Vec2 *minPtr = nullptr;
        if (smartParent && role == "content" && smartParent->inkScaleMode == "fixedInk") {
            contentMin = epaper::document::inkSamplesMin(node.samples);
            minPtr = &contentMin;
        }
        auto toPanel = [&](double x, double y) {
            if (smartParent) {
                const auto w = smartLocalToWorld(x, y, *smartParent, role, node.layoutOffset, minPtr);
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
        const PanelPt tl = worldToPanel(node.gx, node.gy);
        const PanelPt br = worldToPanel(node.gx + node.gw, node.gy + node.gh);
        p.drawRect(QRectF(tl, br).normalized());
        return;
    }
    if (node.geomKind == PrimitiveKind::Ellipse) {
        const PanelPt c = worldToPanel(node.cx, node.cy);
        const PanelPt e = worldToPanel(node.cx + node.rx, node.cy + node.ry);
        p.drawEllipse(c, qAbs(e.x() - c.x()), qAbs(e.y() - c.y()));
    }
}


/**
 * =================================================================================================
 * Connector ink rendering
 *
 * Warped connector polylines for document paint. Panel AABB helpers used by Tool via
 * Surface API are declared there; drawWarpedConnector stays on the paint path.
 * =================================================================================================
 */

/** Stroke a connector’s warpedSamples in panel space. */
void TabletCanvasItem::drawWarpedConnector(QPainter &p, const epaper::document::DocNode &conn)
{
    if (conn.warpedSamples.size() < 2)
        return;
    QPen pen(Qt::black);
    pen.setWidthF(connectorPanelStrokeWidth(conn));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(worldToPanel(conn.warpedSamples[0].x, conn.warpedSamples[0].y));
    for (size_t i = 1; i < conn.warpedSamples.size(); ++i)
        path.lineTo(worldToPanel(conn.warpedSamples[i].x, conn.warpedSamples[i].y));
    p.drawPath(path);
}


/**
 * =================================================================================================
 * Recognizer feedback
 *
 * Temporary ink-width blink and membership highlight after enclose / join. Timers
 * self-expire via m_blinkToken so stale callbacks no-op.
 * =================================================================================================
 */

/** Pulse ink width for listed ink ids, then clear. */
void TabletCanvasItem::beginRecogWidthBlink(const std::vector<std::string> &inkIds)
{
    if (inkIds.empty())
        return;
    ++m_blinkToken;
    const int token = m_blinkToken;
    m_blinkInkIds.clear();
    for (const auto &id : inkIds)
        m_blinkInkIds.insert(id);
    m_blinkWidthMul = 2.0;
    QTimer::singleShot(250, this, [this, token]() {
        if (token != m_blinkToken)
            return;
        m_blinkInkIds.clear();
        m_blinkWidthMul = 1.0;
        if (!m_stroke.active)
            rasterizeVectors(true);
    });
}

/** Mark boundary inks for UI-EP-06 highlight. */
bool TabletCanvasItem::setMembershipHighlight(const std::vector<std::string> &boundaryInkIds)
{
    std::unordered_set<std::string> next;
    for (const auto &id : boundaryInkIds)
        next.insert(id);
    if (next == m_highlightInkIds)
        return false;
    m_highlightInkIds = std::move(next);
    return true;
}

/** Clear membership highlight set. */
void TabletCanvasItem::clearMembershipHighlight()
{
    m_highlightInkIds.clear();
}

/** Gather ink ids under a SmartGroup (boundary or all). */
void TabletCanvasItem::collectSmartGroupInkIds(const epaper::document::DocNode &sg, bool boundaryOnly,
                                               std::vector<std::string> *out) const
{
    using epaper::document::NodeKind;
    if (!out)
        return;
    for (const auto &c : sg.children) {
        if (c.kind != NodeKind::Ink)
            continue;
        const std::string role = c.role ? *c.role : std::string("content");
        if (boundaryOnly && role != "boundary")
            continue;
        out->push_back(c.id);
    }
}


/**
 * =================================================================================================
 * Tool modes / ToolChip
 *
 * QML ToolChip calls armTool / recog toggles. setToolMode updates ChipModel on the session
 * and refreshes Tool selection chrome when leaving sel_*.
 * =================================================================================================
 */

/** Exclusive tool change on session chip (exclusiveToolChanged does the rest). */
void TabletCanvasItem::setToolMode(const QString &mode)
{
    (void)m_session.setExclusiveTool(mode);
}

/** ToolChip tap → setToolMode. */
void TabletCanvasItem::armTool(const QString &mode)
{
    setToolMode(mode);
}

/** Flip ink-box recog arm on session chip. */
void TabletCanvasItem::toggleRecogInkBox()
{
    m_session.flipRecogInkBox();
}

/** Flip connector recog arm on session chip. */
void TabletCanvasItem::toggleRecogConnector()
{
    m_session.flipRecogConnector();
}


/**
 * =================================================================================================
 * Surface API for ToolCanvas
 *
 * Only entry points Tool may call. Intention methods mutate doc/ink/sync or expose
 * read-only geometry helpers; no chrome getters. Peers are not registered here.
 * =================================================================================================
 */

/** Tool-routed stylus sample → ingestMappedTablet. */
void TabletCanvasItem::ingestPen(QEvent::Type type, const PanelPt &canvasPos, RawPt rawPos,
                                const IngestChannels &ch)
{
    ingestMappedTablet(type, canvasPos, rawPos, ch);
}

/** Last QTabletEvent sample for PointHandler pressure path. */
TabletCanvasItem::IngestChannels TabletCanvasItem::stashedChannels(const PanelPt &panel,
                                                                   RawPt *raw) const
{
    IngestChannels ch;
    // No stashed sample: the panel point stands in for a raw one. Only the origin
    // guard reads it, and it compares raw against panel, so the two agreeing is
    // benign — but it is a substitution, hence the explicit cast across spaces.
    *raw = RawPt{panel.x(), panel.y()};
    if (m_stashValid) {
        ch = m_stashTablet;
        *raw = m_stashRaw;
    }
    return ch;
}

/** Invalidate stashed digitizer sample after pen-up. */
void TabletCanvasItem::clearStash()
{
    m_stashValid = false;
}

/** Abort in-flight stroke (pointer cancel). */
void TabletCanvasItem::cancelActiveStroke()
{
    if (m_stroke.active)
        endStroke();
}

/** Surface name for scheduleVectorRasterize. */
void TabletCanvasItem::scheduleDocumentRasterize(bool sharp)
{
    scheduleVectorRasterize(sharp);
}

/** Dirty Tablet panel under live-manip origin hole. */
void TabletCanvasItem::notifyOriginPunch(const QRectF &panelRect)
{
    if (!panelRect.isEmpty())
        update(panelRect.toAlignedRect().adjusted(-8, -8, 8, 8));
}

/** One-way manip_preview JSON to Infini. */
void TabletCanvasItem::publishManipPreview(const std::string &nodeId,
                                           const epaper::document::SmartTransform &liveT,
                                           const epaper::document::SmartBounds *liveB)
{
    if (!m_sync || !m_sync->isConnected())
        return;
    if (nodeId.empty())
        return;
    QJsonObject xf;
    xf.insert(QStringLiteral("x"), liveT.x);
    xf.insert(QStringLiteral("y"), liveT.y);
    xf.insert(QStringLiteral("rotation"), 0);
    xf.insert(QStringLiteral("scaleX"), liveT.scaleX);
    xf.insert(QStringLiteral("scaleY"), liveT.scaleY);
    QJsonObject o;
    o.insert(QStringLiteral("type"), QStringLiteral("manip_preview"));
    o.insert(QStringLiteral("id"), QString::fromStdString(nodeId));
    o.insert(QStringLiteral("transform"), xf);
    if (liveB) {
        QJsonObject b;
        b.insert(QStringLiteral("x"), liveB->x);
        b.insert(QStringLiteral("y"), liveB->y);
        b.insert(QStringLiteral("width"), liveB->width);
        b.insert(QStringLiteral("height"), liveB->height);
        o.insert(QStringLiteral("bounds"), b);
    }
    m_sync->sendLine(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

/** Surface name for flushOneWayWire. */
void TabletCanvasItem::flushWire()
{
    flushOneWayWire();
}

/** Emit canUndo/canRedo after Tool commits a doc op. */
void TabletCanvasItem::notifyHistory()
{
    emit historyChanged();
}

/** Outbound viewport while Epaper→Infini follow is on. */
void TabletCanvasItem::maybePublishLocalViewport(bool settle)
{
    using epaper::handtouch::shouldPublishViewport;
    using epaper::handtouch::uniformScaleOf;
    if (!shouldPublishViewport(followEnum()))
        return;
    ++m_viewportUpCount;
    if (!m_sync || !m_sync->isConnected())
        return;
    QJsonObject dr;
    dr.insert(QStringLiteral("minX"), m_session.frame.drawingRegion.minX);
    dr.insert(QStringLiteral("minY"), m_session.frame.drawingRegion.minY);
    dr.insert(QStringLiteral("maxX"), m_session.frame.drawingRegion.maxX);
    dr.insert(QStringLiteral("maxY"), m_session.frame.drawingRegion.maxY);
    double sx = 1.0;
    double sy = 1.0;
    const double iw = qMax(1.0, double(width()));
    const double ih = qMax(1.0, double(height()));
    uniformScaleOf({0, 0, iw, ih}, m_session.frame.drawingRegion.box(), &sx, &sy);
    QJsonObject o;
    o.insert(QStringLiteral("type"), QStringLiteral("viewport"));
    o.insert(QStringLiteral("source"), QStringLiteral("epaper"));
    o.insert(QStringLiteral("seq"), ++m_viewportSeq);
    o.insert(QStringLiteral("orientation"), QString::fromStdString(m_session.frame.orientation));
    o.insert(QStringLiteral("settle"), settle);
    o.insert(QStringLiteral("scale"), sx);
    Q_UNUSED(sy);
    QJsonObject tr;
    tr.insert(QStringLiteral("x"), 0);
    tr.insert(QStringLiteral("y"), 0);
    o.insert(QStringLiteral("translate"), tr);
    o.insert(QStringLiteral("drawingRegion"), dr);
    m_sync->sendLine(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

/** Bootstrap camera AABB if none yet. */
void TabletCanvasItem::ensureLocalDrawingRegion()
{
    syncFramePanelSize();
    applyFrameIntent(m_session.frame.ensureLocalDrawingRegion());
}

/** Tool-facing debug string → debugInfo property. */
void TabletCanvasItem::setInteractionDebug(const QString &info)
{
    m_debugInfo = info;
    emit debugChanged();
}

/** Live manip ghost: paint SmartGroup + bound connectors. */
void TabletCanvasItem::paintDocumentSubtree(QPainter &painter, const std::string &nodeId)
{
    using namespace epaper::document;
    const DocNode *n = m_session.document.find(nodeId);
    if (!n)
        return;
    drawTree(painter, n->children, n);
    for (const auto &node : m_session.document.rootChildren) {
        if (node.kind != NodeKind::Connector)
            continue;
        if (node.fromNodeId != n->id && node.toNodeId != n->id)
            continue;
        drawWarpedConnector(painter, node);
    }
}

/** Surface name for lodOkPanel. */
bool TabletCanvasItem::lodOkWorld(const epaper::document::SmartBounds &wb) const
{
    return lodOkPanel(wb);
}

/** Panel union of connectors bound to a SmartGroup. */
QRectF TabletCanvasItem::boundConnectorsPanelUnion(const std::string &sgId) const
{
    QRectF u;
    for (const auto &node : m_session.document.rootChildren) {
        if (node.kind != epaper::document::NodeKind::Connector)
            continue;
        if (node.fromNodeId != sgId && node.toNodeId != sgId)
            continue;
        const QRectF r = warpedConnectorPanelRect(node);
        u = u.isEmpty() ? r : u.united(r);
    }
    return u;
}

/** Panel stroke width for a connector node. */
qreal TabletCanvasItem::connectorPanelStrokeWidth(const epaper::document::DocNode &conn) const
{
    double worldSw = conn.style.strokeWidth;
    if (worldSw <= 0.0 && !conn.children.empty())
        worldSw = conn.children.front().style.strokeWidth;
    if (worldSw <= 0.0)
        worldSw = 2.0;
    return qMax<qreal>(1.0, worldSw * panelScale());
}

/** Panel AABB of a connector’s warped polyline. */
QRectF TabletCanvasItem::warpedConnectorPanelRect(const epaper::document::DocNode &conn) const
{
    if (conn.warpedSamples.empty())
        return {};
    // @fix [STORY-EP-031] 0×0 QRectF is empty; united() never grew the connector AABB
    const PanelPt p0 = worldToPanel(conn.warpedSamples[0].x, conn.warpedSamples[0].y);
    qreal minX = p0.x();
    qreal maxX = p0.x();
    qreal minY = p0.y();
    qreal maxY = p0.y();
    for (const auto &s : conn.warpedSamples) {
        const PanelPt p = worldToPanel(s.x, s.y);
        minX = qMin(minX, p.x());
        maxX = qMax(maxX, p.x());
        minY = qMin(minY, p.y());
        maxY = qMax(maxY, p.y());
    }
    return QRectF(PanelPt(minX, minY), PanelPt(maxX, maxY)).normalized().adjusted(-16, -16, 16, 16);
}


/**
 * =================================================================================================
 * Pointer sample stash
 *
 * QtInputFilter pushes full digitizer channels here; Tool’s DragHandler only sees
 * pressure, so Surface ingestPen reads the stash.
 * =================================================================================================
 */

/** Cache raw + PenSample from the input filter. */
void TabletCanvasItem::stashTabletSample(const QPointF &raw, const IngestChannels &ch)
{
    m_stashRaw = RawPt{raw.x(), raw.y()};
    m_stashTablet = ch;
    m_stashValid = true;
}


/**
 * =================================================================================================
 * Region sync and viewport follow
 *
 * Inbound viewport from Infini applies camera when follow allows. Outbound publish is
 * maybePublishLocalViewport (Surface). Follow toggle lives in Main.qml on drawCanvas.
 * =================================================================================================
 */

/** Cycle follow direction; may apply Infini camera. */
void TabletCanvasItem::tapFollowToggle()
{
    m_session.follow.connected = m_sync && m_sync->isConnected();
    m_session.follow.exclusiveTool = toolMode().toStdString();
    const auto r = m_session.follow.tapToggle();
    m_session.setFollowDirection(QString::fromLatin1(epaper::handtouch::followId(m_session.follow.direction)));
    emit followChanged();
    flushFollowOutbound();
    if (r.appliedInfiniViewport)
        applyFollowCamera();
}

/** Inbound viewport JSON → orientation + drawingRegion. */
void TabletCanvasItem::applyViewport(const QJsonObject &obj)
{
    m_viewportSeq = obj.value(QStringLiteral("seq")).toInt(m_viewportSeq);
    using epaper::canvasframe::FrameIntent;
    FrameIntent intent = FrameIntent::None;
    const QString orient = obj.value(QStringLiteral("orientation")).toString();
    if (!orient.isEmpty())
        intent |= m_session.frame.setOrientation(orient.toStdString());

    const QJsonObject dr = obj.value(QStringLiteral("drawingRegion")).toObject();
    if (!dr.isEmpty())
        intent |= m_session.frame.applyDrawingRegion(aabbFromJson(dr), true);

    applyFrameIntent(intent);
    const bool settle = obj.value(QStringLiteral("settle")).toBool(false);
    qInfo() << "[sync] viewport seq" << m_viewportSeq << "orientation"
            << QString::fromStdString(m_session.frame.orientation) << "settle" << settle << "ink"
            << m_session.document.inkCount();
    // Soft path already ran via cameraChanged; settle wants a sharp pass.
    scheduleVectorRasterize(settle);
}

/** Map cached Infini viewport into local camera. */
void TabletCanvasItem::applyFollowCamera()
{
    if (!m_session.follow.mapApplied && !m_session.follow.hasInfiniViewport)
        return;
    m_session.follow.applyInfiniViewportIfFollowing();
    if (m_session.follow.direction != epaper::handtouch::FollowDirection::InfiniToEpaper)
        return;
    applyFrameIntent(m_session.frame.applyDrawingRegion(m_session.follow.localCamera, true));
    scheduleVectorRasterize(true);
}

/** Send follow-session outbound lines on the wire. */
void TabletCanvasItem::flushFollowOutbound()
{
    if (!m_sync || !m_sync->isConnected()) {
        m_session.follow.outbound.clear();
        return;
    }
    for (const std::string &line : m_session.follow.outbound)
        m_sync->sendLine(QByteArray::fromStdString(line));
    m_session.follow.outbound.clear();
}

/** Remember last Infini drawingRegion for follow-on. */
void TabletCanvasItem::cacheInfiniViewport(const QJsonObject &obj)
{
    const QJsonObject dr = obj.value(QStringLiteral("drawingRegion")).toObject();
    if (dr.isEmpty())
        return;
    m_session.follow.cacheInfiniViewport(aabbFromJson(dr));
}

/** Session followDirection string → FollowDirection enum. */
epaper::handtouch::FollowDirection TabletCanvasItem::followEnum() const
{
    return epaper::handtouch::parseFollow(m_session.followDirection().toStdString());
}


/**
 * =================================================================================================
 * One-way sync wire
 *
 * Pen strokes publish begin/point/end through OneWaySyncSession. onHostMessage handles
 * handshake, viewport, and doc_load; clears Tool selection when host says none.
 * =================================================================================================
 */

/** Open an outbound stroke on the one-way session. */
void TabletCanvasItem::syncBegin()
{
    // @implements [SRS-EP-08] stroke_begin without intent
    m_oneWay.beginPreviewStroke(m_stroke.activeStrokeId);
    flushOneWayWire();
}

/** Append a stroke point to the outbound buffer. */
void TabletCanvasItem::syncPoint(const Point &pt)
{
    // @implements [SRS-EP-02] live preview in world — same space as append_ink
    ensureLocalDrawingRegion();
    const WorldPt world = panelToWorld(pt.pos);
    m_oneWay.previewStrokePoint(m_stroke.activeStrokeId, world.x, world.y, pt.pressure);
    flushOneWayWire();
}

/** Close the outbound stroke. */
void TabletCanvasItem::syncEnd()
{
    m_oneWay.endPreviewStroke(m_stroke.activeStrokeId);
    flushOneWayWire();
}

/** Drain session outbound lines to StrokeSync. */
void TabletCanvasItem::flushOneWayWire()
{
    m_oneWay.onLocalCommit();
    if (!m_sync->isConnected())
        return;
    for (const std::string &line : m_oneWay.takeOutbound())
        m_sync->sendLine(QByteArray::fromStdString(line));
}

/** Inbound JSON: follow, viewport, doc sync, selection clear. */
void TabletCanvasItem::onHostMessage(const QJsonObject &obj)
{
    epaper::UiStallSection stall("onHostMessage");
    const QString inboundType = obj.value(QStringLiteral("type")).toString();
    if (inboundType == QLatin1String("viewport_follow")) {
        const QByteArray raw = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        const auto msg = epaper::follow::parseViewportFollowLine(
            std::string(raw.constData(), static_cast<size_t>(raw.size())));
        if (m_session.follow.adoptInbound(msg)) {
            m_session.setFollowDirection(QString::fromLatin1(epaper::handtouch::followId(m_session.follow.direction)));
            emit followChanged();
            if (m_session.follow.epaperFollowOn())
                applyFollowCamera();
        }
        m_oneWay.handleInboundLine(std::string(raw.constData(), static_cast<size_t>(raw.size())));
        flushOneWayWire();
        return;
    }
    if (inboundType != QLatin1String("viewport"))
        qInfo() << "[sync] inbound" << inboundType << "live" << m_oneWay.epochLive()
                << "handshake" << m_oneWay.handshakeInFlight();
    const QByteArray raw = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    m_oneWay.handleInboundLine(std::string(raw.constData(), static_cast<size_t>(raw.size())));
    if (m_oneWay.viewportApplied() > 0 && inboundType == QLatin1String("viewport")) {
        cacheInfiniViewport(obj);
        // @implements [SRS-EP-21] apply Infini viewport only while following
        if (epaper::handtouch::shouldApplyInboundViewport(followEnum()))
            applyViewport(obj);
        else
            qInfo() << "[sync] ignore inbound viewport follow=" << m_session.followDirection();
    }
    flushOneWayWire();
    if (obj.value(QStringLiteral("type")).toString() == QLatin1String("doc_load")
        && m_oneWay.epochLive())
        scheduleVectorRasterize(true);
}


/**
 * =================================================================================================
 * Chrome layout
 *
 * Fixed orientation-top slots for ToolChip, Follow, USB, DBG, hand-touch. Main.qml binds
 * rect properties; interaction stays on ToolCanvas.
 * =================================================================================================
 */

/** Recompute chrome rects from panel size + orientation. */
void TabletCanvasItem::updateToolChipRect()
{
    // UI-EP-04 + ADR-0021: 3 exclusive tools + 12 px publish + 2 toggles + Undo/Redo.
    // @implements [SRS-EP-05] floating ToolChip hit bounds (64×64 tiles, CHL-0019)
    const qreal chipH = epaper::toolchip::kHeight;
    const qreal chipW = epaper::toolchip::chipWidth();
    const qreal inset = 8.0;
    qreal top = inset;
    if (m_session.frame.orientation == "gutOnTop")
        top = height() - inset - chipH;
    const qreal left = (width() - chipW) * 0.5;
    const QRectF next(left, top, chipW, chipH);
    if (next != m_toolChipRect) {
        m_toolChipRect = next;
        emit toolChipRectChanged();
    }
    const bool gutOnTop = m_session.frame.orientation == "gutOnTop";
    const auto fr = epaper::follow::followToggleRect(width(), height(), gutOnTop);
    const auto ur = epaper::follow::usbLinkRect(width(), height(), gutOnTop);
    const auto dr = epaper::follow::debugToggleRect(width(), height(), gutOnTop);
    const auto lr = epaper::follow::debugLogRect(width(), height(), gutOnTop);
    const QRectF followNext = panelToQ(fr);
    const QRectF usbNext = panelToQ(ur);
    const QRectF debugNext = panelToQ(dr);
    const QRectF logNext = panelToQ(lr);
    const QRectF handNext = QRectF(next.x() + epaper::toolchip::kPublish, next.y(),
                                   epaper::toolchip::kTile, epaper::toolchip::kHeight);
    if (followNext != m_followToggleRect || usbNext != m_usbLinkRect
        || debugNext != m_debugToggleRect || handNext != m_handTouchToggleRect
        || logNext != m_debugLogRect) {
        m_followToggleRect = followNext;
        m_usbLinkRect = usbNext;
        m_debugToggleRect = debugNext;
        m_handTouchToggleRect = handNext;
        m_debugLogRect = logNext;
        emit trailingChromeChanged();
        auto fmt = [](const QRectF &r) {
            return QStringLiteral("%1,%2 %3x%4")
                .arg(int(r.x())).arg(int(r.y())).arg(int(r.width())).arg(int(r.height()));
        };
        qInfo().noquote() << QStringLiteral("[chrome] rects panel=%1x%2 chip=%3 dbg=%4 follow=%5 usb=%6")
                                 .arg(int(width())).arg(int(height()))
                                 .arg(fmt(m_toolChipRect), fmt(m_debugToggleRect),
                                      fmt(m_followToggleRect), fmt(m_usbLinkRect));
    }
}


/**
 * =================================================================================================
 * Debug and diagnostics
 *
 * DBG toggle and debugInfo string shown in Main.qml. Interaction debug text is written
 * through Surface setInteractionDebug.
 * =================================================================================================
 */

/** Show/hide the on-device debug log panel. */
void TabletCanvasItem::toggleDebugLog()
{
    m_debugLogVisible = !m_debugLogVisible;
    emit debugLogVisibleChanged();
    // Says whether a missed DBG tap never reached the handler or only failed to paint.
    qInfo().noquote() << QStringLiteral("[chrome] dbg toggle %1 rect=%2,%3 %4x%5")
                             .arg(m_debugLogVisible ? "on" : "off")
                             .arg(int(m_debugToggleRect.x()))
                             .arg(int(m_debugToggleRect.y()))
                             .arg(int(m_debugToggleRect.width()))
                             .arg(int(m_debugToggleRect.height()));
}

