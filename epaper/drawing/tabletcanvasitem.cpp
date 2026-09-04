#include "tabletcanvasitem.h"
#include "debug/ui_stall.hpp"
#include "debug/ink_path_probe.hpp"
#include "debug/rasterize_probe.hpp"
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
#include "rasterize_gate.hpp"
#include "rendering/rendering_qt.hpp"

#include <QStringList>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QQuickWindow>
#include <QCoreApplication>
#include <QMetaObject>
#include <QtMath>
#include <QByteArray>
#include <QTransform>
#include <QTimer>
#include <QDebug>
#include <QSizeF>
#include <cstdio>
#include <memory>
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
        TabletCanvasItem::IngestChannels ch;
        ch.pressure = 0.6;
        const TabletCanvasItem::PanelPt p0(x0, y0);
        canvas->ingestMappedTablet(QEvent::TabletPress, p0, {p0.x(), p0.y()}, ch);
        for (int i = 1; i < n; ++i) {
            ch.pressure = 0.55;
            const TabletCanvasItem::PanelPt p(x0 + i * 12.0, y0 + i * 4.0);
            canvas->ingestMappedTablet(QEvent::TabletMove, p, {p.x(), p.y()}, ch);
        }
        ch.pressure = 0.0;
        const TabletCanvasItem::PanelPt pe(x0 + n * 12.0, y0 + n * 4.0);
        canvas->ingestMappedTablet(QEvent::TabletRelease, pe, {pe.x(), pe.y()}, ch);
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
    armDocProbeFromEnv();

    setAntialiasing(false);
    setRenderTarget(QQuickPaintedItem::Image);
    setOpaquePainting(m_paintsInk);
    setFillColor(m_paintsInk ? QColor(Qt::white) : QColor(Qt::transparent));
    m_flushClock.start();
    m_refreshClock.start();
    m_rasterTimer.setSingleShot(true);
    connect(&m_rasterTimer, &QTimer::timeout, this, &TabletCanvasItem::onRasterTimer);
    m_stroke.mintNodeId = [this]() { return m_session.document.generateNodeId(); };
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
    m_renderer.setAlgorithm(std::make_unique<epaper::render::HierarchyCullAlgorithm>());
    m_cameraJob.start(
        [](const epaper::camerasharp::Job &job, const std::atomic<bool> &cancel) {
            return epaper::camerasharp::run(job, cancel);
        },
        [this](epaper::camerasharp::Result result) {
            QMetaObject::invokeMethod(
                this,
                [this, r = std::move(result)]() mutable { onCameraSharpResult(std::move(r)); },
                Qt::QueuedConnection);
        });

    connect(&m_session, &CanvasSession::cameraChanged, this, [this]() {
        m_rasterWhy = epaper::rasterprobe::Why::Camera;
        scheduleVectorRasterize(false);
    });
    connect(&m_session, &CanvasSession::documentMutated, this, [this]() {
        bumpSnapEpoch();
        // [D05] noteDocumentDirty already painted; FullClear would hitch the next down.
        if (m_consumeMutatedRasterize) {
            m_consumeMutatedRasterize = false;
            return;
        }
        m_rasterWhy = epaper::rasterprobe::Why::Mutated;
        scheduleVectorRasterize(true);
    });
    connect(&m_session, &CanvasSession::exclusiveToolChanged, this, [this]() {
        emit toolModeChanged();
        m_debugInfo = QStringLiteral("tool=%1").arg(toolMode());
        emit debugChanged();
    });
    connect(&m_session, &CanvasSession::followChanged, this, &TabletCanvasItem::followChanged);
    connect(&m_session, &CanvasSession::recogChanged, this, &TabletCanvasItem::recogChanged);
}

TabletCanvasItem::~TabletCanvasItem()
{
    m_cameraJob.stop();
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
 * Live ink stamps into m_image; paint() blits it. During live manip the manipulated subtree is
 * omitted via session suppressIds at rasterize time; ToolCanvas paints the live ghost.
 * flushPending coalesces dirty rects to the e-ink path.
 * =================================================================================================
 */

/** Blit ink buffer. */
void TabletCanvasItem::paint(QPainter *painter)
{
    epaper::inkpath::Span span("tabletPaint");
    if (!m_paintsInk)
        return;

    // SRS-EP-13: do not hit-test or time the stub document here.
    ensureImage();
    painter->drawImage(0, 0, m_image);
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

/** Coalesce dirty rects and swapBuffers. */
void TabletCanvasItem::flushPending()
{
    if (m_pendingDirty.isNull())
        return;

    EpaperBridge::instance()->traceFlush();

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
    const UndoRingEntry *peek =
        isUndo ? m_session.document.newestEntry() : m_session.document.newestRedoEntry();
    std::vector<std::string> ids;
    QRectF dirty;
    if (peek) {
        for (const auto &t : peek->targets)
            ids.push_back(t.nodeId);
    }
    // @fix [CHL-0031] child ink of a SmartGroup is group-local; dirty the parent box
    for (const auto &id : ids) {
        const QRectF b = panelBoundOfNodeId(id);
        if (!b.isEmpty())
            dirty = dirty.isEmpty() ? b : dirty.united(b);
    }

    const UndoResult r = isUndo ? m_session.document.undo() : m_session.document.redo();
    (void)r;
    emitRecogChrome(0, {});
    notifyHistory();

    for (const auto &id : ids) {
        const QRectF b = panelBoundOfNodeId(id);
        if (!b.isEmpty())
            dirty = dirty.isEmpty() ? b : dirty.united(b);
    }
    // [D09] disappeared node with no before/after AABB → FullClear (no hole).
    if (dirty.isEmpty())
        m_session.noteDocumentMutated();
    else {
        scheduleDirtyRasterize(dirty, true);
        m_session.noteDocumentMutated();
    }
    flushOneWayWire();
}


/**
 * =================================================================================================
 * Pen stroke capture and ingest
 *
 * Digitizer samples enter via ingestMappedTablet (also Surface ingestPen).
 * StrokeCapture owns the stroke. Selection/handle presses are decided on Tool before
 * ingestPen; this path is ink only.
 * =================================================================================================
 */

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
    if (has(r.intent, StrokeIntent::PreviewBegin)) {
        epaper::inkpath::Span span("syncBegin");
        syncBegin();
    }
    if (has(r.intent, StrokeIntent::PreviewPoint) && r.hasPreviewSample) {
        epaper::inkpath::Span span("syncPoint");
        syncPoint(makePoint(r.previewSample));
    }
    if (has(r.intent, StrokeIntent::EmitSegment) && r.hasSegment) {
        epaper::inkpath::Span span("emitSegment");
        emitSegment(makePoint(r.segmentFrom), makePoint(r.segmentTo));
    }
    if (has(r.intent, StrokeIntent::PreviewEnd))
        syncEnd();
    if (has(r.intent, StrokeIntent::StrokeCountChanged))
        emit strokeCountChanged();
    // Pixels before ingest — SRS-EP-07 / EP-13.
    if (has(r.intent, StrokeIntent::FlushInk)) {
        epaper::inkpath::Span span("flushPending");
        flushPending();
    }
    if (has(r.intent, StrokeIntent::IngestReady) && r.hasFinished) {
        epaper::inkpath::Span span("ingestDoc");
        ingestCurrentStroke(r.finished);
    }
    if (has(r.intent, StrokeIntent::AbortGesture))
        m_session.document.abortGesture();
    if (has(r.intent, StrokeIntent::NotifyHistory))
        notifyHistory();
    if (has(r.intent, StrokeIntent::FlushWire))
        flushOneWayWire();
    if (has(r.intent, StrokeIntent::ChipPenUp))
        m_session.chip.penUp();
}

/** Pen-down for ink only — Tool already branched selection/handle/erase away. */
void TabletCanvasItem::applyContactPress(const PanelPt &canvasPos, const IngestChannels &ch)
{
    if (m_session.chip.exclusive != "pen")
        return;
    beginStroke(canvasPos, ch);
}

/** Start StrokeCapture + syncBegin. */
void TabletCanvasItem::beginStroke(const PanelPt &canvasPos, const IngestChannels &ch)
{
    bumpSnapEpoch();
    m_heldSharp.reset();
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

    // [D06] Enclose/connector create: one InPlaceDirty of the changed AABB.
    if (m_needEncloseRasterize) {
        m_needEncloseRasterize = false;
        const QRectF dirty = m_encloseDirtyPanel;
        m_encloseDirtyPanel = QRectF();
        m_rasterizeDeferredSharp = false;
        m_rasterizePending = false;
        m_pendingInPlaceDirty = QRectF();
        m_rasterWhy = epaper::rasterprobe::Why::Enclose;
        rasterizeVectors(true, dirty);
        if (m_cameraNeedsSharp)
            submitCameraSharp();
    } else if (m_heldSharp) {
        auto r = std::move(*m_heldSharp);
        m_heldSharp.reset();
        if (r.snapEpoch == m_snapEpoch)
            applyCameraSharp(std::move(r));
        else if (m_cameraNeedsSharp)
            submitCameraSharp();
    } else if (m_cameraNeedsSharp) {
        submitCameraSharp();
    } else if (m_rasterizeDeferredSharp || m_rasterizePending) {
        m_rasterizePending = true;
        armRasterTimer();
    }

    // Status text is refreshed between strokes only: during a stroke it would
    // add a second damage region per flush.
    m_debugInfo = QStringLiteral("(%1,%2) sz=%3x%4 ink=%5")
                      .arg(int(m_stroke.lastPanelX))
                      .arg(int(m_stroke.lastPanelY))
                      .arg(int(width()))
                      .arg(int(height()))
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
    if (tool == "sel_rect" || tool == "sel_freeform" || epaper::toolchip::isEraserId(tool))
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
    std::string extra;
    if (d.outcome == RecogOutcome::EndpointInk && d.endpointInk.bound)
        extra = "end=" + d.endpointInk.end + " id=" + d.endpointInk.connectorId;
    qInfo().noquote() << QString::fromStdString(
        epaper::debuglog::formatRecogLog(d.outcomeName(), d.guard, d.encloseWhy, extra));
    if (!d.connector.diag.empty()) {
        qInfo().noquote() << QString::fromStdString(
            epaper::debuglog::formatConnLog(d.connector.diag, d.connector.reason));
    }
    if (d.outcome == RecogOutcome::Enclose && d.enclose.kind == EncloseKind::Created) {
        const std::string line = epaper::debuglog::formatEncloseLog(
            "Created", d.enclose.reason, d.enclose.smartGroupId, d.enclose.childIds);
        qInfo().noquote() << QString::fromStdString(line);
        m_needEncloseRasterize = true;
        m_encloseDirtyPanel = panelBoundOfNodeId(d.enclose.smartGroupId);
        std::vector<std::string> ids;
        ids.push_back(d.enclose.smartGroupId);
        if (const DocNode *sg = m_session.document.find(d.enclose.smartGroupId))
            collectSmartGroupInkIds(*sg, false, &ids);
        emitRecogChrome(1, ids);
    } else if (d.outcome == RecogOutcome::Membership) {
        std::vector<std::string> ids;
        if (const DocNode *sg = m_session.document.find(d.membership.smartGroupId))
            collectSmartGroupInkIds(*sg, true, &ids);
        // [D01][D16] live stamps are the join; bold is ToolCanvas. Skip Tablet rasterize.
        emitRecogChrome(3, ids);
    } else if (d.outcome == RecogOutcome::Connector) {
        m_needEncloseRasterize = true;
        std::vector<std::string> ids = d.connector.bodyIds;
        ids.push_back(d.connector.fromId);
        ids.push_back(d.connector.toId);
        QRectF dirty;
        if (const DocNode *conn = m_session.document.find(d.connector.connectorId))
            dirty = warpedConnectorPanelRect(*conn);
        dirty = dirty.isEmpty() ? panelBoundOfNodeId(d.connector.fromId)
                                : dirty.united(panelBoundOfNodeId(d.connector.fromId));
        dirty = dirty.united(panelBoundOfNodeId(d.connector.toId));
        m_encloseDirtyPanel = dirty;
        if (const DocNode *sg = m_session.document.find(d.connector.fromId))
            collectSmartGroupInkIds(*sg, false, &ids);
        if (const DocNode *sg = m_session.document.find(d.connector.toId))
            collectSmartGroupInkIds(*sg, false, &ids);
        emitRecogChrome(2, ids);
    } else {
        // Failed empty enclose stays live ink. Do not white-clear the panel.
        const std::string &why = d.enclose.reason;
        const bool failedEncloseStayInk =
            why.find("not_primitive") != std::string::npos
            || why.find("too_small") != std::string::npos;
        emitRecogChrome(0, {});
        if (failedEncloseStayInk) {
            m_rasterizeDeferredSharp = false;
            m_rasterizePending = false;
        }
        // [D01] ordinary Ink: skip document rasterize (live stamps are the settle).
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
 * scheduleVectorRasterize queues a soft/sharp redraw of vectors into m_image.
 * Deferred while live ink or an erase ghost owns the buffer (FullClear would
 * wipe them). Live manip must still rasterize so suppressIds punch the origin.
 * =================================================================================================
 */

epaper::rasterprobe::CamBox TabletCanvasItem::currentCamBox() const
{
    return epaper::camerasharp::camBoxOf(m_session.frame);
}

void TabletCanvasItem::bumpSnapEpoch()
{
    ++m_snapEpoch;
    m_cameraSnap.reset();
}

/** @implements [SRS-EP-03] LatestJob camera vector — not on the GUI pointer stack */
void TabletCanvasItem::submitCameraSharp()
{
    if (!m_paintsInk || m_image.isNull())
        return;
    if (m_stroke.active || m_erasePointerActive)
        return;
    syncFramePanelSize();
    if (!m_session.frame.drawingRegion.valid)
        return;
    if (!m_cameraSnap || m_cameraSnapEpoch != m_snapEpoch) {
        m_cameraSnap = std::make_shared<const std::vector<epaper::document::DocNode>>(
            m_session.document.rootChildren);
        m_cameraSnapEpoch = m_snapEpoch;
    }
    epaper::camerasharp::Job job;
    job.id = ++m_cameraJobSeq;
    job.snapEpoch = m_snapEpoch;
    job.frame = m_session.frame;
    job.tree = m_cameraSnap;
    job.suppressIds = m_session.liveManipSuppressIds();
    job.imageW = m_image.width();
    job.imageH = m_image.height();
    job.format = m_image.format();
    m_cameraNeedsSharp = true;
    // Let in-flight finish: a completed buffer is warped toward `now`. Cancelling
    // every 200 ms nav tick meant dense pages (~700 ms vector) never sharpened.
    m_cameraJob.submit(std::move(job), false);
}

void TabletCanvasItem::onCameraSharpResult(epaper::camerasharp::Result result)
{
    if (result.cancelled || result.image.isNull())
        return;
    if (result.snapEpoch != m_snapEpoch)
        return;
    if (m_stroke.active || m_erasePointerActive) {
        m_heldSharp = std::move(result);
        return;
    }
    applyCameraSharp(std::move(result));
}

void TabletCanvasItem::applyCameraSharp(epaper::camerasharp::Result result)
{
    ensureImage();
    if (m_image.isNull() || result.image.isNull())
        return;
    syncFramePanelSize();
    const auto now = currentCamBox();
    const auto delta = epaper::rasterprobe::classifyCamera(
        result.cam, now, m_session.frame.panelW,
        result.orient != m_session.frame.orientation);

    QElapsedTimer clock;
    clock.start();
    bool warped = false;
    if (delta.kind == epaper::rasterprobe::CamKind::None) {
        m_image = std::move(result.image);
        m_cameraNeedsSharp = false;
    } else {
        QImage next(m_image.size(), m_image.format());
        if (!blitOnto(&next, result.image, result.cam)) {
            m_cameraNeedsSharp = true;
            submitCameraSharp();
            return;
        }
        m_image = std::move(next);
        warped = true;
        m_cameraNeedsSharp = true;
    }
    const int blitMs = int(clock.restart());
    {
        epaper::inkpath::Span span("rasterize.update");
        update(m_image.rect());
    }
    const int updateMs = int(clock.elapsed());

    epaper::rasterprobe::Record rec;
    rec.why = epaper::rasterprobe::Why::Camera;
    rec.cam = delta;
    rec.inplace = false;
    rec.sharp = !warped;
    rec.blit = warped;
    rec.totalMs = (warped ? blitMs : result.renderMs) + updateMs;
    rec.warpMs = 0;
    rec.renderMs = warped ? blitMs : result.renderMs;
    rec.updateMs = updateMs;
    rec.ink = m_session.document.inkCount();
    rec.nodes = m_session.document.nodeCount();
    rec.visits = int(result.stats.visits);
    rec.skipped = int(result.stats.skipped);
    rec.polylines = int(result.stats.polylines);
    rec.pts = int(result.stats.pts);
    finishRasterize(rec, now);
    if (warped)
        submitCameraSharp();
}

bool TabletCanvasItem::blitOnto(QImage *dst, const QImage &src, epaper::rasterprobe::CamBox srcCam)
{
    if (!dst || dst->isNull() || src.isNull() || !srcCam.valid)
        return false;
    const QRect img = dst->rect();
    if (src.size() != dst->size())
        return false;

    const auto now = currentCamBox();
    const auto cam = epaper::rasterprobe::classifyCamera(srcCam, now, m_session.frame.panelW,
                                                         false);
    const auto plan = epaper::rasterprobe::planCameraPaint(cam.kind, false);

    if (plan.paint == epaper::rasterprobe::CameraPaint::Skip) {
        *dst = src;
        return true;
    }

    if (plan.paint == epaper::rasterprobe::CameraPaint::BlitPan) {
        epaper::canvasframe::CanvasFrame oldF = m_session.frame;
        oldF.drawingRegion.minX = srcCam.minX;
        oldF.drawingRegion.minY = srcCam.minY;
        oldF.drawingRegion.maxX = srcCam.maxX;
        oldF.drawingRegion.maxY = srcCam.maxY;
        oldF.drawingRegion.valid = true;
        const double wx = (srcCam.minX + srcCam.maxX) * 0.5;
        const double wy = (srcCam.minY + srcCam.maxY) * 0.5;
        const auto o = oldF.worldToPanel(wx, wy);
        const auto n = m_session.frame.worldToPanel(wx, wy);
        const int dx = int(std::lround(n.x - o.x));
        const int dy = int(std::lround(n.y - o.y));
        if (std::abs(dx) >= img.width() || std::abs(dy) >= img.height())
            return false;
        dst->fill(Qt::white);
        QPainter p(dst);
        p.drawImage(QPoint(dx, dy), src);
        return true;
    }

    if (plan.paint == epaper::rasterprobe::CameraPaint::BlitScale) {
        const auto a = m_session.frame.worldToPanel(srcCam.minX, srcCam.minY);
        const auto b = m_session.frame.worldToPanel(srcCam.maxX, srcCam.minY);
        const auto c = m_session.frame.worldToPanel(srcCam.minX, srcCam.maxY);
        const auto d = m_session.frame.worldToPanel(srcCam.maxX, srcCam.maxY);
        const qreal minX = std::min({a.x, b.x, c.x, d.x});
        const qreal minY = std::min({a.y, b.y, c.y, d.y});
        const qreal maxX = std::max({a.x, b.x, c.x, d.x});
        const qreal maxY = std::max({a.y, b.y, c.y, d.y});
        const QRectF dest(minX, minY, maxX - minX, maxY - minY);
        if (dest.width() < 2.0 || dest.height() < 2.0)
            return false;
        dst->fill(Qt::white);
        QPainter p(dst);
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
        p.drawImage(dest, src, QRectF(src.rect()));
        return true;
    }
    return false;
}

void TabletCanvasItem::armRasterTimer()
{
    if (!m_rasterTimer.isActive())
        m_rasterTimer.start(int(kRefreshMinIntervalMs));
}

void TabletCanvasItem::onRasterTimer()
{
    if (!m_rasterizePending && !m_rasterizeDeferredSharp)
        return;
    if (epaper::render::deferVectorRasterize(m_stroke.active, m_erasePointerActive, m_pointerBusy)) {
        armRasterTimer();
        return;
    }
    if (m_pendingInPlaceDirty.isEmpty()) {
        m_rasterizePending = false;
        m_rasterizeSharp = false;
        m_rasterizeDeferredSharp = false;
        if (m_cameraNeedsSharp)
            submitCameraSharp();
        return;
    }
    const bool doSharp = m_rasterizeSharp || m_rasterizeDeferredSharp;
    m_rasterizePending = false;
    m_rasterizeSharp = false;
    m_rasterizeDeferredSharp = false;
    const QRectF dirty = m_pendingInPlaceDirty;
    m_pendingInPlaceDirty = QRectF();
    if (m_rasterWhy == epaper::rasterprobe::Why::Unknown)
        m_rasterWhy = epaper::rasterprobe::Why::Dirty;
    rasterizeVectors(doSharp, dirty);
}

void TabletCanvasItem::setPointerBusy(bool on)
{
    m_pointerBusy = on;
}

void TabletCanvasItem::flushDeferredRasterize()
{
    if (m_heldSharp && !m_stroke.active && !m_erasePointerActive) {
        auto r = std::move(*m_heldSharp);
        m_heldSharp.reset();
        if (r.snapEpoch == m_snapEpoch)
            applyCameraSharp(std::move(r));
    }
    if (m_cameraNeedsSharp && !m_stroke.active && !m_erasePointerActive)
        submitCameraSharp();
    if (m_rasterizePending || m_rasterizeDeferredSharp || m_rasterizeSharp
        || !m_pendingInPlaceDirty.isEmpty()) {
        m_rasterizePending = true;
        armRasterTimer();
    }
}

void TabletCanvasItem::finishRasterize(const epaper::rasterprobe::Record &rec,
                                      const epaper::rasterprobe::CamBox &next)
{
    epaper::rasterprobe::logRaster(rec);
    const QString line = QString::fromStdString(epaper::rasterprobe::formatRasterLine(rec));
    qInfo().noquote() << line.trimmed();
    m_rasterCam = next;
    m_rasterOrient = m_session.frame.orientation;
    m_rasterWhy = epaper::rasterprobe::Why::Unknown;
}

/** Queue refresh. Camera: blit now + LatestJob sharp. Dirty/enclose stay GUI. */
void TabletCanvasItem::scheduleVectorRasterize(bool sharp)
{
    const bool deferWipe =
        epaper::render::deferFullDocumentRasterize(m_stroke.active, m_erasePointerActive);

    if (m_rasterWhy == epaper::rasterprobe::Why::Camera) {
        syncFramePanelSize();
        const auto cam = epaper::rasterprobe::classifyCamera(
            m_rasterCam, currentCamBox(), m_session.frame.panelW,
            m_rasterOrient != m_session.frame.orientation);
        const auto plan = epaper::rasterprobe::planCameraPaint(cam.kind, sharp);
        if (plan.paint == epaper::rasterprobe::CameraPaint::Skip) {
            if (m_cameraNeedsSharp && !deferWipe)
                submitCameraSharp();
            return;
        }
        const bool blit = plan.paint == epaper::rasterprobe::CameraPaint::BlitPan
            || plan.paint == epaper::rasterprobe::CameraPaint::BlitScale;
        if (blit) {
            if (deferWipe) {
                m_cameraNeedsSharp = true;
                return;
            }
            m_rasterizePending = false;
            m_rasterizeDeferredSharp = false;
            rasterizeVectors(false);
            return;
        }
        if (epaper::render::deferVectorRasterize(m_stroke.active, m_erasePointerActive,
                                                 m_pointerBusy)) {
            submitCameraSharp();
            return;
        }
        m_rasterizePending = false;
        m_rasterizeDeferredSharp = false;
        rasterizeVectors(true);
        m_cameraNeedsSharp = false;
        return;
    }

    if (deferWipe) {
        if (sharp)
            m_rasterizeDeferredSharp = true;
        else if (!m_rasterizeDeferredSharp)
            m_rasterizePending = true;
        armRasterTimer();
        return;
    }

    if (sharp) {
        m_rasterizePending = false;
        m_rasterizeDeferredSharp = false;
        m_pendingInPlaceDirty = QRectF();
        rasterizeVectors(true);
        m_rasterizeSharp = false;
        m_cameraNeedsSharp = false;
        return;
    }
    if (m_rasterizePending)
        return;
    m_rasterizePending = true;
    armRasterTimer();
}


/** [D05] InPlaceDirty of a panel AABB; FullClear wins if also requested. */
void TabletCanvasItem::scheduleDirtyRasterize(const QRectF &panelDirty, bool sharp)
{
    m_consumeMutatedRasterize = true;
    m_rasterWhy = epaper::rasterprobe::Why::Dirty;
    const QRectF padded = padRasterDirty(panelDirty);
    if (epaper::render::deferFullDocumentRasterize(m_stroke.active, m_erasePointerActive)) {
        m_pendingInPlaceDirty = m_pendingInPlaceDirty.isEmpty()
            ? padded
            : m_pendingInPlaceDirty.united(padded);
        if (sharp)
            m_rasterizeDeferredSharp = true;
        else if (!m_rasterizeDeferredSharp)
            m_rasterizePending = true;
        armRasterTimer();
        return;
    }
    m_pendingInPlaceDirty = QRectF();
    rasterizeVectors(sharp, padded);
}

QRectF TabletCanvasItem::padRasterDirty(const QRectF &panel) const
{
    const qreal pad = std::max(16.0, 8.0 * panelScale());
    return panel.adjusted(-pad, -pad, pad, pad);
}

epaper::render::WorldAabb TabletCanvasItem::panelRectToWorldClip(const QRectF &panel) const
{
    const WorldPt a = panelToWorld(panel.topLeft());
    const WorldPt b = panelToWorld(panel.topRight());
    const WorldPt c = panelToWorld(panel.bottomLeft());
    const WorldPt d = panelToWorld(panel.bottomRight());
    epaper::render::WorldAabb w;
    w.minX = std::min({a.x, b.x, c.x, d.x});
    w.maxX = std::max({a.x, b.x, c.x, d.x});
    w.minY = std::min({a.y, b.y, c.y, d.y});
    w.maxY = std::max({a.y, b.y, c.y, d.y});
    return w;
}

QRectF TabletCanvasItem::panelBoundOfNodeId(const std::string &id) const
{
    using namespace epaper::document;
    SmartBounds b;
    if (!nodeInvalidateAabb(m_session.document, id, b))
        return {};
    return worldBoundsToPanel(b);
}

void TabletCanvasItem::emitRecogChrome(int kind, const std::vector<std::string> &ids)
{
    QStringList list;
    for (const auto &id : ids)
        list.push_back(QString::fromStdString(id));
    m_session.emitRecogChrome(kind, list);
}

bool TabletCanvasItem::tryPaintCameraBlit(bool /*sharp*/, epaper::rasterprobe::CamDelta /*cam*/,
                                          const epaper::rasterprobe::CameraPlan &plan, int *warpMs,
                                          int *renderMs)
{
    if (warpMs)
        *warpMs = 0;
    if (renderMs)
        *renderMs = 0;
    if (m_image.isNull() || !m_rasterCam.valid)
        return false;
    if (plan.paint != epaper::rasterprobe::CameraPaint::BlitPan
        && plan.paint != epaper::rasterprobe::CameraPaint::BlitScale)
        return false;
    QElapsedTimer blitClock;
    blitClock.start();
    QImage src = m_image;
    {
        epaper::inkpath::Span span("rasterize.blit");
        if (!blitOnto(&m_image, src, m_rasterCam))
            return false;
    }
    if (renderMs)
        *renderMs = int(blitClock.elapsed());
    return true;
}

/** Paint vectors into m_image. Camera pan/zoom blit the previous bitmap. */
void TabletCanvasItem::rasterizeVectors(bool sharp, const QRectF &panelDirty)
{
    epaper::UiStallSection stall("rasterizeVectors");
    QElapsedTimer clock;
    clock.start();
    if (!m_paintsInk)
        return;
    ensureImage();
    if (m_image.isNull())
        return;

    syncFramePanelSize();

    const epaper::rasterprobe::CamBox nextCam = currentCamBox();
    const bool orientChanged = m_rasterOrient != m_session.frame.orientation;
    const epaper::rasterprobe::CamDelta cam = epaper::rasterprobe::classifyCamera(
        m_rasterCam, nextCam, m_session.frame.panelW, orientChanged);

    int warpMs = 0;
    int renderMs = 0;
    bool blit = false;
    bool useDirty = false;
    QRectF dirty = panelDirty;
    const QRect img = m_image.rect();

    if (m_rasterWhy == epaper::rasterprobe::Why::Camera) {
        const auto plan = epaper::rasterprobe::planCameraPaint(cam.kind, sharp);
        if (plan.paint == epaper::rasterprobe::CameraPaint::Skip) {
            if (m_cameraNeedsSharp)
                submitCameraSharp();
            return;
        }
        if (plan.paint == epaper::rasterprobe::CameraPaint::BlitPan
            || plan.paint == epaper::rasterprobe::CameraPaint::BlitScale) {
            if (tryPaintCameraBlit(sharp, cam, plan, &warpMs, &renderMs)) {
                blit = true;
                m_cameraNeedsSharp = true;
            } else {
                submitCameraSharp();
                return;
            }
        }
    }

    if (!blit) {
        using epaper::document::refreshAllConnectorWarps;
        {
            epaper::inkpath::Span span("rasterize.warp");
            refreshAllConnectorWarps(m_session.document);
        }
        warpMs = int(clock.restart());

        epaper::render::FrameProjector proj;
        proj.frame = &m_session.frame;

        const double imgArea = double(img.width()) * double(img.height());
        useDirty = dirty.isValid() && !dirty.isEmpty()
            && (dirty.width() * dirty.height() <= 0.5 * imgArea);

        epaper::render::RenderRequest req;
        req.sharp = sharp;
        if (useDirty) {
            req.mode = epaper::render::RenderRequest::BufferMode::InPlaceDirty;
            dirty = padRasterDirty(dirty);
            req.dirtyPanelX = dirty.x();
            req.dirtyPanelY = dirty.y();
            req.dirtyPanelW = dirty.width();
            req.dirtyPanelH = dirty.height();
            req.worldClip = epaper::render::intersectWorldAabb(
                proj.drawingWorldClip(), panelRectToWorldClip(dirty));
        } else {
            req.mode = epaper::render::RenderRequest::BufferMode::FullClear;
            req.worldClip = proj.drawingWorldClip();
        }
        for (const auto &id : m_session.liveManipSuppressIds())
            req.suppressIds.insert(id);

        clock.restart();
        {
            epaper::inkpath::Span span("rasterize.render");
            epaper::render::QImagePixelSink sink(&m_image);
            m_renderer.render(m_session.document, proj, req, sink);
        }
        renderMs = int(clock.restart());
        m_cameraNeedsSharp = false;
    }

    m_refreshClock.restart();
    int updateMs = 0;
    {
        epaper::inkpath::Span span("rasterize.update");
        clock.restart();
        update(blit || !useDirty ? img : dirty.toAlignedRect());
        updateMs = int(clock.restart());
    }
    if (!blit && sharp && qEnvironmentVariableIsSet("RM_EP_SWAP")) {
        if (auto *win = window()) {
            const QRect scene = mapRectToScene(QRectF(useDirty ? dirty : QRectF(img)))
                                    .toAlignedRect();
            QObject::connect(
                win,
                &QQuickWindow::afterRendering,
                this,
                [scene]() { EpaperBridge::instance()->swapPen(scene); },
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
            win->update();
        }
    }

    epaper::render::LastPaintStats st;
    if (m_renderer.algorithm())
        st = m_renderer.algorithm()->lastPaintStats();
    int samples = 0;
    m_session.document.forEachPaintNode([&](const epaper::document::DocNode &n) {
        if (n.kind == epaper::document::NodeKind::Ink)
            samples += int(n.samples.size());
    });

    epaper::rasterprobe::Record rec;
    rec.why = m_rasterWhy;
    rec.cam = cam;
    rec.inplace = useDirty;
    rec.sharp = sharp;
    rec.blit = blit;
    rec.totalMs = warpMs + renderMs + updateMs;
    rec.warpMs = warpMs;
    rec.renderMs = renderMs;
    rec.updateMs = updateMs;
    rec.ink = m_session.document.inkCount();
    rec.nodes = m_session.document.nodeCount();
    rec.samples = samples;
    rec.visits = int(st.visits);
    rec.skipped = int(st.skipped);
    rec.polylines = int(st.polylines);
    rec.pts = int(st.pts);
    finishRasterize(rec, nextCam);
    if (blit)
        submitCameraSharp();
}

/**
 * =================================================================================================
 * Connector ink rendering
 *
 * Warped connector panel helpers for Tool via Surface API.
 * =================================================================================================
 */


/**
 * =================================================================================================
 * Recognizer feedback
 *
 * [D13] Blink / membership stamp live on ToolCanvas NodeEmphasis.
 * =================================================================================================
 */

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

bool TabletCanvasItem::togglePenEraser()
{
    return m_session.togglePenEraser();
}

bool TabletCanvasItem::beginTempErase()
{
    return m_session.beginTempErase();
}

bool TabletCanvasItem::endTempErase()
{
    return m_session.endTempErase();
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

void TabletCanvasItem::setErasePointerActive(bool on)
{
    if (m_erasePointerActive == on)
        return;
    m_erasePointerActive = on;
    if (!on && (m_rasterizeDeferredSharp || m_rasterizePending || m_rasterizeSharp))
        scheduleVectorRasterize(m_rasterizeDeferredSharp || m_rasterizeSharp);
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
    if (m_rasterWhy == epaper::rasterprobe::Why::Unknown)
        m_rasterWhy = epaper::rasterprobe::Why::Camera;
    scheduleVectorRasterize(sharp);
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
    // Soft path already ran via cameraChanged blit. Unchanged camera must not
    // FullClear (that was cam=none). Settle-sharp uses the camera plan (pan strips /
    // zoom vector) — never a second anonymous FullClear.
    if (settle) {
        m_rasterWhy = epaper::rasterprobe::Why::Camera;
        scheduleVectorRasterize(true);
    }
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
    m_rasterWhy = epaper::rasterprobe::Why::Camera;
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
        && m_oneWay.epochLive()) {
        m_rasterWhy = epaper::rasterprobe::Why::DocLoad;
        scheduleVectorRasterize(true);
    }
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
    // UI-EP-04 + ADR-0021: six exclusive tools + 12 px publish + 2 toggles + Undo/Redo.
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

