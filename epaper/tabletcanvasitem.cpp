#include "tabletcanvasitem.h"
#include "ui_stall.hpp"
#include "strokesync.h"
#include "epaperbridge.h"
#include "usb_link.hpp"
#include "latencyprobe/stub_document.hpp"
#include "document/connector_warp.hpp"
#include "document/recognizer_dispatch.hpp"
#include "document/recognize_enclose.hpp"
#include "document/membership.hpp"
#include "document/surround_create.hpp"
#include "document/manipulate.hpp"
#include "document/capability.hpp"
#include "debuglog/debug_log_format.hpp"
#include "toolcanvasitem.h"
#include "toolchip_layout.hpp"
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

/** Debug Switch chip — must match Main.qml xochitlSwitch geometry. */
constexpr QRectF kXochitlSwitchRect(8, 8, 64, 64);

QRectF infiniReconnectRect(qreal panelW)
{
    return QRectF(panelW - 8.0 - 64.0, 8.0, 64.0, 64.0);
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
        flushOneWayWire();
    });
    connect(m_sync, &StrokeSync::socketDisconnected, this, [this]() {
        m_oneWay.onLinkDown();
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
    const QPointF canvasPos = mapInputToCanvas(pos);
    m_lastPoint = canvasPos;
    m_lastRaw = pos;

    if (tryDebugChromeAtWindowPos(pos) || tryDebugChromeAtWindowPos(canvasPos))
        return;

    const bool isPress = (type == QEvent::TabletPress || type == QEvent::MouseButtonPress);
    const bool isMove = (type == QEvent::TabletMove || type == QEvent::MouseMove);
    const bool isRelease = (type == QEvent::TabletRelease || type == QEvent::MouseButtonRelease);
    const qreal panelH = ingestPanelHeight();
    const bool stale = epaper::ingest::isStaleOriginSample(
        double(pos.x()), double(pos.y()), double(canvasPos.x()), double(canvasPos.y()),
        double(panelH));
    // @fix [STORY-EP-033] reject origin/stale first sample on pen-down
    const auto guard = epaper::ingest::decideOriginPress(
        isPress, isMove, isRelease, stale, &m_awaitingPlausiblePress);
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

void TabletCanvasItem::applyContactPress(const QPointF &canvasPos, const IngestChannels &ch)
{
    // Pen on ToolChip — not ink; arm via tile hit-test (pen-on-chip fallback).
    // First plausible sample, including Move-after-stale-Press (STORY-EP-033).
    if (kXochitlSwitchRect.contains(canvasPos)) {
        tryArmToolAtCanvasPos(canvasPos);
        return;
    }
    if (infiniReconnectRect(width()).contains(canvasPos)) {
        tryArmToolAtCanvasPos(canvasPos);
        return;
    }
    if (pointInToolChip(canvasPos)) {
        tryArmToolAtCanvasPos(canvasPos);
        return;
    }
    if (pointInEncloseCta(canvasPos)) {
        encloseSelection();
        return;
    }
    if (isSelectionTool())
        beginSelectionGesture(canvasPos);
    else
        beginStroke(canvasPos, ch);
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
    if (m_selectionGesture
        && (m_selGesture == SelGesture::Move || m_selGesture == SelGesture::Resize)
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
    // @implements [SRS-EP-04] latch exclusive tool and both toggles at pen-down
    m_chip.latchPenDown();
    m_strokeArmedTool = QString::fromStdString(m_chip.latchedTool);
    const QString latch = QString::fromStdString(m_chip.dispatchTuple());
    if (m_lastStrokeLatch != latch) {
        m_lastStrokeLatch = latch;
        emit lastStrokeLatchChanged();
    }

    m_current.clear();
    m_activeStrokeId = QStringLiteral("s-%1").arg(++m_strokeSeq);
    m_activeWorldStrokeWidth = worldStrokeWidth(ch.pressure);
    m_current.append(makePoint(canvasPos, ch));
    m_lastEmitted = m_current.last();
    m_hasEmitted = false;
    m_strokeActive = true;
    m_pendingDirty = QRectF();
    m_flushClock.restart();
    m_strokePreviewSent = false;

    // @fix [STORY-EP-033] do not stamp a speckle at digitizer origin, and do not
    // preview it to Infini (stroke_begin/point is device→desktop, not the reverse).
    if (epaper::ingest::isStaleOriginSample(double(m_lastRaw.x()), double(m_lastRaw.y()),
                                           double(canvasPos.x()), double(canvasPos.y()),
                                           double(ingestPanelHeight()))) {
        m_hasEmitted = false;
        return;
    }

    syncBegin();
    syncPoint(m_current.last());
    m_strokePreviewSent = true;

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

    const qreal panelH = ingestPanelHeight();
    // Origin sample after a real press (tip → bottom-left) must not be appended.
    if (epaper::ingest::isStaleOriginSample(
            double(m_lastRaw.x()), double(m_lastRaw.y()), double(canvasPos.x()),
            double(canvasPos.y()), double(panelH))) {
        return;
    }

    if (m_current.size() == 1
        && epaper::ingest::isImplausibleOriginJump(
            double(m_current.first().pos.x()), double(m_current.first().pos.y()),
            double(canvasPos.x()), double(canvasPos.y()), double(panelH))) {
        // @fix [STORY-EP-033] replace stale origin start; do not emit the diagonal
        m_current[0] = makePoint(canvasPos, ch);
        m_lastEmitted = m_current[0];
        if (!m_strokePreviewSent) {
            syncBegin();
            m_strokePreviewSent = true;
        }
        syncPoint(m_current[0]);
        emitSegment(m_lastEmitted, m_lastEmitted);
        m_hasEmitted = true;
        flushPending();
        return;
    }

    Point next = makePoint(canvasPos, ch);
    m_current.append(next);
    if (!m_strokePreviewSent) {
        syncBegin();
        m_strokePreviewSent = true;
    }
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
    notifyHistory();
    flushOneWayWire();

    m_current.clear();
    m_hasEmitted = false;
    m_strokeActive = false;
    m_strokePreviewSent = false;
    m_chip.penUp();

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
/** @implements [SRS-EP-10] ADR-0022 closure-first dispatch (one [recog] line) */
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

    const std::string tool = m_chip.latchedTool;
    if (tool == "sel_rect" || tool == "sel_freeform")
        return;

    RecogLatch latch;
    latch.inkBox = m_chip.latchedInkBox;
    latch.connector = m_chip.latchedConnector;
    const RecogDispatchResult d = dispatchFinishedStroke(m_document, stroke, map, latch);
    m_ingestNs.push_back(d.ns);
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
    m_oneWay.beginPreviewStroke(m_activeStrokeId.toStdString());
    flushOneWayWire();
}

void TabletCanvasItem::syncPoint(const Point &pt)
{
    m_oneWay.previewStrokePoint(m_activeStrokeId.toStdString(), pt.pos.x(), pt.pos.y(),
                                pt.pressure);
    flushOneWayWire();
}

void TabletCanvasItem::syncEnd()
{
    m_oneWay.endPreviewStroke(m_activeStrokeId.toStdString());
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
    if (m_gesturePickableId.isEmpty())
        return;
    QJsonObject xf;
    xf.insert(QStringLiteral("x"), m_liveT.x);
    xf.insert(QStringLiteral("y"), m_liveT.y);
    xf.insert(QStringLiteral("rotation"), 0);
    xf.insert(QStringLiteral("scaleX"), m_liveT.scaleX);
    xf.insert(QStringLiteral("scaleY"), m_liveT.scaleY);
    QJsonObject o;
    o.insert(QStringLiteral("type"), QStringLiteral("manip_preview"));
    o.insert(QStringLiteral("id"), m_gesturePickableId);
    o.insert(QStringLiteral("transform"), xf);
    if (m_selGesture == SelGesture::Resize) {
        QJsonObject b;
        b.insert(QStringLiteral("x"), m_liveB.x);
        b.insert(QStringLiteral("y"), m_liveB.y);
        b.insert(QStringLiteral("width"), m_liveB.width);
        b.insert(QStringLiteral("height"), m_liveB.height);
        o.insert(QStringLiteral("bounds"), b);
    }
    m_sync->sendLine(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

void TabletCanvasItem::onHostMessage(const QJsonObject &obj)
{
    epaper::UiStallSection stall("onHostMessage");
    const QString inboundType = obj.value(QStringLiteral("type")).toString();
    if (inboundType != QLatin1String("viewport"))
        qInfo() << "[sync] inbound" << inboundType << "live" << m_oneWay.epochLive()
                << "handshake" << m_oneWay.handshakeInFlight();
    const QByteArray raw = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    m_oneWay.handleInboundLine(std::string(raw.constData(), static_cast<size_t>(raw.size())));
    if (m_oneWay.viewportApplied() > 0 && obj.value(QStringLiteral("type")).toString()
            == QLatin1String("viewport")) {
        applyViewport(obj);
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
    if (!m_chip.setExclusive(mode.toStdString()))
        return;
    const bool hadHighlight = !m_highlightInkIds.empty();
    m_toolMode = QString::fromStdString(m_chip.exclusive);
    clearMembershipHighlight();
    if (hadHighlight && !m_strokeActive)
        scheduleVectorRasterize(true);
    emit toolModeChanged();
    m_debugInfo = QStringLiteral("tool=%1 pickables=%2")
                      .arg(m_toolMode)
                      .arg(m_pickables.size());
    emit debugChanged();
    syncToolCanvasPresence();
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

QString TabletCanvasItem::toolChipHitAt(const QPointF &canvasPos) const
{
    if (!pointInToolChip(canvasPos))
        return {};
    const qreal relX = canvasPos.x() - m_toolChipRect.x();
    return QString::fromLatin1(epaper::toolchip::hitId(epaper::toolchip::hitAtRelX(relX)));
}

bool TabletCanvasItem::tryDebugChromeAtWindowPos(const QPointF &windowPos)
{
    qreal w = width();
    if (w < 2.0 && window())
        w = window()->width();
    if (window() && window()->width() > w)
        w = window()->width();
    constexpr qreal pad = 80.0;
    if (QRectF(0, 0, pad, pad).contains(windowPos)) {
        if (EpaperBridge *b = EpaperBridge::instance())
            b->restoreXochitl();
        return true;
    }
    if (w > pad && QRectF(w - pad, 0, pad, pad).contains(windowPos)) {
        if (auto *u = epaper::UsbLink::instance())
            u->recoverInfini();
        return true;
    }
    return false;
}

bool TabletCanvasItem::tryArmToolAtCanvasPos(const QPointF &canvasPos)
{
    if (tryDebugChromeAtWindowPos(canvasPos))
        return true;
    const QString hit = toolChipHitAt(canvasPos);
    if (hit.isEmpty())
        return false;
    if (hit == QLatin1String("undo")) {
        requestUndo();
        return true;
    }
    if (hit == QLatin1String("redo")) {
        requestRedo();
        return true;
    }
    if (hit == QLatin1String("gap") || hit == QLatin1String("publish"))
        return true;
    if (hit == QLatin1String("tgl.recog.ink_box")) {
        toggleRecogInkBox();
        return true;
    }
    if (hit == QLatin1String("tgl.recog.connector")) {
        toggleRecogConnector();
        return true;
    }
    armTool(hit);
    return true;
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
        m_gesturePickableId.clear();
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

bool TabletCanvasItem::isSelectionTool() const
{
    return m_toolMode == QLatin1String("sel_rect") || m_toolMode == QLatin1String("sel_freeform");
}

bool TabletCanvasItem::pointInEncloseCta(const QPointF &canvasPos) const
{
    return m_encloseVisible && m_encloseCtaRect.contains(canvasPos);
}

QString TabletCanvasItem::hitLocalSmartGroup(const QPointF &world) const
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
        if (world.x() >= b.x && world.x() <= b.x + b.width && world.y() >= b.y
            && world.y() <= b.y + b.height)
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
        const QPointF tl = worldToPanel(unionB.x, unionB.y);
        const QPointF br = worldToPanel(unionB.x + unionB.width, unionB.y + unionB.height);
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
    if (!ids.empty() && !bounds.isEmpty()
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
    m_selectedIds.clear();
    m_selectedPickableId.clear();
    m_gesturePickableId.clear();
    m_encloseVisible = false;
    m_encloseCtaRect = QRectF();
    m_encloseRefuseReason.clear();
    m_selGesture = SelGesture::None;
    m_selectionGesture = false;
    refreshSelectionChrome();
    scheduleVectorRasterize(true);
    notifyHistory();
    flushOneWayWire();
}

void TabletCanvasItem::beginMarqueeOrLasso(const QPointF &canvasPos)
{
    m_selectionGesture = true;
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
    m_selectionGesture = false;
    qreal gestureSize = QLineF(m_marqueeStartPanel, m_marqueeEndPanel).length();
    if (m_selGesture == SelGesture::Lasso && m_lassoPanel.size() >= 2) {
        qreal pathLen = 0;
        QRectF bb(m_lassoPanel.first(), m_lassoPanel.first());
        for (int i = 1; i < m_lassoPanel.size(); ++i) {
            pathLen += QLineF(m_lassoPanel.at(i - 1), m_lassoPanel.at(i)).length();
            bb |= QRectF(m_lassoPanel.at(i), m_lassoPanel.at(i));
        }
        gestureSize = std::max(pathLen, QLineF(bb.topLeft(), bb.bottomRight()).length());
    }
    if (gestureSize < 8.0) {
        m_selectedIds.clear();
        m_selectedPickableId.clear();
        m_selGesture = SelGesture::None;
        m_lassoPanel.clear();
        m_debugInfo = QStringLiteral("sel=0 (tap)");
        emit debugChanged();
        refreshSelectionChrome();
        return;
    }
    std::vector<std::string> ids;
    if (m_selGesture == SelGesture::Lasso) {
        std::vector<InkSample> poly;
        poly.reserve(size_t(m_lassoPanel.size()));
        for (const QPointF &p : m_lassoPanel) {
            const QPointF w = panelToWorld(p);
            InkSample s;
            s.x = w.x();
            s.y = w.y();
            poly.push_back(s);
        }
        ids = selectByFreeform(m_document, poly);
    } else {
        const QPointF a = panelToWorld(m_marqueeStartPanel);
        const QPointF b = panelToWorld(m_marqueeEndPanel);
        SmartBounds rect;
        rect.x = std::min(a.x(), b.x());
        rect.y = std::min(a.y(), b.y());
        rect.width = std::abs(a.x() - b.x());
        rect.height = std::abs(a.y() - b.y());
        ids = selectByRect(m_document, rect);
    }
    m_lassoPanel.clear();
    m_selGesture = SelGesture::None;
    m_selectedIds.clear();
    for (const auto &id : ids)
        m_selectedIds.append(QString::fromStdString(id));
    if (!m_selectedIds.isEmpty())
        m_selectedPickableId = m_selectedIds.first();
    else
        m_selectedPickableId.clear();
    m_debugInfo = m_selectedIds.isEmpty()
        ? QStringLiteral("sel=0 (no nodes ≥80% inside)")
        : QStringLiteral("sel=%1 %2").arg(m_selectedIds.size()).arg(m_selectedIds.join(QLatin1Char(',')));
    emit debugChanged();
    refreshSelectionChrome();
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
    // @implements [SRS-EP-11] capability-descriptor gesture route
    using namespace epaper::document;
    m_encloseRefuseReason.clear();
    m_manipUnavailable.clear();
    m_manipUnavailableRect = QRectF();
    const QPointF world = panelToWorld(canvasPos);

    auto worldToPanelCb = [this](double wx, double wy, double *px, double *py) {
        const QPointF p = worldToPanel(wx, wy);
        *px = p.x();
        *py = p.y();
    };

    const DocNode *selected = nullptr;
    if (!m_selectedPickableId.isEmpty())
        selected = m_document.find(m_selectedPickableId.toStdString());

    bool handleHit = false;
    bool toggleHit = false;
    ResizeHandle handle = ResizeHandle::None;
    if (selected) {
        const CapabilityDescriptor cap0 = descriptorFor(selected->kind);
        SmartBounds wb;
        if (boundsOf(*selected, wb)) {
            if (cap0.has(Verb::Resize)) {
                handle = hitResizeHandlePanel(wb, canvasPos.x(), canvasPos.y(), panelScale(), worldToPanelCb);
                handleHit = handle != ResizeHandle::None;
            }
            if (cap0.has(Verb::SetInkScaleMode)) {
                const QPointF tl = worldToPanel(wb.x, wb.y);
                const QPointF br = worldToPanel(wb.x + wb.width, wb.y + wb.height);
                const QRectF r = QRectF(tl, br).normalized();
                const QRectF tog = modeChipRect(r);
                toggleHit = tog.contains(canvasPos);
            }
            if (!lodOkPanel(wb) && (handleHit || toggleHit
                    || (world.x() >= wb.x && world.x() <= wb.x + wb.width
                        && world.y() >= wb.y && world.y() <= wb.y + wb.height))) {
                showManipUnavailable(wb);
                return;
            }
        }
    }

    std::vector<const DocNode *> pick;
    collectPickable(m_document.rootChildren, pick);
    const DocNode *hit = nullptr;
    for (int i = int(pick.size()) - 1; i >= 0; --i) {
        const DocNode *n = pick[size_t(i)];
        // Single-press pickables are SmartGroups (Move), not free ink [SRS-EP-11].
        if (!n || !descriptorFor(n->kind).has(Verb::Move))
            continue;
        SmartBounds b;
        if (!boundsOf(*n, b))
            continue;
        if (world.x() >= b.x && world.x() <= b.x + b.width && world.y() >= b.y
            && world.y() <= b.y + b.height) {
            hit = n;
            break;
        }
    }

    const DocNode *subject = selected && (handleHit || toggleHit) ? selected : hit;
    CapabilityDescriptor cap;
    bool lodOk = true;
    bool nodeHit = subject != nullptr;
    if (subject) {
        cap = descriptorFor(subject->kind);
        SmartBounds wb;
        if (boundsOf(*subject, wb))
            lodOk = lodOkPanel(wb);
    }

    const GestureKind kind = resolvePress(cap, lodOk, handleHit, toggleHit, nodeHit);
    if (kind == GestureKind::Unavailable) {
        SmartBounds wb;
        if (subject && boundsOf(*subject, wb))
            showManipUnavailable(wb);
        else {
            m_manipUnavailable = QStringLiteral("Too far out to move");
            emit selectionChromeChanged();
            damageToolChrome(m_manipUnavailableRect);
        }
        return;
    }
    if (kind == GestureKind::ToggleMode && subject) {
        const std::string next = subject->inkScaleMode == "fixedInk" ? "withBounds" : "fixedInk";
        static int seq = 0;
        m_document.commitOp(makeSetInkScaleModeOp(std::string("ism-") + std::to_string(++seq),
                                                  subject->id, next));
        scheduleVectorRasterize(true);
        refreshSelectionChrome();
        notifyHistory();
        flushOneWayWire();
        return;
    }
    if (kind == GestureKind::Resize || kind == GestureKind::SelectMove) {
        m_selectedPickableId = QString::fromStdString(subject->id);
        m_gesturePickableId = m_selectedPickableId;
        m_selectedIds = QStringList{m_selectedPickableId};
        m_originT = subject->transform;
        m_originB = subject->smartBounds;
        m_liveT = m_originT;
        m_liveB = m_originB;
        m_gestureStartWorld = world;
        m_gestureLastWorld = world;
        m_selectionGesture = true;
        m_resizeHandle = handle;
        m_selGesture = kind == GestureKind::Resize ? SelGesture::Resize : SelGesture::Move;
        m_document.beginGesture();
        m_selectionGhostClock.invalidate();
        m_liveDirtyPrev = QRectF();
        m_selectionChromeDirty = QRectF();
        {
            SmartBounds originWorld;
            if (boundsOf(*subject, originWorld))
                m_originPanelRect = worldBoundsToPanel(originWorld).adjusted(-8, -8, 8, 8);
            else
                m_originPanelRect = QRectF();
        }
        refreshAllConnectorWarps(m_document);
        captureOriginConnectorPunches(subject->id);
        refreshSelectionChrome();
        redrawLiveManipRegion();
        return;
    }
    beginMarqueeOrLasso(canvasPos);
}

void TabletCanvasItem::updateSelectionGesture(const QPointF &canvasPos)
{
    using namespace epaper::document;
    if (!m_selectionGesture)
        return;
    if (m_selGesture == SelGesture::Marquee) {
        const QRectF next = QRectF(m_marqueeStartPanel, canvasPos).normalized();
        m_marqueeEndPanel = canvasPos;
        damageToolChrome(next.adjusted(-8, -8, 8, 8));
        return;
    }
    if (m_selGesture == SelGesture::Lasso) {
        const QPointF prev = m_lassoPanel.isEmpty() ? canvasPos : m_lassoPanel.last();
        m_lassoPanel.append(canvasPos);
        m_marqueeEndPanel = canvasPos;
        const QRectF seg = QRectF(prev, canvasPos).normalized().adjusted(-8, -8, 8, 8);
        damageToolChromeSegment(seg);
        return;
    }
    m_gestureLastWorld = panelToWorld(canvasPos);

    const double dx = m_gestureLastWorld.x() - m_gestureStartWorld.x();
    const double dy = m_gestureLastWorld.y() - m_gestureStartWorld.y();
    if (m_selGesture == SelGesture::Move) {
        m_liveT = m_originT;
        m_liveT.x = m_originT.x + dx;
        m_liveT.y = m_originT.y + dy;
        m_liveT.rotation = 0;
        m_liveB = m_originB;
    } else if (m_selGesture == SelGesture::Resize) {
        const DocNode *n = m_document.find(m_gesturePickableId.toStdString());
        const std::string mode = n ? n->inkScaleMode : std::string("withBounds");
        const WorldBox origin = originWorldAabb(m_originB, m_originT);
        const WorldBox nw = resizeWorldAabbFromHandle(origin, m_resizeHandle, m_gestureLastWorld.x(),
                                                      m_gestureLastWorld.y());
        const auto mapped = smartTransformFromWorldAabb(nw, m_originB, m_originT, mode);
        m_liveT = mapped.transform;
        m_liveB = mapped.bounds;
    }
    m_document.applyLiveSmartGeometry(m_gesturePickableId.toStdString(), m_liveT, m_liveB);
    refreshConnectorsBoundTo(m_document, m_gesturePickableId.toStdString());
    m_document.previewManipulationFrame();
    // Keep live transform current so a short drag still commits [SRS-EP-11].
    // Throttle only the panel redraw (≥5 Hz / stall ≤200 ms).
    if (m_selectionGhostClock.isValid()
        && m_selectionGhostClock.elapsed() < kSelectionGhostMinIntervalMs)
        return;
    m_selectionGhostClock.restart();
    sendManipPreviewToInfini();
    redrawLiveManipRegion();
}

void TabletCanvasItem::redrawLiveManipRegion()
{
    using namespace epaper::document;
    SmartBounds wb;
    const DocNode *n = m_document.find(m_gesturePickableId.toStdString());
    QRectF liveBounds;
    QRectF next;
    if (n && boundsOf(*n, wb)) {
        liveBounds = worldBoundsToPanel(wb);
        next = liveBounds.adjusted(-12, -12, 12, 48);
    }
    const QRectF connLive = boundConnectorsPanelUnion(m_gesturePickableId.toStdString());
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
    m_handleCount = 0;
    m_modeChipVisible = false;
    emit selectionChromeChanged();
    syncToolCanvasPresence();
    damageToolChrome(toolDirty);
}

void TabletCanvasItem::commitLiveManip()
{
    using namespace epaper::document;
    m_document.applyLiveSmartGeometry(m_gesturePickableId.toStdString(), m_originT, m_originB);
    refreshConnectorsBoundTo(m_document, m_gesturePickableId.toStdString());
    const double dx = m_liveT.x - m_originT.x;
    const double dy = m_liveT.y - m_originT.y;
    const bool resized = m_selGesture == SelGesture::Resize;
    const bool moved = resized || qHypot(dx, dy) >= 2.0
        || qHypot(m_liveT.scaleX - m_originT.scaleX, m_liveT.scaleY - m_originT.scaleY) > 0.01
        || std::abs(m_liveB.width - m_originB.width) > 0.5;
    m_selectionGesture = false;
    m_selGesture = SelGesture::None;
    const QRectF punch = m_originPanelRect.united(m_selectionChromeDirty).united(m_originConnPunch);
    m_originPanelRect = QRectF();
    m_originConnPunch = QRectF();
    m_originConnStrokes.clear();
    if (!moved) {
        m_document.abortGesture();
        m_selectedPickableId = m_gesturePickableId;
        if (!punch.isEmpty())
            update(punch.toAlignedRect());
        refreshSelectionChrome();
        notifyHistory();
        return;
    }
    ++m_toolIntentSeq;
    const std::string opId = std::string("sst-") + std::to_string(m_toolIntentSeq);
    const SmartBounds *bptr = resized ? &m_liveB : nullptr;
    const ApplyResult r = m_document.commitOp(
        makeSetSmartTransformOp(opId, m_gesturePickableId.toStdString(), m_liveT, bptr));
    (void)r;
    refreshAllConnectorWarps(m_document);
    m_selectedPickableId = m_gesturePickableId;
    m_gesturePickableId.clear();
    scheduleVectorRasterize(true);
    m_liveDirtyPrev = QRectF();
    refreshSelectionChrome();
    notifyHistory();
    flushOneWayWire();
}

void TabletCanvasItem::endSelectionGesture()
{
    if (!m_selectionGesture)
        return;
    if (m_selGesture == SelGesture::Marquee || m_selGesture == SelGesture::Lasso) {
        finishMarqueeOrLasso();
        return;
    }
    if (m_selGesture == SelGesture::Move || m_selGesture == SelGesture::Resize) {
        commitLiveManip();
        return;
    }
    m_selectionGesture = false;
    m_selGesture = SelGesture::None;
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
    const DocNode *n = m_document.find(m_gesturePickableId.toStdString());
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
    QPen solid(Qt::black);
    solid.setWidthF(4.0);
    painter->setPen(solid);
    for (const QPointF &pt : pts)
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

    if (m_selectedIds.isEmpty() && m_selectedPickableId.isEmpty() && !m_selectionGesture) {
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
        const QPointF tl = worldToPanel(unionB.x, unionB.y);
        const QPointF br = worldToPanel(unionB.x + unionB.width, unionB.y + unionB.height);
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

QRectF TabletCanvasItem::worldBoundsToPanel(const epaper::document::SmartBounds &wb) const
{
    const QPointF tl = worldToPanel(wb.x, wb.y);
    const QPointF br = worldToPanel(wb.x + wb.width, wb.y + wb.height);
    return QRectF(tl, br).normalized();
}

bool TabletCanvasItem::lodOkPanel(const epaper::document::SmartBounds &wb) const
{
    // @implements [SRS-EP-11] LOD only when zoomed out; scale ≥ 1.0 always manipulable
    if (!viewportZoomedOut())
        return true;
    const QRectF r = worldBoundsToPanel(wb);
    return epaper::document::lodAllowsPanel(r.width(), r.height());
}

bool TabletCanvasItem::viewportZoomedOut() const
{
    if (!m_drawingRegion.valid)
        return false;
    const double rw = m_drawingRegion.maxX - m_drawingRegion.minX;
    const double rh = m_drawingRegion.maxY - m_drawingRegion.minY;
    if (rw <= 0.0 || rh <= 0.0)
        return false;
    const double sx = width() / rw;
    const double sy = height() / rh;
    return std::min(sx, sy) < 1.0 - 1e-6;
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
    using epaper::document::inkSamplesMin;
    using epaper::document::smartLocalToWorld;
    if (node.kind != NodeKind::Ink && node.kind != NodeKind::Primitive)
        return;

    const qreal worldSw = node.style.strokeWidth;
    const double rw = m_drawingRegion.valid ? (m_drawingRegion.maxX - m_drawingRegion.minX)
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
    const QPointF p0 = worldToPanel(conn.warpedSamples[0].x, conn.warpedSamples[0].y);
    qreal minX = p0.x();
    qreal maxX = p0.x();
    qreal minY = p0.y();
    qreal maxY = p0.y();
    for (const auto &s : conn.warpedSamples) {
        const QPointF p = worldToPanel(s.x, s.y);
        minX = qMin(minX, p.x());
        maxX = qMax(maxX, p.x());
        minY = qMin(minY, p.y());
        maxY = qMax(maxY, p.y());
    }
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).normalized().adjusted(-16, -16, 16, 16);
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
        if (!m_strokeActive)
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
