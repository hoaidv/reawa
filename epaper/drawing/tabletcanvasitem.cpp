#include "tabletcanvasitem.h"
#include "debug/ui_stall.hpp"
#include "regionsync/strokesync.h"
#include "epaperbridge.h"
#include "debug/latency_probe.hpp"
#include "document/connector_warp.hpp"
#include "document/recognizer_dispatch.hpp"
#include "document/recognize_enclose.hpp"
#include "document/membership.hpp"
#include "document/surround_create.hpp"
#include "document/manipulate.hpp"
#include "document/capability.hpp"
#include "debug/debug_log_format.hpp"
#include "toolcanvasitem.h"
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
#include <QLineF>
#include <QDebug>
#include <QSizeF>
#include <algorithm>
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
QRectF modeChipRect(const QRectF &box)
{
    constexpr qreal w = 120.0;
    constexpr qreal h = 36.0;
    constexpr qreal gap = 32.0; // handle half (14) + pad
    return QRectF(box.center().x() - w * 0.5, box.bottom() + gap, w, h);
}

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

TabletCanvasItem::TabletCanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_sync(new StrokeSync(this))
    , m_oneWay(m_document)
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
        m_follow.onReconnect();
        emit followChanged();
        flushOneWayWire();
    });
    connect(m_sync, &StrokeSync::socketDisconnected, this, [this]() {
        m_oneWay.onLinkDown();
        // @implements [SRS-EP-49] disconnect forces follow none
        m_follow.onDisconnect();
        m_followDirection = QStringLiteral("none");
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
}

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

qreal TabletCanvasItem::ingestPanelHeight() const
{
    qreal h = height();
    if (h < 2.0 && window())
        h = window()->height();
    return qMax<qreal>(1.0, h);
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
        // @implements [SRS-EP-04] selection mode never inks (STORY-EP-007)
        // @implements [SRS-EP-07] Selection armed → no stroke, no Ink node
        if (m_stroke.active)
            appendPoint(canvasPos, bounded);
        else if (selectionGestureActive())
            updateSelectionGesture(canvasPos);
        break;
    case QEvent::TabletRelease:
    case QEvent::MouseButtonRelease:
        if (m_stroke.active)
            endStroke();
        else if (selectionGestureActive())
            endSelectionGesture();
        break;
    default:
        break;
    }
}

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
        else if (selectionGestureActive())
            updateSelectionGesture(canvasPos);
        break;
    case QEvent::TabletRelease:
        if (m_stroke.active)
            endStroke();
        else if (selectionGestureActive())
            endSelectionGesture();
        break;
    default:
        break;
    }
}

int TabletCanvasItem::handleIndexAtPanel(const PanelPt &panel, double hitDu) const
{
    // Same 8 panel points as Main.qml ResizeKnob; canvas hit-test owns the knobs.
    // @implements [SRS-EP-11] handle hit 56 du
    if (m_handleCount != 8)
        return -1;
    const QRectF r = m_selectionBoundsRect;
    if (r.width() <= 0.0 || r.height() <= 0.0)
        return -1;
    const PanelPt pts[8] = {
        {r.left(), r.top()},
        {r.center().x(), r.top()},
        {r.right(), r.top()},
        {r.right(), r.center().y()},
        {r.right(), r.bottom()},
        {r.center().x(), r.bottom()},
        {r.left(), r.bottom()},
        {r.left(), r.center().y()},
    };
    const qreal half = hitDu * 0.5;
    for (int i = 0; i < 8; ++i) {
        if (qAbs(panel.x() - pts[i].x()) <= half && qAbs(panel.y() - pts[i].y()) <= half)
            return i;
    }
    return -1;
}

bool TabletCanvasItem::tryBeginHandleAtPanel(const PanelPt &panel, double hitDu)
{
    const int idx = handleIndexAtPanel(panel, hitDu);
    if (idx < 0)
        return false;
    beginHandleDrag(idx, panelToWorld(panel));
    return true;
}

void TabletCanvasItem::applyContactPress(const PanelPt &canvasPos, const IngestChannels &ch)
{
    if (tryBeginHandleAtPanel(canvasPos, epaper::document::kHandleHitDu))
        return;
    if (isSelectionTool())
        beginSelectionGesture(canvasPos);
    else
        beginStroke(canvasPos, ch);
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
    if ((m_selGesture == SelGesture::Move || m_selGesture == SelGesture::Resize)
        && !m_originPanelRect.isEmpty()) {
        // Punch only the original box — not origin∪live (that wipes a vertical strip).
        painter->fillRect(m_originPanelRect, Qt::white);
        // Connector origin hole: thick white stroke along the rest-pose polyline.
        // fillRect(AABB) painted a white slab on a diagonal/curve (dirty during drag).
        // Box AABB ≈ the box, so a rect punch stays clean. [SRS-EP-18] [CHL-0018]
        if (!m_originConnStrokes.isEmpty()) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->setBrush(Qt::NoBrush);
            for (const OriginConnStroke &st : m_originConnStrokes) {
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

    const qreal pad = lineW * 0.5 + 8.0;
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

void TabletCanvasItem::applyStrokeIntent(const epaper::strokecapture::StrokeResult &r)
{
    using epaper::strokecapture::StrokeIntent;
    using epaper::strokecapture::has;
    if (has(r.intent, StrokeIntent::CancelSettle))
        ++m_settleFollowUpToken;
    if (has(r.intent, StrokeIntent::BeginGesture))
        m_document.beginGesture();
    if (has(r.intent, StrokeIntent::LatchChip)) {
        m_chip.latchPenDown();
        const QString latch = QString::fromStdString(m_chip.dispatchTuple());
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
        m_document.abortGesture();
    if (has(r.intent, StrokeIntent::NotifyHistory))
        notifyHistory();
    if (has(r.intent, StrokeIntent::FlushWire))
        flushOneWayWire();
    if (has(r.intent, StrokeIntent::ChipPenUp))
        m_chip.penUp();
}

void TabletCanvasItem::beginStroke(const PanelPt &canvasPos, const IngestChannels &ch)
{
    m_stroke.setPanelHeight(double(ingestPanelHeight()));
    m_pendingDirty = QRectF();
    m_flushClock.restart();
    applyStrokeIntent(m_stroke.begin(canvasPos.x(), canvasPos.y(), toChannels(ch)));
}

void TabletCanvasItem::appendPoint(const PanelPt &canvasPos, const IngestChannels &ch)
{
    if (!m_stroke.active || m_stroke.current.empty()) {
        beginStroke(canvasPos, ch);
        return;
    }
    m_stroke.setPanelHeight(double(ingestPanelHeight()));
    const bool flushDue = m_flushClock.elapsed() >= kFlushIntervalMs;
    applyStrokeIntent(
        m_stroke.append(canvasPos.x(), canvasPos.y(), toChannels(ch), flushDue));
}

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
                      .arg(m_document.inkCount());
    emit debugChanged();
}

/** @implements [SRS-EP-07] finished stroke → append_ink at pen-up */
/** @implements [SRS-EP-10] ADR-0022 closure-first dispatch (one [recog] line) */
/** @implements [SRS-EP-15] [ink] / [enclose] log sources after ingest */
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

    const std::string tool = m_chip.latchedTool;
    if (tool == "sel_rect" || tool == "sel_freeform")
        return;

    RecogLatch latch;
    latch.inkBox = m_chip.latchedInkBox;
    latch.connector = m_chip.latchedConnector;
    const RecogDispatchResult d = dispatchFinishedStroke(m_document, stroke, map, latch);
    m_ingestNs.push_back(d.ns);
    // Recognizers walk the whole document per stroke; name it when it hitches.
    if (d.ns > 100'000'000LL) {
        qInfo().noquote() << QStringLiteral("[perf] recog ms=%1 samples=%2 nodes=%3 ink=%4")
                                 .arg(d.ns / 1'000'000LL)
                                 .arg(stroke.samples.size())
                                 .arg(m_document.nodeCount())
                                 .arg(m_document.inkCount());
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
        if (const DocNode *sg = m_document.find(d.enclose.smartGroupId)) {
            std::vector<std::string> ids;
            collectSmartGroupInkIds(*sg, false, &ids);
            beginRecogWidthBlink(ids);
        }
        clearMembershipHighlight();
    } else if (d.outcome == RecogOutcome::Membership) {
        bool highlightChanged = false;
        if (const DocNode *sg = m_document.find(d.membership.smartGroupId)) {
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
        if (const DocNode *sg = m_document.find(d.connector.fromId))
            collectSmartGroupInkIds(*sg, false, &ids);
        if (const DocNode *sg = m_document.find(d.connector.toId))
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

void TabletCanvasItem::syncBegin()
{
    // @implements [SRS-EP-08] stroke_begin without intent
    m_oneWay.beginPreviewStroke(m_stroke.activeStrokeId);
    flushOneWayWire();
}

void TabletCanvasItem::syncPoint(const Point &pt)
{
    // @implements [SRS-EP-02] live preview in world — same space as append_ink
    ensureLocalDrawingRegion();
    const WorldPt world = panelToWorld(pt.pos);
    m_oneWay.previewStrokePoint(m_stroke.activeStrokeId, world.x, world.y, pt.pressure);
    flushOneWayWire();
}

void TabletCanvasItem::syncEnd()
{
    m_oneWay.endPreviewStroke(m_stroke.activeStrokeId);
    flushOneWayWire();
}

void TabletCanvasItem::flushOneWayWire()
{
    m_oneWay.onLocalCommit();
    if (!m_sync->isConnected())
        return;
    for (const std::string &line : m_oneWay.takeOutbound())
        m_sync->sendLine(QByteArray::fromStdString(line));
}

/** @fix [STORY-IN-032] live pose to Infini without a committed doc_change seq */
void TabletCanvasItem::sendManipPreviewToInfini()
{
    if (!m_sync || !m_sync->isConnected())
        return;
    if (m_drag.nodeId().isEmpty())
        return;
    QJsonObject xf;
    xf.insert(QStringLiteral("x"), m_drag.liveT().x);
    xf.insert(QStringLiteral("y"), m_drag.liveT().y);
    xf.insert(QStringLiteral("rotation"), 0);
    xf.insert(QStringLiteral("scaleX"), m_drag.liveT().scaleX);
    xf.insert(QStringLiteral("scaleY"), m_drag.liveT().scaleY);
    QJsonObject o;
    o.insert(QStringLiteral("type"), QStringLiteral("manip_preview"));
    o.insert(QStringLiteral("id"), m_drag.nodeId());
    o.insert(QStringLiteral("transform"), xf);
    if (m_selGesture == SelGesture::Resize) {
        QJsonObject b;
        b.insert(QStringLiteral("x"), m_drag.liveB().x);
        b.insert(QStringLiteral("y"), m_drag.liveB().y);
        b.insert(QStringLiteral("width"), m_drag.liveB().width);
        b.insert(QStringLiteral("height"), m_drag.liveB().height);
        o.insert(QStringLiteral("bounds"), b);
    }
    m_sync->sendLine(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

void TabletCanvasItem::onHostMessage(const QJsonObject &obj)
{
    epaper::UiStallSection stall("onHostMessage");
    const QString inboundType = obj.value(QStringLiteral("type")).toString();
    if (inboundType == QLatin1String("viewport_follow")) {
        const QByteArray raw = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        const auto msg = epaper::follow::parseViewportFollowLine(
            std::string(raw.constData(), static_cast<size_t>(raw.size())));
        if (m_follow.adoptInbound(msg)) {
            m_followDirection = QString::fromLatin1(epaper::handtouch::followId(m_follow.direction));
            emit followChanged();
            if (m_follow.epaperFollowOn())
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
            qInfo() << "[sync] ignore inbound viewport follow=" << m_followDirection;
    }
    if (m_document.selectionId() == std::nullopt) {
        m_selectedIds.clear();
        m_selectedPickableId.clear();
    }
    flushOneWayWire();
    if (obj.value(QStringLiteral("type")).toString() == QLatin1String("doc_load")
        && m_oneWay.epochLive())
        scheduleVectorRasterize(true);
}

void TabletCanvasItem::applyViewport(const QJsonObject &obj)
{
    m_viewportSeq = obj.value(QStringLiteral("seq")).toInt(m_viewportSeq);
    using epaper::canvasframe::FrameIntent;
    FrameIntent intent = FrameIntent::None;
    const QString orient = obj.value(QStringLiteral("orientation")).toString();
    if (!orient.isEmpty())
        intent |= m_frame.setOrientation(orient.toStdString());

    const QJsonObject dr = obj.value(QStringLiteral("drawingRegion")).toObject();
    if (!dr.isEmpty())
        intent |= m_frame.applyDrawingRegion(aabbFromJson(dr), true);

    applyFrameIntent(intent);
    const bool settle = obj.value(QStringLiteral("settle")).toBool(false);
    qInfo() << "[sync] viewport seq" << m_viewportSeq << "orientation"
            << QString::fromStdString(m_frame.orientation) << "settle" << settle << "ink"
            << m_document.inkCount();
    scheduleVectorRasterize(settle);
    refreshSelectionChrome();
}

void TabletCanvasItem::setToolMode(const QString &mode)
{
    if (!m_chip.setExclusive(mode.toStdString()))
        return;
    const bool hadHighlight = !m_highlightInkIds.empty();
    m_toolMode = QString::fromStdString(m_chip.exclusive);
    clearMembershipHighlight();
    if (hadHighlight && !m_stroke.active)
        scheduleVectorRasterize(true);
    emit toolModeChanged();
    m_debugInfo = QStringLiteral("tool=%1").arg(m_toolMode);
    emit debugChanged();
    // @fix residual knobs / scale chip when leaving sel_* (QML chrome is not ToolCanvas)
    refreshSelectionChrome();
}

void TabletCanvasItem::armTool(const QString &mode)
{
    setToolMode(mode);
}

void TabletCanvasItem::toggleRecogInkBox()
{
    if (!m_chip.flipRecogInkBox())
        return;
    emit recogChanged();
}

void TabletCanvasItem::toggleRecogConnector()
{
    if (!m_chip.flipRecogConnector())
        return;
    emit recogChanged();
}

void TabletCanvasItem::tapFollowToggle()
{
    m_follow.connected = m_sync && m_sync->isConnected();
    m_follow.exclusiveTool = m_toolMode.toStdString();
    const auto r = m_follow.tapToggle();
    m_followDirection = QString::fromLatin1(epaper::handtouch::followId(m_follow.direction));
    emit followChanged();
    flushFollowOutbound();
    if (r.appliedInfiniViewport)
        applyFollowCamera();
}

void TabletCanvasItem::flushFollowOutbound()
{
    if (!m_sync || !m_sync->isConnected()) {
        m_follow.outbound.clear();
        return;
    }
    for (const std::string &line : m_follow.outbound)
        m_sync->sendLine(QByteArray::fromStdString(line));
    m_follow.outbound.clear();
}

void TabletCanvasItem::applyFollowCamera()
{
    if (!m_follow.mapApplied && !m_follow.hasInfiniViewport)
        return;
    m_follow.applyInfiniViewportIfFollowing();
    if (m_follow.direction != epaper::handtouch::FollowDirection::InfiniToEpaper)
        return;
    applyFrameIntent(m_frame.applyDrawingRegion(m_follow.localCamera, true));
    scheduleVectorRasterize(true);
}

void TabletCanvasItem::cacheInfiniViewport(const QJsonObject &obj)
{
    const QJsonObject dr = obj.value(QStringLiteral("drawingRegion")).toObject();
    if (dr.isEmpty())
        return;
    m_follow.cacheInfiniViewport(aabbFromJson(dr));
}

epaper::handtouch::FollowDirection TabletCanvasItem::followEnum() const
{
    return epaper::handtouch::parseFollow(m_followDirection.toStdString());
}

bool TabletCanvasItem::fingerHitsBox(const PanelPt &canvasPos) const
{
    using namespace epaper::document;
    const WorldPt world = panelToWorld(canvasPos);
    const QString id = hitLocalSmartGroup(world);
    if (id.isEmpty())
        return false;
    const DocNode *n = m_document.find(id.toStdString());
    SmartBounds wb;
    if (!n || !boundsOf(*n, wb))
        return false;
    return lodOkPanel(wb);
}

void TabletCanvasItem::ensureLocalDrawingRegion()
{
    syncFramePanelSize();
    applyFrameIntent(m_frame.ensureLocalDrawingRegion());
}

void TabletCanvasItem::applyLocalFingerPan(const PanelPt &canvasPos)
{
    using epaper::handtouch::mapUvToWorld;
    using epaper::handtouch::panKeepWorldUnderFinger;
    ensureLocalDrawingRegion();
    // The contact is mapped through the *origin* region, not the live one.
    const FrameUv uv = panelToFrameUv(canvasPos);
    double nowX = 0;
    double nowY = 0;
    mapUvToWorld(m_fingerPanOrigin.box(), uv.u, uv.v, &nowX, &nowY);
    applyFrameIntent(m_frame.applyDrawingRegion(
        panKeepWorldUnderFinger(m_fingerPanOrigin.box(), m_fingerDownWorld.x, m_fingerDownWorld.y,
                                nowX, nowY),
        false));
}

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
    dr.insert(QStringLiteral("minX"), m_frame.drawingRegion.minX);
    dr.insert(QStringLiteral("minY"), m_frame.drawingRegion.minY);
    dr.insert(QStringLiteral("maxX"), m_frame.drawingRegion.maxX);
    dr.insert(QStringLiteral("maxY"), m_frame.drawingRegion.maxY);
    double sx = 1.0;
    double sy = 1.0;
    const double iw = qMax(1.0, double(width()));
    const double ih = qMax(1.0, double(height()));
    uniformScaleOf({0, 0, iw, ih}, m_frame.drawingRegion.box(), &sx, &sy);
    QJsonObject o;
    o.insert(QStringLiteral("type"), QStringLiteral("viewport"));
    o.insert(QStringLiteral("source"), QStringLiteral("epaper"));
    o.insert(QStringLiteral("seq"), ++m_viewportSeq);
    o.insert(QStringLiteral("orientation"), QString::fromStdString(m_frame.orientation));
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

bool TabletCanvasItem::beginFingerTouch(const PanelPt &canvasPos)
{
    using namespace epaper::handtouch;
    if (!m_handTouchArmed)
        return false;
    m_fingerGesture = FingerGesture::None;
    m_fingerDownPanel = canvasPos;
    const bool knob = handleIndexAtPanel(canvasPos, kFingerHandleHitDu) >= 0;
    const HitKind hit = classifyHit(false, knob, fingerHitsBox(canvasPos));
    const FingerAction act = actionOnDown(hit);
    qInfo().noquote() << QStringLiteral("[hand] down (%1,%2) knob=%3 box=%4")
                             .arg(int(canvasPos.x()))
                             .arg(int(canvasPos.y()))
                             .arg(knob ? 1 : 0)
                             .arg(hit == HitKind::Box ? 1 : 0);

    if (act == FingerAction::Resize) {
        // @implements [SRS-EP-21] knob wins over box-move
        m_fingerGesture = FingerGesture::Resize;
        tryBeginHandleAtPanel(canvasPos, kFingerHandleHitDu);
        return true;
    }
    if (act == FingerAction::SelectMove) {
        // @implements [SRS-EP-23] finger-down on box → sel_freeform
        armTool(QStringLiteral("sel_freeform"));
        m_fingerGesture = FingerGesture::Move;
        beginSelectionGesture(canvasPos);
        return true;
    }
    m_fingerGesture = FingerGesture::EmptyPending;
    ensureLocalDrawingRegion();
    m_fingerPanOrigin = m_frame.drawingRegion;
    m_fingerDownWorld = panelToWorld(canvasPos);
    return true;
}

void TabletCanvasItem::updateFingerTouch(const PanelPt &canvasPos, int fingerCount)
{
    using namespace epaper::handtouch;
    if (m_fingerGesture == FingerGesture::None || m_fingerGesture == FingerGesture::Chip
        || m_fingerGesture == FingerGesture::TwoFinger)
        return;
    if (m_fingerGesture == FingerGesture::Move || m_fingerGesture == FingerGesture::Resize) {
        updateSelectionGesture(canvasPos);
        return;
    }
    if (fingerCount >= 2)
        return;
    const double dx = canvasPos.x() - m_fingerDownPanel.x();
    const double dy = canvasPos.y() - m_fingerDownPanel.y();
    if (m_fingerGesture == FingerGesture::EmptyPending) {
        if (actionOnEmptyMove(travelDu(dx, dy)) != FingerAction::LocalPan)
            return;
        // @implements [SRS-EP-21] no pan while following Infini
        const LocalNav nav = onLocalNav(followEnum());
        if (nav.blocked)
            return;
        m_fingerGesture = FingerGesture::EmptyPan;
        m_fingerPanClock.invalidate();
        // Pan origin stays at touch-down: panKeepWorldUnderFinger is the contract,
        // so the content catches up with the finger the moment palm travel clears.
        qInfo().noquote() << QStringLiteral("[hand] pan promote at (%1,%2)")
                                 .arg(int(canvasPos.x()))
                                 .arg(int(canvasPos.y()));
    }
    if (m_fingerGesture != FingerGesture::EmptyPan)
        return;
    applyLocalFingerPan(canvasPos);
    if (!m_fingerPanClock.isValid() || m_fingerPanClock.elapsed() >= kSelectionGhostMinIntervalMs) {
        m_fingerPanClock.restart();
        maybePublishLocalViewport(false);
        scheduleVectorRasterize(false);
        refreshSelectionChrome();
    }
}

void TabletCanvasItem::endFingerTouch(const PanelPt &canvasPos)
{
    const FingerGesture g = m_fingerGesture;
    m_fingerGesture = FingerGesture::None;
    // Travel here against kPalmTravelDu (178) says whether a swipe that felt long
    // enough to pan actually moved the centroid that far.
    qInfo().noquote() << QStringLiteral("[hand] up gesture=%1 travel=%2")
                             .arg(int(g))
                             .arg(int(epaper::handtouch::travelDu(
                                 canvasPos.x() - m_fingerDownPanel.x(),
                                 canvasPos.y() - m_fingerDownPanel.y())));
   
    if (g == FingerGesture::Move || g == FingerGesture::Resize) {
        endSelectionGesture();
        return;
    }
    if (g == FingerGesture::EmptyPan) {
        applyLocalFingerPan(canvasPos);
        maybePublishLocalViewport(true);
        scheduleVectorRasterize(true);
        refreshSelectionChrome();
        return;
    }
    if (g == FingerGesture::TwoFinger) {
        applyLocalTwoFinger(m_twoA, m_twoB);
        maybePublishLocalViewport(true);
        scheduleVectorRasterize(true);
        refreshSelectionChrome();
        return;
    }
    if (g == FingerGesture::EmptyPending) {
        // @implements [SRS-EP-21] empty tap deselects
        using namespace epaper::handtouch;
        const double dx = canvasPos.x() - m_fingerDownPanel.x();
        const double dy = canvasPos.y() - m_fingerDownPanel.y();
        if (emptyTapClearsSelection(travelDu(dx, dy))) {
            clearSelection();
            m_selGesture = SelGesture::None;
            refreshSelectionChrome();
        }
        return;
    }
}

/**
 * Second contact arrived mid-manip: revert the node, never commit it. Mirrors
 * commitLiveManip's no-move branch, which is the only in-tree revert path.
 * @implements [SRS-EP-24] two-finger outranks a one-finger manip
 */
void TabletCanvasItem::abortFingerManip()
{
    using namespace epaper::document;
    if (m_drag.active()) {
        m_document.applyLiveSmartGeometry(m_drag.nodeId().toStdString(), m_drag.originT(),
                                          m_drag.originB());
        refreshConnectorsBoundTo(m_document, m_drag.nodeId().toStdString());
        m_document.abortGesture();
        m_selectedPickableId = m_drag.nodeId();
        m_drag.reset();
        refreshAllConnectorWarps(m_document);
    }
    m_selGesture = SelGesture::None;
    const QRectF punch = m_originPanelRect.united(m_selectionChromeDirty).united(m_originConnPunch);
    m_originPanelRect = QRectF();
    m_originConnPunch = QRectF();
    m_originConnStrokes.clear();
    m_liveDirtyPrev = QRectF();
    m_fingerGesture = FingerGesture::None;
    if (!punch.isEmpty())
        update(punch.toAlignedRect());
    refreshSelectionChrome();
    syncToolCanvasPresence();
}

epaper::handtouch::TwoFingerContacts TabletCanvasItem::uvPair(const PanelPt &a, const PanelPt &b) const
{
    const FrameUv ua = panelToFrameUv(a);
    const FrameUv ub = panelToFrameUv(b);
    return epaper::handtouch::TwoFingerContacts{ua.u, ua.v, ub.u, ub.v};
}

void TabletCanvasItem::applyLocalTwoFinger(const PanelPt &a, const PanelPt &b)
{
    using epaper::handtouch::applyTwoFingerPanPinch;
    ensureLocalDrawingRegion();
    m_twoA = a;
    m_twoB = b;
    applyFrameIntent(m_frame.applyDrawingRegion(
        applyTwoFingerPanPinch(m_fingerPanOrigin.box(), m_twoOriginContacts, uvPair(a, b)), false));
}

bool TabletCanvasItem::beginTwoFingerTouch(const PanelPt &a, const PanelPt &b)
{
    using namespace epaper::handtouch;
    if (m_fingerGesture == FingerGesture::Move || m_fingerGesture == FingerGesture::Resize
        || m_fingerGesture == FingerGesture::Chip)
        return false;
    ensureLocalDrawingRegion();
    // @implements [SRS-EP-24] no pinch while following Infini
    const LocalNav nav = onLocalNav(followEnum());
    if (nav.blocked)
        return false;
    m_fingerPanOrigin = m_frame.drawingRegion;
    m_twoOriginContacts = uvPair(a, b);
    m_fingerGesture = FingerGesture::TwoFinger;
    m_fingerPanClock.invalidate();
    applyLocalTwoFinger(a, b);
    maybePublishLocalViewport(false);
    scheduleVectorRasterize(false);
    refreshSelectionChrome();
    return true;
}

void TabletCanvasItem::updateTwoFingerTouch(const PanelPt &a, const PanelPt &b)
{
    if (m_fingerGesture != FingerGesture::TwoFinger)
        return;
    applyLocalTwoFinger(a, b);
    if (!m_fingerPanClock.isValid() || m_fingerPanClock.elapsed() >= kSelectionGhostMinIntervalMs) {
        m_fingerPanClock.restart();
        maybePublishLocalViewport(false);
        scheduleVectorRasterize(false);
        refreshSelectionChrome();
    }
}

void TabletCanvasItem::endTwoFingerTouch()
{
    if (m_fingerGesture != FingerGesture::TwoFinger)
        return;
    m_fingerGesture = FingerGesture::None;
    // The finger still on glass would otherwise re-arm as a fresh one-finger
    // gesture the moment the pinch handler lets go of it.
    m_fingerLockedUntilLift = true;
    applyLocalTwoFinger(m_twoA, m_twoB);
    maybePublishLocalViewport(true);
    scheduleVectorRasterize(true);
    refreshSelectionChrome();
}

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

void TabletCanvasItem::toggleHandTouch()
{
    // @implements [SRS-EP-22] btn.hand_touch kill-switch
    m_handTouchArmed = !m_handTouchArmed;
    if (!m_handTouchArmed)
        cancelHandTouch();
    emit handTouchArmedChanged();
    qInfo().noquote() << (m_handTouchArmed ? QStringLiteral("[hand] toggle on")
                                           : QStringLiteral("[hand] toggle off"));
}

void TabletCanvasItem::cancelHandTouch()
{
    // Full hand-touch reset, lock included: whoever cancels must not have to know
    // that a lifted-contact latch exists.
    m_fingerLockedUntilLift = false;
    if (m_fingerGesture == FingerGesture::TwoFinger) {
        endTwoFingerTouch(); // settles the viewport it was already panning
        return;
    }
    const FingerGesture g = m_fingerGesture;
    m_fingerGesture = FingerGesture::None;
    if (g == FingerGesture::Move || g == FingerGesture::Resize || m_drag.active())
        endSelectionGesture();
}

void TabletCanvasItem::requestUndo()
{
    applyHistoryRestore(true);
}

void TabletCanvasItem::requestRedo()
{
    applyHistoryRestore(false);
}

void TabletCanvasItem::applyHistoryRestore(bool isUndo)
{
    using namespace epaper::document;
    const UndoResult r = isUndo ? m_document.undo() : m_document.redo();
    (void)r;
    pruneSelectionAfterHistory();
    clearMembershipHighlight();
    notifyHistory();
    scheduleVectorRasterize(true);
    refreshSelectionChrome();
    flushOneWayWire();
}

void TabletCanvasItem::pruneSelectionAfterHistory()
{
    QStringList keep;
    for (const QString &id : m_selectedIds) {
        if (m_document.find(id.toStdString()))
            keep.append(id);
    }
    m_selectedIds = keep;
    if (!m_selectedPickableId.isEmpty()
        && !m_document.find(m_selectedPickableId.toStdString())) {
        m_selectedPickableId.clear();
        m_drag.clearNodeId();
    }
}

void TabletCanvasItem::notifyHistory()
{
    emit historyChanged();
}

void TabletCanvasItem::updateToolChipRect()
{
    // UI-EP-04 + ADR-0021: 3 exclusive tools + 12 px publish + 2 toggles + Undo/Redo.
    // @implements [SRS-EP-05] floating ToolChip hit bounds (64×64 tiles, CHL-0019)
    const qreal chipH = epaper::toolchip::kHeight;
    const qreal chipW = epaper::toolchip::chipWidth();
    const qreal inset = 8.0;
    qreal top = inset;
    if (m_frame.orientation == "gutOnTop")
        top = height() - inset - chipH;
    const qreal left = (width() - chipW) * 0.5;
    const QRectF next(left, top, chipW, chipH);
    if (next != m_toolChipRect) {
        m_toolChipRect = next;
        emit toolChipRectChanged();
    }
    const bool gutOnTop = m_frame.orientation == "gutOnTop";
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

bool TabletCanvasItem::isSelectionTool() const
{
    return m_toolMode == QLatin1String("sel_rect") || m_toolMode == QLatin1String("sel_freeform");
}

void TabletCanvasItem::clearSelection()
{
    m_selectedIds.clear();
    m_selectedPickableId.clear();
    m_drag.clearNodeId();
}

void TabletCanvasItem::setSelection(const std::vector<std::string> &ids)
{
    m_selectedIds.clear();
    for (const auto &id : ids)
        m_selectedIds.append(QString::fromStdString(id));
    if (!m_selectedIds.isEmpty())
        m_selectedPickableId = m_selectedIds.first();
    else
        m_selectedPickableId.clear();
}

QString TabletCanvasItem::hitLocalSmartGroup(WorldPt world) const
{
    using namespace epaper::document;
    std::vector<const DocNode *> pick;
    collectPickable(m_document.rootChildren, pick);
    for (int i = int(pick.size()) - 1; i >= 0; --i) {
        const DocNode *n = pick[size_t(i)];
        if (!n || n->kind != NodeKind::SmartGroup)
            continue;
        SmartBounds b;
        if (!nodeWorldAabb(*n, b))
            continue;
        if (world.x >= b.x && world.x <= b.x + b.width && world.y >= b.y
            && world.y <= b.y + b.height)
            return QString::fromStdString(n->id);
    }
    return {};
}

void TabletCanvasItem::refreshSelectionChrome()
{
    using namespace epaper::document;
    m_encloseRefuseReason.clear();
    SmartBounds unionB;
    std::vector<std::string> ids;
    for (const QString &id : m_selectedIds)
        ids.push_back(id.toStdString());
    QRectF bounds;
    if (!ids.empty() && unionAabbOfIds(m_document, ids, unionB)) {
        const PanelPt tl = worldToPanel(unionB.x, unionB.y);
        const PanelPt br = worldToPanel(unionB.x + unionB.width, unionB.y + unionB.height);
        bounds = QRectF(tl, br).normalized();
    }
    m_encloseVisible = isSelectionTool() && m_selectedIds.size() >= 2
        && m_selGesture != SelGesture::Marquee && m_selGesture != SelGesture::Lasso;
    if (m_encloseVisible && !bounds.isEmpty()) {
        m_encloseCtaRect = QRectF(bounds.center().x() - 32.0, bounds.bottom() + 36.0, 64.0, 64.0);
    } else
        m_encloseCtaRect = QRectF();
    m_selectionChromeDirty = bounds.united(m_encloseCtaRect);
    if (ids.size() == 1 && !bounds.isEmpty())
        m_selectionChromeDirty = m_selectionChromeDirty.united(modeChipRect(bounds));
    m_selectionChromeDirty.adjust(-12, -12, 12, 12);
    m_selectionBoundsRect = bounds;
    m_handleCount = 0;
    m_handleSize = 16.0;
    m_modeChipVisible = false;
    m_modeChipLabel.clear();
    m_modeChipRect = QRectF();
    if (isSelectionTool() && !ids.empty() && !bounds.isEmpty()
        && m_selGesture != SelGesture::Marquee && m_selGesture != SelGesture::Lasso
        && m_selGesture != SelGesture::Move && m_selGesture != SelGesture::Resize) {
        const DocNode *one = ids.size() == 1 ? m_document.find(ids[0]) : nullptr;
        const bool manipChrome = one && descriptorFor(one->kind).has(Verb::Resize);
        m_handleCount = manipChrome ? 8 : 6;
        m_handleSize = manipChrome ? kHandleVisualDu : 16.0;
        if (manipChrome && one) {
            m_modeChipVisible = true;
            m_modeChipLabel = QString::fromStdString(
                one->inkScaleMode == "fixedInk" ? "Keep size" : "Scale ink");
            m_modeChipRect = modeChipRect(bounds);
        }
    }
    emit selectionChromeChanged();
    syncToolCanvasPresence();
    damageToolChrome(m_selectionChromeDirty);
}

void TabletCanvasItem::encloseSelection()
{
    // @implements [SRS-EP-10] cta.enclose selection-create (never on pen-up)
    using namespace epaper::document;
    if (!m_encloseVisible)
        return;
    std::vector<std::string> ids;
    for (const QString &id : m_selectedIds)
        ids.push_back(id.toStdString());
    const SelectionCreateResult r = createSmartGroupFromSelection(m_document, ids);
    // @fix leftover selection chrome after cta.enclose (stale pickable AABB)
    if (!r.created) {
        m_encloseRefuseReason = r.reason == "smartgroup_in_selection"
            ? QStringLiteral("Cannot enclose a Smart Group")
            : QStringLiteral("No surrounding stroke");
        const std::string line = epaper::debuglog::formatEncloseLog(
            "OrdinaryInk", r.reason, "", {});
        qInfo().noquote() << QString::fromStdString(line);
        m_debugInfo = QString::fromStdString(line);
        emit debugChanged();
        emit selectionChromeChanged();
        damageToolChrome(m_selectionChromeDirty);
        return;
    }
    const std::string line = epaper::debuglog::formatEncloseLog(
        "Created", "", r.smartGroupId, r.childIds);
    qInfo().noquote() << QString::fromStdString(line);
    m_debugInfo = QString::fromStdString(line);
    emit debugChanged();
    clearSelection();
    m_encloseVisible = false;
    m_encloseCtaRect = QRectF();
    m_encloseRefuseReason.clear();
    m_selGesture = SelGesture::None;
    refreshSelectionChrome();
    scheduleVectorRasterize(true);
    notifyHistory();
    flushOneWayWire();
}

void TabletCanvasItem::beginMarqueeOrLasso(const PanelPt &canvasPos)
{
    m_drag.reset();
    m_marqueeStartPanel = canvasPos;
    m_marqueeEndPanel = canvasPos;
    m_lassoPanel.clear();
    m_lassoPanel.append(canvasPos);
    m_selGesture = m_toolMode == QLatin1String("sel_freeform") ? SelGesture::Lasso
                                                              : SelGesture::Marquee;
    m_encloseVisible = false;
    m_handleCount = 0;
    m_modeChipVisible = false;
    emit selectionChromeChanged();
    syncToolCanvasPresence();
    if (m_toolCanvas)
        m_toolCanvas->setStrokeWaveform(true);
    const QRectF live = QRectF(canvasPos, canvasPos).adjusted(-8, -8, 8, 8);
    damageToolChrome(live);
}

void TabletCanvasItem::finishMarqueeOrLasso()
{
    using namespace epaper::document;
    if (m_toolCanvas)
        m_toolCanvas->setStrokeWaveform(false);
    // Latch the kind: the gesture stays active until this call resolves it.
    const SelGesture kind = m_selGesture;
    qreal gestureSize = QLineF(m_marqueeStartPanel, m_marqueeEndPanel).length();
    if (kind == SelGesture::Lasso && m_lassoPanel.size() >= 2) {
        qreal pathLen = 0;
        QRectF bb(m_lassoPanel.first(), m_lassoPanel.first());
        for (int i = 1; i < m_lassoPanel.size(); ++i) {
            pathLen += QLineF(m_lassoPanel.at(i - 1), m_lassoPanel.at(i)).length();
            bb |= QRectF(m_lassoPanel.at(i), m_lassoPanel.at(i));
        }
        gestureSize = std::max(pathLen, QLineF(bb.topLeft(), bb.bottomRight()).length());
    }
    if (gestureSize < 8.0) {
        // m_drag was reset when the marquee began, so the drag clear is a no-op here.
        clearSelection();
        m_selGesture = SelGesture::None;
        m_lassoPanel.clear();
        m_debugInfo = QStringLiteral("sel=0 (tap)");
        emit debugChanged();
        refreshSelectionChrome();
        return;
    }
    std::vector<std::string> ids;
    if (kind == SelGesture::Lasso) {
        std::vector<InkSample> poly;
        poly.reserve(size_t(m_lassoPanel.size()));
        for (const PanelPt &p : m_lassoPanel) {
            const WorldPt w = panelToWorld(p);
            InkSample s;
            s.x = w.x;
            s.y = w.y;
            poly.push_back(s);
        }
        ids = selectByFreeform(m_document, poly);
    } else {
        const WorldPt a = panelToWorld(m_marqueeStartPanel);
        const WorldPt b = panelToWorld(m_marqueeEndPanel);
        SmartBounds rect;
        rect.x = std::min(a.x, b.x);
        rect.y = std::min(a.y, b.y);
        rect.width = std::abs(a.x - b.x);
        rect.height = std::abs(a.y - b.y);
        ids = selectByRect(m_document, rect);
    }
    m_lassoPanel.clear();
    m_selGesture = SelGesture::None;
    setSelection(ids);
    m_debugInfo = m_selectedIds.isEmpty()
        ? QStringLiteral("sel=0 (no nodes ≥80% inside)")
        : QStringLiteral("sel=%1 %2").arg(m_selectedIds.size()).arg(m_selectedIds.join(QLatin1Char(',')));
    emit debugChanged();
    refreshSelectionChrome();
}

void TabletCanvasItem::startLiveManip(const epaper::document::DocNode *subject,
                                     epaper::document::ResizeHandle handle, WorldPt world)
{
    using namespace epaper::document;
    m_selectedPickableId = QString::fromStdString(subject->id);
    m_selectedIds = QStringList{m_selectedPickableId};
    m_drag.begin(m_selectedPickableId, handle, worldQ(world), subject->transform, subject->smartBounds);
    m_selGesture = handle == ResizeHandle::None ? SelGesture::Move : SelGesture::Resize;
    m_document.beginGesture();
    m_selectionGhostClock.invalidate();
    m_liveDirtyPrev = QRectF();
    m_selectionChromeDirty = QRectF();
    SmartBounds originWorld;
    if (boundsOf(*subject, originWorld))
        m_originPanelRect = worldBoundsToPanel(originWorld).adjusted(-8, -8, 8, 8);
    else
        m_originPanelRect = QRectF();
    refreshAllConnectorWarps(m_document);
    captureOriginConnectorPunches(subject->id);
    refreshSelectionChrome();
    redrawLiveManipRegion();
}

void TabletCanvasItem::applyDragWorld(WorldPt world)
{
    using namespace epaper::document;
    if (!m_drag.active())
        return;
    epaper::UiStallSection stall("applyDragWorld");
    m_drag.setCurrentWorld(worldQ(world));
    // Not panel: ManipDrag reports a world-space delta, and a delta is not a WorldPt.
    const QPointF dWorld = m_drag.deltaWorld();
    SmartTransform liveT = m_drag.originT();
    SmartBounds liveB = m_drag.originB();
    if (m_selGesture == SelGesture::Move) {
        liveT.x = m_drag.originT().x + dWorld.x();
        liveT.y = m_drag.originT().y + dWorld.y();
        liveT.rotation = 0;
    } else if (m_selGesture == SelGesture::Resize) {
        const DocNode *n = m_document.find(m_drag.nodeId().toStdString());
        const std::string mode = n ? n->inkScaleMode : std::string("withBounds");
        const WorldBox origin = originWorldAabb(m_drag.originB(), m_drag.originT());
        const WorldBox nw = resizeWorldAabbFromHandle(origin, m_drag.handle(), world.x, world.y);
        const auto mapped = smartTransformFromWorldAabb(nw, m_drag.originB(), m_drag.originT(), mode);
        liveT = mapped.transform;
        liveB = mapped.bounds;
    }
    m_drag.setLive(liveT, liveB);
    m_document.applyLiveSmartGeometry(m_drag.nodeId().toStdString(), m_drag.liveT(), m_drag.liveB());
    refreshConnectorsBoundTo(m_document, m_drag.nodeId().toStdString());
    m_document.previewManipulationFrame();
    if (m_selectionGhostClock.isValid()
        && m_selectionGhostClock.elapsed() < kSelectionGhostMinIntervalMs)
        return;
    m_selectionGhostClock.restart();
    sendManipPreviewToInfini();
    redrawLiveManipRegion();
}

void TabletCanvasItem::beginHandleDrag(int handleIndex, WorldPt world)
{
    using namespace epaper::document;
    const ResizeHandle handle = epaper::gesture::handleFromIndex(handleIndex);
    if (handle == ResizeHandle::None)
        return;
    const DocNode *selected = m_document.find(m_selectedPickableId.toStdString());
    if (!selected || !descriptorFor(selected->kind).has(Verb::Resize))
        return;
    SmartBounds wb;
    if (!boundsOf(*selected, wb))
        return;
    if (!lodOkPanel(wb)) {
        showManipUnavailable(wb);
        return;
    }
    m_fingerGesture = FingerGesture::Resize;
    startLiveManip(selected, handle, world);
}

void TabletCanvasItem::tapModeChip()
{
    using namespace epaper::document;
    const DocNode *selected = m_document.find(m_selectedPickableId.toStdString());
    if (!selected || !descriptorFor(selected->kind).has(Verb::SetInkScaleMode))
        return;
    SmartBounds wb;
    if (boundsOf(*selected, wb) && !lodOkPanel(wb)) {
        showManipUnavailable(wb);
        return;
    }
    const std::string next = selected->inkScaleMode == "fixedInk" ? "withBounds" : "fixedInk";
    static int seq = 0;
    m_document.commitOp(makeSetInkScaleModeOp(std::string("ism-") + std::to_string(++seq),
                                              selected->id, next));
    scheduleVectorRasterize(true);
    refreshSelectionChrome();
    notifyHistory();
    flushOneWayWire();
}

TabletCanvasItem::PanelPt TabletCanvasItem::pinchArmPoint(qreal x, qreal y, qreal scale,
                                                          bool positive) const
{
    const qreal s0 = m_pinchScale0 > 0.01 ? m_pinchScale0 : 1.0;
    const qreal arm = m_pinchArm * (scale / s0);
    return PanelPt(x + (positive ? arm : -arm), y);
}

void TabletCanvasItem::stashTabletSample(const QPointF &raw, const IngestChannels &ch)
{
    m_stashRaw = RawPt{raw.x(), raw.y()};
    m_stashTablet = ch;
    m_stashValid = true;
}

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

void TabletCanvasItem::onPointerStart(qreal x, qreal y, qreal pressure, bool pen)
{
    // Qt already decided this point is not chrome — no rect hit-test here.
    // @implements [SRS-EP-04] canvas pointer entry
    const PanelPt panel(x, y);
    if (pen) {
        RawPt raw;
        IngestChannels ch = stashedChannels(panel, &raw);
        ch.pressure = pressure;
        ingestMappedTablet(QEvent::TabletPress, panel, raw, ch);
        return;
    }
    if (m_fingerLockedUntilLift)
        return;
    beginFingerTouch(panel);
}

void TabletCanvasItem::onPointerMove(qreal x, qreal y, qreal pressure, bool pen)
{
    if (m_fingerGesture == FingerGesture::TwoFinger)
        return;
    const PanelPt panel(x, y);
    if (pen) {
        RawPt raw;
        IngestChannels ch = stashedChannels(panel, &raw);
        ch.pressure = pressure;
        ingestMappedTablet(QEvent::TabletMove, panel, raw, ch);
        return;
    }
    updateFingerTouch(panel, 1);
}

void TabletCanvasItem::onPointerEnd(qreal x, qreal y, bool pen)
{
    const PanelPt panel(x, y);
    if (pen) {
        RawPt raw;
        const IngestChannels ch = stashedChannels(panel, &raw);
        ingestMappedTablet(QEvent::TabletRelease, panel, raw, ch);
        m_stashValid = false;
        return;
    }
    // PinchHandler owns two-finger; the one-finger handler deactivates on takeover.
    if (m_fingerGesture == FingerGesture::TwoFinger)
        return;
    // Cleared by the contact counter, not here: a finger still on glass after a
    // pinch must not re-arm just because the drag handler cycled.
    if (m_fingerLockedUntilLift)
        return;
    endFingerTouch(panel);
}

/**
 * Tap: press and release at one point. The travel-based branches in
 * endFingerTouch all read zero, which is exactly the select/deselect case.
 * @implements [SRS-EP-24] one-finger tap selects
 */
void TabletCanvasItem::onFingerTap(qreal x, qreal y)
{
    if (m_fingerLockedUntilLift)
        return;
    const PanelPt panel(x, y);
    beginFingerTouch(panel);
    endFingerTouch(panel);
}

void TabletCanvasItem::onPointerCancel()
{
    if (m_stroke.active)
        endStroke();
    else if (selectionGestureActive())
        endSelectionGesture();
    cancelHandTouch();
    m_stashValid = false;
    m_fingerLockedUntilLift = false;
}

/**
 * A second finger is down. PinchHandler may not have activated yet, so revert any
 * one-finger manip now and let nothing else commit until the glass is clear.
 * @implements [SRS-EP-24] two contacts are navigation
 */
void TabletCanvasItem::onSecondContact()
{
    const bool manip = m_fingerGesture == FingerGesture::Move
        || m_fingerGesture == FingerGesture::Resize || m_drag.active();
    if (manip)
        abortFingerManip();
    if (m_fingerGesture != FingerGesture::TwoFinger)
        m_fingerGesture = FingerGesture::None;
    m_fingerLockedUntilLift = true;
    qInfo().noquote() << QStringLiteral("[hand] second contact manip=%1").arg(manip ? 1 : 0);
}

void TabletCanvasItem::onContactsCleared()
{
    m_fingerLockedUntilLift = false;
}

void TabletCanvasItem::onPinchStart(qreal x, qreal y, qreal scale)
{
    // Two fingers are always navigation, whatever sits under them: the one-finger
    // handler owns the first contact and may already have grabbed a node.
    // @implements [SRS-EP-24] PinchHandler two-finger pan pinch
    if (!m_handTouchArmed || m_fingerGesture == FingerGesture::Chip) {
        m_pinchIgnore = true;
        return;
    }
    if (m_fingerGesture == FingerGesture::Move || m_fingerGesture == FingerGesture::Resize
        || m_drag.active())
        abortFingerManip();
    m_fingerGesture = FingerGesture::None;
    m_pinchArm = 80.0;
    m_pinchScale0 = scale > 0.01 ? scale : 1.0;
    m_pinchIgnore = !beginTwoFingerTouch(pinchArmPoint(x, y, scale, true),
                                        pinchArmPoint(x, y, scale, false));
    qInfo().noquote() << QStringLiteral("[hand] pinch start (%1,%2) scale=%3 taken=%4")
                             .arg(int(x))
                             .arg(int(y))
                             .arg(scale, 0, 'f', 2)
                             .arg(m_pinchIgnore ? 0 : 1);
}

void TabletCanvasItem::onPinchUpdate(qreal x, qreal y, qreal scale)
{
    if (m_pinchIgnore)
        return;
    updateTwoFingerTouch(pinchArmPoint(x, y, scale, true),
                         pinchArmPoint(x, y, scale, false));
}

void TabletCanvasItem::onPinchEnd()
{
    if (!m_pinchIgnore)
        endTwoFingerTouch();
    m_pinchIgnore = false;
}

void TabletCanvasItem::beginSelectionGesture(const PanelPt &canvasPos)
{
    // @implements [SRS-EP-11] capability-descriptor gesture route (box / empty leftover)
    using namespace epaper::document;
    m_encloseRefuseReason.clear();
    m_manipUnavailable.clear();
    m_manipUnavailableRect = QRectF();
    const WorldPt world = panelToWorld(canvasPos);

    std::vector<const DocNode *> pick;
    collectPickable(m_document.rootChildren, pick);
    const DocNode *hit = nullptr;
    for (int i = int(pick.size()) - 1; i >= 0; --i) {
        const DocNode *n = pick[size_t(i)];
        if (!n || !descriptorFor(n->kind).has(Verb::Move))
            continue;
        SmartBounds b;
        if (!boundsOf(*n, b))
            continue;
        if (world.x >= b.x && world.x <= b.x + b.width && world.y >= b.y
            && world.y <= b.y + b.height) {
            hit = n;
            break;
        }
    }

    CapabilityDescriptor cap;
    bool lodOk = true;
    if (hit) {
        cap = descriptorFor(hit->kind);
        SmartBounds wb;
        if (boundsOf(*hit, wb))
            lodOk = lodOkPanel(wb);
    }

    const GestureKind kind = resolvePress(cap, lodOk, false, false, hit != nullptr);
    if (kind == GestureKind::Unavailable) {
        SmartBounds wb;
        if (hit && boundsOf(*hit, wb))
            showManipUnavailable(wb);
        else {
            m_manipUnavailable = QStringLiteral("Too far out to move");
            emit selectionChromeChanged();
            damageToolChrome(m_manipUnavailableRect);
        }
        return;
    }
    if (kind == GestureKind::SelectMove && hit) {
        startLiveManip(hit, ResizeHandle::None, world);
        return;
    }
    beginMarqueeOrLasso(canvasPos);
}

void TabletCanvasItem::updateSelectionGesture(const PanelPt &canvasPos)
{
    using namespace epaper::document;
    if (!selectionGestureActive())
        return;
    if (m_selGesture == SelGesture::Marquee) {
        const QRectF next = QRectF(m_marqueeStartPanel, canvasPos).normalized();
        m_marqueeEndPanel = canvasPos;
        damageToolChrome(next.adjusted(-8, -8, 8, 8));
        return;
    }
    if (m_selGesture == SelGesture::Lasso) {
        const PanelPt prev = m_lassoPanel.isEmpty() ? canvasPos : m_lassoPanel.last();
        m_lassoPanel.append(canvasPos);
        m_marqueeEndPanel = canvasPos;
        const QRectF seg = QRectF(prev, canvasPos).normalized().adjusted(-8, -8, 8, 8);
        damageToolChromeSegment(seg);
        return;
    }
    applyDragWorld(panelToWorld(canvasPos));
}

void TabletCanvasItem::redrawLiveManipRegion()
{
    using namespace epaper::document;
    SmartBounds wb;
    const DocNode *n = m_document.find(m_drag.nodeId().toStdString());
    QRectF liveBounds;
    QRectF next;
    if (n && boundsOf(*n, wb)) {
        liveBounds = worldBoundsToPanel(wb);
        next = liveBounds.adjusted(-12, -12, 12, 48);
    }
    const QRectF connLive = boundConnectorsPanelUnion(m_drag.nodeId().toStdString());
    if (!connLive.isEmpty())
        next = next.isEmpty() ? connLive : next.united(connLive);
    if (!m_originConnPunch.isEmpty())
        next = next.isEmpty() ? m_originConnPunch : next.united(m_originConnPunch);
    // CanvasLayer: re-assert the origin hole only. ToolCanvas: origin∪live chrome.
    if (!m_originPanelRect.isEmpty())
        update(m_originPanelRect.toAlignedRect());
    if (!m_originConnPunch.isEmpty())
        update(m_originConnPunch.toAlignedRect());
    const QRectF toolDirty = m_liveDirtyPrev.isNull()
        ? next.united(m_originPanelRect)
        : m_liveDirtyPrev.united(next);
    m_liveDirtyPrev = next;
    m_selectionChromeDirty = m_originPanelRect;
    if (!liveBounds.isEmpty())
        m_selectionBoundsRect = liveBounds;
    const bool holdKnobs = m_drag.active() && m_drag.resizing();
    if (!holdKnobs)
        m_handleCount = 0;
    m_modeChipVisible = false;
    emit selectionChromeChanged();
    syncToolCanvasPresence();
    damageToolChrome(toolDirty);
}

void TabletCanvasItem::commitLiveManip()
{
    using namespace epaper::document;
    m_document.applyLiveSmartGeometry(m_drag.nodeId().toStdString(), m_drag.originT(), m_drag.originB());
    refreshConnectorsBoundTo(m_document, m_drag.nodeId().toStdString());
    const double dx = m_drag.liveT().x - m_drag.originT().x;
    const double dy = m_drag.liveT().y - m_drag.originT().y;
    const bool resized = m_selGesture == SelGesture::Resize;
    const bool moved = resized || qHypot(dx, dy) >= 2.0
        || qHypot(m_drag.liveT().scaleX - m_drag.originT().scaleX,
                  m_drag.liveT().scaleY - m_drag.originT().scaleY) > 0.01
        || std::abs(m_drag.liveB().width - m_drag.originB().width) > 0.5;
    m_selGesture = SelGesture::None;
    const QRectF punch = m_originPanelRect.united(m_selectionChromeDirty).united(m_originConnPunch);
    m_originPanelRect = QRectF();
    m_originConnPunch = QRectF();
    m_originConnStrokes.clear();
    if (!moved) {
        m_document.abortGesture();
        m_selectedPickableId = m_drag.nodeId();
        m_drag.reset();
        if (!punch.isEmpty())
            update(punch.toAlignedRect());
        refreshSelectionChrome();
        notifyHistory();
        return;
    }
    ++m_toolIntentSeq;
    const std::string opId = std::string("sst-") + std::to_string(m_toolIntentSeq);
    const SmartBounds liveB = m_drag.liveB();
    const SmartBounds *bptr = resized ? &liveB : nullptr;
    const ApplyResult r = m_document.commitOp(
        makeSetSmartTransformOp(opId, m_drag.nodeId().toStdString(), m_drag.liveT(), bptr));
    (void)r;
    refreshAllConnectorWarps(m_document);
    m_selectedPickableId = m_drag.nodeId();
    m_drag.reset();
    scheduleVectorRasterize(true);
    m_liveDirtyPrev = QRectF();
    refreshSelectionChrome();
    notifyHistory();
    flushOneWayWire();
}

void TabletCanvasItem::endSelectionGesture()
{
    if (!selectionGestureActive())
        return;
    if (m_selGesture == SelGesture::Marquee || m_selGesture == SelGesture::Lasso) {
        finishMarqueeOrLasso();
        return;
    }
    if (m_selGesture == SelGesture::Move || m_selGesture == SelGesture::Resize) {
        commitLiveManip();
        return;
    }
    m_selGesture = SelGesture::None;
}

void TabletCanvasItem::bindToolCanvas(ToolCanvasItem *overlay)
{
    m_toolCanvas = overlay;
    syncToolCanvasPresence();
}

void TabletCanvasItem::damageToolChrome(const QRectF &next)
{
    QRectF u = m_toolChromePrev.isNull() ? next : m_toolChromePrev.united(next);
    m_toolChromePrev = next;
    if (!m_toolCanvas || u.isEmpty())
        return;
    m_toolCanvas->update(u.toAlignedRect().adjusted(-8, -8, 8, 8));
}

void TabletCanvasItem::damageToolChromeSegment(const QRectF &seg)
{
    m_toolChromePrev = m_toolChromePrev.united(seg);
    if (!m_toolCanvas || seg.isEmpty())
        return;
    m_toolCanvas->update(seg.toAlignedRect());
}

void TabletCanvasItem::syncToolCanvasPresence()
{
    if (!m_toolCanvas)
        return;
    const bool liveManip = m_selGesture == SelGesture::Move || m_selGesture == SelGesture::Resize;
    const bool strokeChrome = m_selGesture == SelGesture::Marquee || m_selGesture == SelGesture::Lasso;
    const bool settled = isSelectionTool() && !m_selectedIds.isEmpty() && !liveManip && !strokeChrome;
    const bool on = isSelectionTool() && (strokeChrome || liveManip || settled);
    m_toolCanvas->setVisible(on);
    if (on && !strokeChrome)
        m_toolCanvas->setStrokeWaveform(false);
}

void TabletCanvasItem::paintLiveManipOnToolCanvas(QPainter *painter)
{
    using namespace epaper::document;
    const DocNode *n = m_document.find(m_drag.nodeId().toStdString());
    if (!n)
        return;
    drawTree(*painter, n->children, n);
    for (const auto &node : m_document.rootChildren) {
        if (node.kind != NodeKind::Connector)
            continue;
        if (node.fromNodeId != n->id && node.toNodeId != n->id)
            continue;
        drawWarpedConnector(*painter, node);
    }

    SmartBounds wb;
    if (!boundsOf(*n, wb))
        return;
    const QRectF r = QRectF(worldToPanel(wb.x, wb.y),
                            worldToPanel(wb.x + wb.width, wb.y + wb.height)).normalized();
    QPen dotted(Qt::black);
    dotted.setWidthF(3.0);
    dotted.setStyle(Qt::DotLine);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(dotted);
    painter->drawRect(r);

    const qreal h = kHandleVisualDu;
    const PanelPt pts[8] = {
        r.topLeft(),
        PanelPt(r.center().x(), r.top()),
        r.topRight(),
        PanelPt(r.right(), r.center().y()),
        r.bottomRight(),
        PanelPt(r.center().x(), r.bottom()),
        r.bottomLeft(),
        PanelPt(r.left(), r.center().y()),
    };
    painter->setBrush(Qt::white);
    QPen solid(Qt::black);
    solid.setWidthF(4.0);
    painter->setPen(solid);
    for (const PanelPt &pt : pts)
        painter->drawRect(QRectF(pt.x() - h * 0.5, pt.y() - h * 0.5, h, h));
    const QRectF chip = modeChipRect(r);
    painter->fillRect(chip, Qt::white);
    painter->drawRect(chip);
    painter->drawText(chip, Qt::AlignCenter,
                      QString::fromStdString(n->inkScaleMode == "fixedInk" ? "Keep size" : "Scale ink"));
}

void TabletCanvasItem::paintToolChrome(QPainter *painter)
{
    // @implements [SRS-EP-12] ovl.marquee / ovl.lasso / ovl.nodes_bounds
    if (!isSelectionTool())
        return;

    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (m_selGesture == SelGesture::Move || m_selGesture == SelGesture::Resize) {
        paintLiveManipOnToolCanvas(painter);
        painter->restore();
        return;
    }

    QPen dotted(Qt::black);
    dotted.setWidthF(3.0);
    dotted.setStyle(Qt::DotLine);
    painter->setBrush(Qt::NoBrush);

    if (m_selGesture == SelGesture::Marquee) {
        painter->setPen(dotted);
        painter->drawRect(QRectF(m_marqueeStartPanel, m_marqueeEndPanel).normalized());
        painter->restore();
        return;
    }
    if (m_selGesture == SelGesture::Lasso && m_lassoPanel.size() >= 2) {
        painter->setPen(dotted);
        QPainterPath path;
        path.moveTo(m_lassoPanel.first());
        for (int i = 1; i < m_lassoPanel.size(); ++i)
            path.lineTo(m_lassoPanel.at(i));
        painter->drawPath(path);
        painter->restore();
        return;
    }

    if (m_selectedIds.isEmpty() && m_selectedPickableId.isEmpty() && !selectionGestureActive()) {
        painter->restore();
        return;
    }

    using namespace epaper::document;
    std::vector<std::string> ids;
    for (const QString &id : m_selectedIds)
        ids.push_back(id.toStdString());
    if (ids.empty() && !m_selectedPickableId.isEmpty())
        ids.push_back(m_selectedPickableId.toStdString());

    SmartBounds unionB;
    QRectF r;
    if (unionAabbOfIds(m_document, ids, unionB)) {
        const PanelPt tl = worldToPanel(unionB.x, unionB.y);
        const PanelPt br = worldToPanel(unionB.x + unionB.width, unionB.y + unionB.height);
        r = QRectF(tl, br).normalized();
    }
    if (r.isEmpty()) {
        painter->restore();
        return;
    }

    painter->setPen(dotted);
    painter->drawRect(r);
    painter->restore();
}

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

void TabletCanvasItem::syncFramePanelSize() const
{
    qreal w = width();
    qreal h = height();
    if (w < 2.0 && window())
        w = window()->width();
    if (h < 2.0 && window())
        h = window()->height();
    m_frame.setPanelSize(double(w), double(h));
}

void TabletCanvasItem::applyFrameIntent(epaper::canvasframe::FrameIntent intent)
{
    using epaper::canvasframe::FrameIntent;
    using epaper::canvasframe::has;
    if (has(intent, FrameIntent::OrientationChanged))
        updateToolChipRect();
    // CameraChanged: callers schedule rasterize / chrome refresh themselves —
    // those depend on settle flags and gesture context the frame does not know.
    Q_UNUSED(FrameIntent::CameraChanged);
}

double TabletCanvasItem::panelScale() const
{
    syncFramePanelSize();
    return m_frame.panelScale();
}

QRectF TabletCanvasItem::worldBoundsToPanel(const epaper::document::SmartBounds &wb) const
{
    syncFramePanelSize();
    epaper::canvasframe::PanelPt tl;
    epaper::canvasframe::PanelPt br;
    m_frame.worldBoundsToPanel(wb.x, wb.y, wb.width, wb.height, &tl, &br);
    return QRectF(QPointF(tl.x, tl.y), QPointF(br.x, br.y)).normalized();
}

bool TabletCanvasItem::lodOkPanel(const epaper::document::SmartBounds &wb) const
{
    // @implements [SRS-EP-11] LOD only when zoomed out; scale ≥ 1.0 always manipulable
    syncFramePanelSize();
    return m_frame.lodOkPanel(wb.x, wb.y, wb.width, wb.height);
}

bool TabletCanvasItem::viewportZoomedOut() const
{
    syncFramePanelSize();
    return m_frame.viewportZoomedOut();
}

void TabletCanvasItem::showManipUnavailable(const epaper::document::SmartBounds &wb)
{
    // Design copy: ind.manipulation_unavailable — not the debug HUD.
    m_manipUnavailable = QStringLiteral("Too far out to move");
    const QRectF box = worldBoundsToPanel(wb);
    constexpr qreal kW = 220.0;
    constexpr qreal kH = 36.0;
    qreal x = box.center().x() - kW * 0.5;
    qreal y = box.bottom() + 8.0;
    if (y + kH > height())
        y = std::max(8.0, box.top() - kH - 8.0);
    x = qBound(8.0, x, qMax(8.0, width() - kW - 8.0));
    m_manipUnavailableRect = QRectF(x, y, kW, kH);
    emit selectionChromeChanged();
}

qreal TabletCanvasItem::worldStrokeWidth(qreal pressure) const
{
    return qreal(epaper::strokecapture::worldStrokeWidth(double(pressure),
                                                         epaper::strokecapture::StrokeCapture::kBaseWorldStroke));
}

TabletCanvasItem::FrameUv TabletCanvasItem::panelToFrameUv(const PanelPt &panel) const
{
    syncFramePanelSize();
    return m_frame.panelToFrameUv({panel.x(), panel.y()});
}

TabletCanvasItem::PanelPt TabletCanvasItem::frameUvToPanel(FrameUv uv) const
{
    syncFramePanelSize();
    const auto p = m_frame.frameUvToPanel(uv);
    return PanelPt(p.x, p.y);
}

TabletCanvasItem::PanelPt TabletCanvasItem::worldToPanel(double wx, double wy) const
{
    syncFramePanelSize();
    const auto p = m_frame.worldToPanel(wx, wy);
    return PanelPt(p.x, p.y);
}

TabletCanvasItem::WorldPt TabletCanvasItem::panelToWorld(const PanelPt &panel) const
{
    syncFramePanelSize();
    return m_frame.panelToWorld({panel.x(), panel.y()});
}

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
    const double rw = m_frame.drawingRegion.valid ? (m_frame.drawingRegion.maxX - m_frame.drawingRegion.minX)
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

qreal TabletCanvasItem::connectorPanelStrokeWidth(const epaper::document::DocNode &conn) const
{
    double worldSw = conn.style.strokeWidth;
    if (worldSw <= 0.0 && !conn.children.empty())
        worldSw = conn.children.front().style.strokeWidth;
    if (worldSw <= 0.0)
        worldSw = 2.0;
    return qMax<qreal>(1.0, worldSw * panelScale());
}

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

void TabletCanvasItem::captureOriginConnectorPunches(const std::string &sgId)
{
    m_originConnStrokes.clear();
    m_originConnPunch = QRectF();
    using epaper::document::NodeKind;
    for (const auto &node : m_document.rootChildren) {
        if (node.kind != NodeKind::Connector)
            continue;
        if (node.fromNodeId != sgId && node.toNodeId != sgId)
            continue;
        if (node.warpedSamples.size() < 2)
            continue;
        OriginConnStroke st;
        st.width = connectorPanelStrokeWidth(node);
        st.panel.reserve(int(node.warpedSamples.size()));
        for (const auto &s : node.warpedSamples)
            st.panel.append(worldToPanel(s.x, s.y));
        m_originConnStrokes.append(st);
        const QRectF r = warpedConnectorPanelRect(node);
        m_originConnPunch = m_originConnPunch.isEmpty() ? r : m_originConnPunch.united(r);
    }
}

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

QRectF TabletCanvasItem::boundConnectorsPanelUnion(const std::string &sgId) const
{
    QRectF u;
    for (const auto &node : m_document.rootChildren) {
        if (node.kind != epaper::document::NodeKind::Connector)
            continue;
        if (node.fromNodeId != sgId && node.toNodeId != sgId)
            continue;
        const QRectF r = warpedConnectorPanelRect(node);
        u = u.isEmpty() ? r : u.united(r);
    }
    return u;
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
        else if (node.kind == NodeKind::Connector)
            drawWarpedConnector(p, node);
        else
            drawDocNode(p, node, smartParent);
    }
}

void TabletCanvasItem::rasterizeVectors(bool sharp)
{
    epaper::UiStallSection stall("rasterizeVectors");
    using epaper::document::refreshAllConnectorWarps;
    refreshAllConnectorWarps(m_document);
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

/** @implements [SRS-EP-12] enclose one-shot width pulse (UI-EP-06 / CHL-0020) */
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

/** @implements [SRS-EP-12] last-join membership highlight, no blink (UI-EP-06) */
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

void TabletCanvasItem::clearMembershipHighlight()
{
    m_highlightInkIds.clear();
}
