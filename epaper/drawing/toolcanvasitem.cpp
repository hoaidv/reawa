#include "toolcanvasitem.h"
#include "tabletcanvasitem.h"
#include "canvas_session.h"
#include "debug/debug_log_format.hpp"
#include "debug/ui_stall.hpp"
#include "epaperbridge.h"
#include "document/capability.hpp"
#include "document/connector_warp.hpp"
#include "document/manipulate.hpp"
#include "document/surround_create.hpp"
#include "rendering/rendering_qt.hpp"

#include <memory>
#include <unordered_set>

#include <QDebug>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {

QRectF modeChipRect(const QRectF &box)
{
    constexpr qreal w = 120.0;
    constexpr qreal h = 36.0;
    constexpr qreal gap = 32.0;
    return QRectF(box.center().x() - w * 0.5, box.bottom() + gap, w, h);
}

} // namespace

/**
 * =================================================================================================
 * Construction and Qt item lifecycle
 *
 * Painted overlay for selection chrome. Mono-mode region attaches here (ADR-0019).
 * Handlers live in ToolCanvas.qml so the overlay can hide without dropping input.
 * =================================================================================================
 */

/** Transparent painted item; starts hidden until selection chrome needs it. */
ToolCanvasItem::ToolCanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    m_renderer.setAlgorithm(std::make_unique<epaper::render::HierarchyCullAlgorithm>());
    setAntialiasing(false);
    setRenderTarget(QQuickPaintedItem::Image);
    setOpaquePainting(false);
    setFillColor(Qt::transparent);
    setAcceptedMouseButtons(Qt::NoButton);
    setAcceptTouchEvents(false);
    setAcceptHoverEvents(false);
    setVisible(false);
}

/** Delegate to paintToolChrome — never blits the document. */
void ToolCanvasItem::paint(QPainter *painter)
{
    paintToolChrome(painter);
}

/** Pen vs Mono overlay waveform while a marquee/lasso stroke is in flight. */
void ToolCanvasItem::setStrokeWaveform(bool penInFlight)
{
    EpaperBridge::instance()->setOverlayStrokePen(penInFlight);
}

/** Attach Mono-mode region for lasso/marquee waveform. */
void ToolCanvasItem::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    if (!EpaperBridge::instance()->attachMonoModeRegion(this)) {
        qInfo() << "[tool-canvas] Mono attach failed — tight bbox fallback (ADR-0019)";
    }
}

/** Re-attach Mono region on resize (panel size is owned by Tablet → session.frame). */
void ToolCanvasItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        EpaperBridge::instance()->attachMonoModeRegion(this);
    }
}

/**
 * =================================================================================================
 * Host wiring (composition / QML)
 *
 * setSurface stores the Surface API target only. Session is injected separately via setSession.
 * =================================================================================================
 */

/** Bind Tablet Surface API target (ingest / rasterize / wire). */
void ToolCanvasItem::setSurface(TabletCanvasItem *surface)
{
    if (m_surface == surface)
        return;
    m_surface = surface;
    emit surfaceChanged();
    update();
}

/** Connect documentMutated / cameraChanged / exclusiveToolChanged. */
void ToolCanvasItem::setSession(CanvasSession *session)
{
    if (m_session == session)
        return;
    if (m_docConn)
        disconnect(m_docConn);
    if (m_camConn)
        disconnect(m_camConn);
    if (m_toolConn)
        disconnect(m_toolConn);
    m_session = session;
    emit sessionChanged();
    if (!m_session)
        return;
    m_docConn = connect(m_session, &CanvasSession::documentMutated, this,
                        &ToolCanvasItem::onDocumentOrCameraChanged);
    m_camConn = connect(m_session, &CanvasSession::cameraChanged, this,
                        &ToolCanvasItem::onDocumentOrCameraChanged);
    m_toolConn = connect(m_session, &CanvasSession::exclusiveToolChanged, this, [this]() {
        syncToolCanvasPresence();
        refreshSelectionChrome();
    });
}

/** Prune dead selection ids and reproject chrome after session mutation. */
void ToolCanvasItem::onDocumentOrCameraChanged()
{
    // Prune selection if nodes vanished; reproject chrome.
    if (m_session) {
        std::vector<std::string> keep;
        keep.reserve(m_selection.ids.size());
        for (const std::string &id : m_selection.ids) {
            if (doc().find(id))
                keep.push_back(id);
        }
        m_selection.setIds(keep);
        if (!m_selection.pickableId.empty() && !doc().find(m_selection.pickableId)) {
            m_selection.pickableId.clear();
            m_manip.clearNodeId();
        }
    }
    refreshSelectionChrome();
}

/** Pen-near / cancel path → onPointerCancel. */
void ToolCanvasItem::cancelInteraction()
{
    onPointerCancel();
}

/** Drop selection + manip node (finger empty-tap / enclose success). */
void ToolCanvasItem::clearSelection()
{
    m_selection.clear();
    m_manip.clearNodeId();
}

/** Current exclusive tool id from the session chip. */
QString ToolCanvasItem::exclusiveTool() const
{
    return m_session ? m_session->exclusiveTool() : QStringLiteral("pen");
}

/** Session document reference for intention code. */
epaper::document::DeviceDocument &ToolCanvasItem::doc()
{
    Q_ASSERT(m_session);
    return m_session->document;
}

/** Const session document reference. */
const epaper::document::DeviceDocument &ToolCanvasItem::doc() const
{
    Q_ASSERT(m_session);
    return m_session->document;
}

/** Session camera/orientation frame. */
epaper::canvasframe::CanvasFrame &ToolCanvasItem::frame()
{
    Q_ASSERT(m_session);
    return m_session->frame;
}

/** Const session frame. */
const epaper::canvasframe::CanvasFrame &ToolCanvasItem::frame() const
{
    Q_ASSERT(m_session);
    return m_session->frame;
}


/** Pen/finger hit on a resize knob → beginHandleDrag. */
bool ToolCanvasItem::tryBeginHandleAtPanel(const PanelPt &panel, double hitDu)
{
    const int idx = handleIndexAtPanel(panel, hitDu);
    if (idx < 0)
        return false;
    beginHandleDrag(idx, panelToWorld(panel));
    return true;
}

/** Press on canvas in selection tool: pick, move, or marquee/lasso. */
void ToolCanvasItem::beginSelectionGesture(const PanelPt &canvasPos)
{
    // @implements [SRS-EP-11] capability-descriptor gesture route (box / empty leftover)
    using namespace epaper::document;
    m_encloseRefuseReason.clear();
    m_manipUnavailable.clear();
    m_manipUnavailableRect = QRectF();
    const WorldPt world = panelToWorld(canvasPos);

    std::vector<const DocNode *> pick;
    collectPickable(doc().rootChildren, pick);
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

/** Drag while marquee/lasso/move/resize is active. */
void ToolCanvasItem::updateSelectionGesture(const PanelPt &canvasPos)
{
    if (!selectionGestureActive())
        return;
    if (m_selection.gesture == epaper::selection::Gesture::Marquee) {
        applySelectionIntent(m_selection.updateMarquee(canvasPos.x(), canvasPos.y()));
        return;
    }
    if (m_selection.gesture == epaper::selection::Gesture::Lasso) {
        applySelectionIntent(m_selection.updateLasso(canvasPos.x(), canvasPos.y()));
        return;
    }
    applyDragWorld(panelToWorld(canvasPos));
}

/** Release: finish marquee/lasso or commit live manip. */
void ToolCanvasItem::endSelectionGesture()
{
    if (!selectionGestureActive())
        return;
    if (m_selection.gesture == epaper::selection::Gesture::Marquee || m_selection.gesture == epaper::selection::Gesture::Lasso) {
        finishMarqueeOrLasso();
        return;
    }
    if (m_selection.gesture == epaper::selection::Gesture::Move || m_selection.gesture == epaper::selection::Gesture::Resize) {
        commitLiveManip();
        return;
    }
    m_selection.gesture = epaper::selection::Gesture::None;
}


/**
 * =================================================================================================
 * Canvas frame helpers (via session)
 *
 * Panel size lives on session.frame (Tablet syncFramePanelSize is the writer). Camera
 * writes go through applyCameraRegion so Tablet rasterize listens on cameraChanged.
 * =================================================================================================
 */

/** Finger pan/pinch → session.applyCamera (emits cameraChanged). */
void ToolCanvasItem::applyCameraRegion(const epaper::handtouch::WorldAabb &region, bool markValid)
{
    if (!m_session)
        return;
    m_session->applyCamera(region, markValid);
}

/** Panel → world for hit-tests and manip. */
ToolCanvasItem::WorldPt ToolCanvasItem::panelToWorld(const PanelPt &panel) const
{
    Q_ASSERT(m_session);
    const auto w = frame().panelToWorld({panel.x(), panel.y()});
    return {w.x, w.y};
}

/** World → panel for chrome and punches. */
ToolCanvasItem::PanelPt ToolCanvasItem::worldToPanel(double wx, double wy) const
{
    Q_ASSERT(m_session);
    const auto p = frame().worldToPanel(wx, wy);
    return PanelPt(p.x, p.y);
}

/** Panel → UV for one-finger pan through pan-origin. */
ToolCanvasItem::FrameUv ToolCanvasItem::panelToFrameUv(const PanelPt &panel) const
{
    Q_ASSERT(m_session);
    return frame().panelToFrameUv({panel.x(), panel.y()});
}

/** SmartBounds → panel QRectF. */
QRectF ToolCanvasItem::worldBoundsToPanel(const epaper::document::SmartBounds &wb) const
{
    Q_ASSERT(m_session);
    epaper::canvasframe::PanelPt tl, br;
    frame().worldBoundsToPanel(wb.x, wb.y, wb.width, wb.height, &tl, &br);
    return QRectF(QPointF(tl.x, tl.y), QPointF(br.x, br.y)).normalized();
}

/** Gate manip when the box is too small on screen. */
bool ToolCanvasItem::lodOkPanel(const epaper::document::SmartBounds &wb) const
{
    Q_ASSERT(m_session);
    return frame().lodOkPanel(wb.x, wb.y, wb.width, wb.height);
}

/** Session followDirection → FollowDirection for pan-block rules. */
epaper::handtouch::FollowDirection ToolCanvasItem::followEnum() const
{
    return m_session ? epaper::handtouch::parseFollow(m_session->followDirection().toStdString())
                     : epaper::handtouch::FollowDirection::None;
}


/**
 * =================================================================================================
 * Pointer routing and contact arbitration
 *
 * ToolCanvas.qml DragHandler/PinchHandler/TapHandler call these. Pen: Tool decides
 * selection vs ink; only ink goes to Surface ingestPen. Fingers go to hand-touch.
 * =================================================================================================
 */

/** Stylus → selection on Tool or Surface ingestPen; finger → beginFingerTouch. */
void ToolCanvasItem::onPointerStart(qreal x, qreal y, qreal pressure, bool pen)
{
    // Qt already decided this point is not chrome — no rect hit-test here.
    // @implements [SRS-EP-04] canvas pointer entry
    if (!m_surface)
        return;
    const PanelPt panel(x, y);
    if (pen) {
        // Handle / selection stay on Tool; only ink uses Tablet ingest.
        if (tryBeginHandleAtPanel(panel, epaper::document::kHandleHitDu))
            return;
        if (selectionToolArmed()) {
            beginSelectionGesture(panel);
            return;
        }
        TabletCanvasItem::RawPt raw;
        IngestChannels ch = m_surface->stashedChannels(panel, &raw);
        ch.pressure = pressure;
        m_surface->ingestPen(QEvent::TabletPress, panel, raw, ch);
        return;
    }
    if (m_finger.lockedUntilLift)
        return;
    beginFingerTouch(panel);
}

/** Stylus move or one-finger update (ignore during two-finger). */
void ToolCanvasItem::onPointerMove(qreal x, qreal y, qreal pressure, bool pen)
{
    if (!m_surface)
        return;
    if (m_finger.isTwoFinger())
        return;
    const PanelPt panel(x, y);
    if (pen) {
        if (selectionGestureActive()) {
            updateSelectionGesture(panel);
            return;
        }
        TabletCanvasItem::RawPt raw;
        IngestChannels ch = m_surface->stashedChannels(panel, &raw);
        ch.pressure = pressure;
        m_surface->ingestPen(QEvent::TabletMove, panel, raw, ch);
        return;
    }
    updateFingerTouch(panel, 1);
}

/** Stylus release or one-finger end. */
void ToolCanvasItem::onPointerEnd(qreal x, qreal y, bool pen)
{
    if (!m_surface)
        return;
    const PanelPt panel(x, y);
    if (pen) {
        if (selectionGestureActive()) {
            endSelectionGesture();
            m_surface->clearStash();
            return;
        }
        TabletCanvasItem::RawPt raw;
        const IngestChannels ch = m_surface->stashedChannels(panel, &raw);
        m_surface->ingestPen(QEvent::TabletRelease, panel, raw, ch);
        m_surface->clearStash();
        return;
    }
    // PinchHandler owns two-finger; the one-finger handler deactivates on takeover.
    if (m_finger.isTwoFinger())
        return;
    // Cleared by the contact counter, not here: a finger still on glass after a
    // pinch must not re-arm just because the drag handler cycled.
    if (m_finger.lockedUntilLift)
        return;
    endFingerTouch(panel);
}

/** Stationary tap: begin+end finger touch (select/deselect). */
void ToolCanvasItem::onFingerTap(qreal x, qreal y)
{
    if (m_finger.lockedUntilLift)
        return;
    const PanelPt panel(x, y);
    beginFingerTouch(panel);
    endFingerTouch(panel);
}

/** Abort stroke or selection gesture; cancel hand touch. */
void ToolCanvasItem::onPointerCancel()
{
    if (m_surface && m_surface->strokeActive())
        m_surface->cancelActiveStroke();
    else if (selectionGestureActive())
        endSelectionGesture();
    cancelHandTouch();
    if (m_surface)
        m_surface->clearStash();
    m_finger.lockedUntilLift = false;
}

/** Second finger down: abort one-finger manip, lock until lift. */
void ToolCanvasItem::onSecondContact()
{
    const bool manip = m_finger.isLiveManip() || m_manip.active;
    applyFingerIntent(m_finger.secondContact(m_manip.active));
    qInfo().noquote() << QStringLiteral("[hand] second contact manip=%1").arg(manip ? 1 : 0);
}

/** All contacts up: clear finger lock state. */
void ToolCanvasItem::onContactsCleared()
{
    m_finger.contactsCleared();
}

/** Two-finger navigation start (abort live manip if needed). */
void ToolCanvasItem::onPinchStart(qreal x, qreal y, qreal scale)
{
    // Two fingers are always navigation, whatever sits under them: the one-finger
    // handler owns the first contact and may already have grabbed a node.
    // @implements [SRS-EP-24] PinchHandler two-finger pan pinch
    if (!m_finger.armed || m_finger.gesture == epaper::fingergesture::Kind::Chip) {
        m_pinchIgnore = true;
        return;
    }
    if (m_finger.isLiveManip() || m_manip.active)
        abortFingerManip();
    m_finger.clearGestureForPinch();
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

/** Pinch centroid/scale → updateTwoFingerTouch. */
void ToolCanvasItem::onPinchUpdate(qreal x, qreal y, qreal scale)
{
    if (m_pinchIgnore)
        return;
    updateTwoFingerTouch(pinchArmPoint(x, y, scale, true),
                         pinchArmPoint(x, y, scale, false));
}

/** End two-finger pan/pinch. */
void ToolCanvasItem::onPinchEnd()
{
    if (!m_pinchIgnore)
        endTwoFingerTouch();
    m_pinchIgnore = false;
}

/** Synthetic contact pair around pinch centroid for UV math. */
ToolCanvasItem::PanelPt ToolCanvasItem::pinchArmPoint(qreal x, qreal y, qreal scale,
                                                          bool positive) const
{
    const qreal s0 = m_pinchScale0 > 0.01 ? m_pinchScale0 : 1.0;
    const qreal arm = m_pinchArm * (scale / s0);
    return PanelPt(x + (positive ? arm : -arm), y);
}


/**
 * =================================================================================================
 * Hand touch
 *
 * FingerGestureMachine decides knob / box / empty / pan / two-finger. applyFingerIntent
 * runs Surface/session effects (camera, selection, arm sel_freeform).
 * =================================================================================================
 */

/** Armed one-finger down: classify hit and start machine. */
bool ToolCanvasItem::beginFingerTouch(const PanelPt &canvasPos)
{
    using namespace epaper::handtouch;
    using namespace epaper::fingergesture;
    if (!m_finger.armed)
        return false;
    const bool knob = handleIndexAtPanel(canvasPos, kFingerHandleHitDu) >= 0;
    const bool box = fingerHitsBox(canvasPos);
    qInfo().noquote() << QStringLiteral("[hand] down (%1,%2) knob=%3 box=%4")
                             .arg(int(canvasPos.x()))
                             .arg(int(canvasPos.y()))
                             .arg(knob ? 1 : 0)
                             .arg(box ? 1 : 0);

    m_surface->ensureLocalDrawingRegion();
    const FingerResult r =
        m_finger.begin(canvasPos.x(), canvasPos.y(), knob, box, frame().drawingRegion,
                       panelToWorld(canvasPos));
    if (!r.accepted)
        return false;
    applyFingerIntent(r, canvasPos);
    return true;
}

/** One-finger move; may promote empty-pending → pan. */
void ToolCanvasItem::updateFingerTouch(const PanelPt &canvasPos, int fingerCount)
{
    using namespace epaper::fingergesture;
    if (m_finger.ignoresOneFingerUpdate())
        return;
    if (m_finger.isLiveManip()) {
        updateSelectionGesture(canvasPos);
        return;
    }
    double nowX = 0;
    double nowY = 0;
    worldThroughPanOrigin(canvasPos, &nowX, &nowY);
    const Kind before = m_finger.gesture;
    bool previewDue = !m_fingerPanClock.isValid()
        || m_fingerPanClock.elapsed() >= kSelectionGhostMinIntervalMs;
    // First frame after palm-travel promote must publish (clock was idle on pending).
    if (before == Kind::EmptyPending)
        previewDue = true;
    const FingerResult r = m_finger.update(canvasPos.x(), canvasPos.y(), fingerCount, followEnum(),
                                           previewDue, nowX, nowY);
    if (before == Kind::EmptyPending && m_finger.gesture == Kind::EmptyPan) {
        m_fingerPanClock.invalidate();
        qInfo().noquote() << QStringLiteral("[hand] pan promote at (%1,%2)")
                                 .arg(int(canvasPos.x()))
                                 .arg(int(canvasPos.y()));
    }
    if (has(r.intent, FingerIntent::PublishViewportLive))
        m_fingerPanClock.restart();
    applyFingerIntent(r, canvasPos);
}

/** One-finger up: select, deselect, or settle pan. */
void ToolCanvasItem::endFingerTouch(const PanelPt &canvasPos)
{
    using namespace epaper::fingergesture;
    const Kind g = m_finger.gesture;
    qInfo().noquote() << QStringLiteral("[hand] up gesture=%1 travel=%2")
                             .arg(int(g))
                             .arg(int(epaper::handtouch::travelDu(
                                 canvasPos.x() - m_finger.downPanel.x,
                                 canvasPos.y() - m_finger.downPanel.y)));

    double nowX = 0;
    double nowY = 0;
    worldThroughPanOrigin(canvasPos, &nowX, &nowY);
    applyFingerIntent(m_finger.end(canvasPos.x(), canvasPos.y(), nowX, nowY), canvasPos);
}

/** Revert live manip geometry (second contact / cancel). */
void ToolCanvasItem::abortFingerManip()
{
    if (m_manip.active) {
        const std::string id = m_manip.nodeId;
        const epaper::manip::ManipResult r = m_manip.abort();
        // Clear live gesture before apply so RefreshChrome restores settled knobs.
        m_selection.gesture = epaper::selection::Gesture::None;
        m_selection.pickableId = id;
        applyManipIntent(r, /*restoreOrigin=*/true);
        m_manip.reset();
    } else {
        m_selection.gesture = epaper::selection::Gesture::None;
    }
    m_liveDirtyPrev = QRectF();
    m_finger.gesture = epaper::fingergesture::Kind::None;
}

/** Two panel contacts → UV pair in the current frame. */
epaper::handtouch::TwoFingerContacts ToolCanvasItem::uvPair(const PanelPt &a, const PanelPt &b) const
{
    const FrameUv ua = panelToFrameUv(a);
    const FrameUv ub = panelToFrameUv(b);
    return epaper::handtouch::TwoFingerContacts{ua.u, ua.v, ub.u, ub.v};
}

/** Start two-finger pan/pinch on the local camera. */
bool ToolCanvasItem::beginTwoFingerTouch(const PanelPt &a, const PanelPt &b)
{
    using namespace epaper::fingergesture;
    m_surface->ensureLocalDrawingRegion();
    const FingerResult r =
        m_finger.beginTwo(a.x(), a.y(), b.x(), b.y(), frame().drawingRegion, uvPair(a, b),
                          followEnum());
    if (!r.accepted)
        return false;
    m_fingerPanClock.invalidate();
    applyFingerIntent(r);
    return true;
}

/** Live two-finger map update + optional viewport publish. */
void ToolCanvasItem::updateTwoFingerTouch(const PanelPt &a, const PanelPt &b)
{
    using namespace epaper::fingergesture;
    if (!m_finger.isTwoFinger())
        return;
    const bool previewDue = !m_fingerPanClock.isValid()
        || m_fingerPanClock.elapsed() >= kSelectionGhostMinIntervalMs;
    const FingerResult r =
        m_finger.updateTwo(a.x(), a.y(), b.x(), b.y(), uvPair(a, b), previewDue);
    if (previewDue && has(r.intent, FingerIntent::PublishViewportLive))
        m_fingerPanClock.restart();
    applyFingerIntent(r);
}

/** Settle two-finger camera. */
void ToolCanvasItem::endTwoFingerTouch()
{
    applyFingerIntent(m_finger.endTwo());
}

/** ToolChip hand-touch toggle; cancel if disarming. */
void ToolCanvasItem::toggleHandTouch()
{
    // @implements [SRS-EP-22] btn.hand_touch kill-switch
    m_finger.setArmed(!m_finger.armed);
    if (!m_finger.armed)
        cancelHandTouch();
    emit handTouchArmedChanged();
    qInfo().noquote() << (m_finger.armed ? QStringLiteral("[hand] toggle on")
                                         : QStringLiteral("[hand] toggle off"));
}

/** Pen-near or escape: abort machine + manip. */
void ToolCanvasItem::cancelHandTouch()
{
    // Full hand-touch reset, lock included: whoever cancels must not have to know
    // that a lifted-contact latch exists.
    if (m_finger.isTwoFinger()) {
        endTwoFingerTouch(); // settles the viewport; leaves lock-until-lift set
        return;
    }
    applyFingerIntent(m_finger.cancel(m_manip.active));
}

/** FingerIntent bits → Surface/session/selection effects. */
void ToolCanvasItem::applyFingerIntent(const epaper::fingergesture::FingerResult &r,
                                         const PanelPt &panel)
{
    using epaper::fingergesture::FingerIntent;
    using epaper::fingergesture::has;
    if (has(r.intent, FingerIntent::ArmSelFreeform) && m_session)
        m_session->setExclusiveTool(QStringLiteral("sel_freeform"));
    if (has(r.intent, FingerIntent::BeginHandleResize))
        tryBeginHandleAtPanel(panel, epaper::handtouch::kFingerHandleHitDu);
    if (has(r.intent, FingerIntent::BeginSelectMove))
        beginSelectionGesture(panel);
    if (has(r.intent, FingerIntent::UpdateSelection))
        updateSelectionGesture(panel);
    if (has(r.intent, FingerIntent::ApplyCameraRegion) && r.hasRegion)
        applyCameraRegion(r.region, false);
    if (has(r.intent, FingerIntent::PublishViewportLive))
        m_surface->maybePublishLocalViewport(false);
    if (has(r.intent, FingerIntent::PublishViewportSettle))
        m_surface->maybePublishLocalViewport(true);
    if (has(r.intent, FingerIntent::ScheduleRasterizeLive))
        m_surface->scheduleDocumentRasterize(false);
    if (has(r.intent, FingerIntent::ScheduleRasterizeSettle))
        m_surface->scheduleDocumentRasterize(true);
    // Clear before RefreshChrome — empty-tap deselect packs both bits.
    if (has(r.intent, FingerIntent::ClearSelection)) {
        clearSelection();
        m_selection.gesture = epaper::selection::Gesture::None;
    }
    if (has(r.intent, FingerIntent::RefreshChrome))
        refreshSelectionChrome();
    if (has(r.intent, FingerIntent::EndSelectionGesture))
        endSelectionGesture();
    if (has(r.intent, FingerIntent::AbortManip))
        abortFingerManip();
    // LockUntilLift is already stored on m_finger by the machine.
    Q_UNUSED(FingerIntent::LockUntilLift);
}

/** Map contact through pan-origin region (one-finger pan). */
void ToolCanvasItem::worldThroughPanOrigin(const PanelPt &panel, double *wx, double *wy) const
{
    using epaper::handtouch::mapUvToWorld;
    const FrameUv uv = panelToFrameUv(panel);
    mapUvToWorld(m_finger.panOrigin.box(), uv.u, uv.v, wx, wy);
}

/** Whether panel point hits a local SmartGroup AABB. */
bool ToolCanvasItem::fingerHitsBox(const PanelPt &canvasPos) const
{
    using namespace epaper::document;
    const WorldPt world = panelToWorld(canvasPos);
    const QString id = hitLocalSmartGroup(world);
    if (id.isEmpty())
        return false;
    const DocNode *n = doc().find(id.toStdString());
    SmartBounds wb;
    if (!n || !boundsOf(*n, wb))
        return false;
    return lodOkPanel(wb);
}


/**
 * =================================================================================================
 * Selection and direct manipulation
 *
 * SelectionSession / ManipSession return intents; sinks call Surface (rasterize, punch,
 * manip preview, wire) and mutate the session document.
 * =================================================================================================
 */

/** True when exclusive tool is sel_rect or sel_freeform. */
bool ToolCanvasItem::isSelectionTool() const
{
    const QString m = exclusiveTool();
    return m == QLatin1String("sel_rect") || m == QLatin1String("sel_freeform");
}

/** Replace selected node id list. */
void ToolCanvasItem::setSelection(const std::vector<std::string> &ids)
{
    m_selection.setIds(ids);
}

/** SelectionIntent → chrome damage / waveform / refresh. */
void ToolCanvasItem::applySelectionIntent(const epaper::selection::SelectionResult &r)
{
    using epaper::selection::SelectionIntent;
    using epaper::selection::has;
    if (has(r.intent, SelectionIntent::ResetDrag))
        m_manip.reset();
    if (has(r.intent, SelectionIntent::StrokeWaveformOn) )
        setStrokeWaveform(true);
    if (has(r.intent, SelectionIntent::StrokeWaveformOff) )
        setStrokeWaveform(false);
    if (has(r.intent, SelectionIntent::ChromeChanged)) {
        m_encloseVisible = false;
        m_handleCount = 0;
        m_modeChipVisible = false;
        emit selectionChromeChanged();
    }
    if (has(r.intent, SelectionIntent::SyncToolCanvas))
        syncToolCanvasPresence();
    if (has(r.intent, SelectionIntent::DamageLive) && r.hasDamage) {
        const QRectF live =
            QRectF(QPointF(r.damageA.x, r.damageA.y), QPointF(r.damageB.x, r.damageB.y))
                .normalized()
                .adjusted(-8, -8, 8, 8);
        damageToolChrome(live);
    }
    if (has(r.intent, SelectionIntent::DamageSegment) && r.hasDamage) {
        const QRectF seg =
            QRectF(QPointF(r.damageA.x, r.damageA.y), QPointF(r.damageB.x, r.damageB.y))
                .normalized()
                .adjusted(-8, -8, 8, 8);
        damageToolChromeSegment(seg);
    }
    if (has(r.intent, SelectionIntent::DebugChanged) && !r.debugInfo.empty()) {
        m_surface->setInteractionDebug(QString::fromStdString(r.debugInfo));
    }
    if (has(r.intent, SelectionIntent::RefreshChrome))
        refreshSelectionChrome();
}

/** ManipIntent → live geometry, commit op, preview, wire. */
void ToolCanvasItem::applyManipIntent(const epaper::manip::ManipResult &r, bool restoreOrigin)
{
    using epaper::manip::ManipIntent;
    using epaper::manip::has;
    if (has(r.intent, ManipIntent::BeginGesture))
        doc().beginGesture();
    if (has(r.intent, ManipIntent::ApplyLiveGeometry)) {
        if (restoreOrigin)
            doc().applyLiveSmartGeometry(m_manip.nodeId, m_manip.originT, m_manip.originB);
        else
            doc().applyLiveSmartGeometry(m_manip.nodeId, m_manip.liveT, m_manip.liveB);
    }
    if (has(r.intent, ManipIntent::RefreshBoundConnectors))
        refreshConnectorsBoundTo(doc(), m_manip.nodeId);
    if (has(r.intent, ManipIntent::PreviewFrame))
        doc().previewManipulationFrame();
    if (has(r.intent, ManipIntent::SendPreview))
        sendManipPreviewToInfini();
    if (has(r.intent, ManipIntent::RefreshAllConnectors)
        && !has(r.intent, ManipIntent::CommitTransform))
        refreshAllConnectorWarps(doc());
    if (has(r.intent, ManipIntent::AbortGesture))
        doc().abortGesture();
    if (has(r.intent, ManipIntent::CommitTransform)) {
        ++m_toolIntentSeq;
        const std::string opId = std::string("sst-") + std::to_string(m_toolIntentSeq);
        const epaper::document::SmartBounds liveB = m_manip.liveB;
        const epaper::document::SmartBounds *bptr = r.resized ? &liveB : nullptr;
        doc().commitOp(
            epaper::document::makeSetSmartTransformOp(opId, m_manip.nodeId, m_manip.liveT, bptr));
        m_originPanelRect = QRectF();
        if (m_session)
            m_session->clearLiveManipSuppressIds();
    }
    if (has(r.intent, ManipIntent::RefreshAllConnectors)
        && has(r.intent, ManipIntent::CommitTransform))
        refreshAllConnectorWarps(doc());
    if (has(r.intent, ManipIntent::ScheduleRasterize) && m_session) {
        if (has(r.intent, ManipIntent::AbortGesture))
            m_session->clearLiveManipSuppressIds();
        m_session->noteDocumentMutated();
    }
    if (has(r.intent, ManipIntent::RefreshChrome))
        refreshSelectionChrome();
    if (has(r.intent, ManipIntent::Redraw))
        redrawLiveManipRegion();
    if (has(r.intent, ManipIntent::NotifyHistory))
        m_surface->notifyHistory();
    if (has(r.intent, ManipIntent::FlushWire))
        m_surface->flushWire();
}

/** Topmost SmartGroup under a world point. */
QString ToolCanvasItem::hitLocalSmartGroup(WorldPt world) const
{
    using namespace epaper::document;
    std::vector<const DocNode *> pick;
    collectPickable(doc().rootChildren, pick);
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

/** cta.enclose: create SmartGroup from selection or refuse. */
void ToolCanvasItem::encloseSelection()
{
    // @implements [SRS-EP-10] cta.enclose selection-create (never on pen-up)
    using namespace epaper::document;
    if (!m_encloseVisible)
        return;
    const SelectionCreateResult r = createSmartGroupFromSelection(doc(), m_selection.ids);
    // @fix leftover selection chrome after cta.enclose (stale pickable AABB)
    if (!r.created) {
        m_encloseRefuseReason = r.reason == "smartgroup_in_selection"
            ? QStringLiteral("Cannot enclose a Smart Group")
            : QStringLiteral("No surrounding stroke");
        const std::string line = epaper::debuglog::formatEncloseLog(
            "OrdinaryInk", r.reason, "", {});
        qInfo().noquote() << QString::fromStdString(line);
        if (m_surface)
            m_surface->setInteractionDebug(QString::fromStdString(line));
        emit selectionChromeChanged();
        damageToolChrome(m_selectionChromeDirty);
        return;
    }
    const std::string line = epaper::debuglog::formatEncloseLog(
        "Created", "", r.smartGroupId, r.childIds);
    qInfo().noquote() << QString::fromStdString(line);
    if (m_surface)
        m_surface->setInteractionDebug(QString::fromStdString(line));
    clearSelection();
    m_encloseVisible = false;
    m_encloseCtaRect = QRectF();
    m_encloseRefuseReason.clear();
    m_selection.gesture = epaper::selection::Gesture::None;
    refreshSelectionChrome();
    if (m_session)
        m_session->noteDocumentMutated();
    m_surface->notifyHistory();
    m_surface->flushWire();
}

/** Start rect or freeform gesture from exclusive tool. */
void ToolCanvasItem::beginMarqueeOrLasso(const PanelPt &canvasPos)
{
    applySelectionIntent(m_selection.beginMarqueeOrLasso(
        canvasPos.x(), canvasPos.y(), exclusiveTool() == QLatin1String("sel_freeform")));
}

/** Commit marquee/lasso hit-test into selection. */
void ToolCanvasItem::finishMarqueeOrLasso()
{
    applySelectionIntent(m_selection.finish(
        kMinMarqueeGesture, doc(),
        [this](double px, double py, double *wx, double *wy) {
            const WorldPt w = panelToWorld(PanelPt(px, py));
            *wx = w.x;
            *wy = w.y;
        }));
}

/** Begin move/resize; suppress subtree on Tablet and rasterize. */
void ToolCanvasItem::startLiveManip(const epaper::document::DocNode *subject,
                                     epaper::document::ResizeHandle handle, WorldPt world)
{
    using namespace epaper::document;
    m_selection.setIds({subject->id});
    m_selection.gesture =
        handle == ResizeHandle::None ? epaper::selection::Gesture::Move
                                     : epaper::selection::Gesture::Resize;
    const epaper::manip::ManipResult r =
        m_manip.begin(subject->id, handle, world, subject->transform, subject->smartBounds);
    m_selectionGhostClock.invalidate();
    m_liveDirtyPrev = QRectF();
    m_selectionChromeDirty = QRectF();
    SmartBounds originWorld;
    if (boundsOf(*subject, originWorld))
        m_originPanelRect = worldBoundsToPanel(originWorld).adjusted(-8, -8, 8, 8);
    else
        m_originPanelRect = QRectF();
    if (m_session) {
        std::unordered_set<std::string> suppress;
        epaper::render::collectManipSuppressIds(doc(), subject->id, &suppress);
        m_session->setLiveManipSuppressIds(std::move(suppress));
    }
    applyManipIntent(r);
}

/** Live manip sample (throttled preview to Infini). */
void ToolCanvasItem::applyDragWorld(WorldPt world)
{
    if (!m_manip.active)
        return;
    epaper::UiStallSection stall("applyDragWorld");
    const bool previewDue = !m_selectionGhostClock.isValid()
        || m_selectionGhostClock.elapsed() >= kSelectionGhostMinIntervalMs;
    const epaper::document::DocNode *n = doc().find(m_manip.nodeId);
    const std::string mode = n ? n->inkScaleMode : std::string("withBounds");
    applyManipIntent(m_manip.apply(world, mode, previewDue));
    if (previewDue)
        m_selectionGhostClock.restart();
}

/** Resize from a specific handle index. */
void ToolCanvasItem::beginHandleDrag(int handleIndex, WorldPt world)
{
    using namespace epaper::document;
    const ResizeHandle handle = epaper::manip::handleFromIndex(handleIndex);
    if (handle == ResizeHandle::None)
        return;
    const DocNode *selected = doc().find(m_selection.pickableId);
    if (!selected || !descriptorFor(selected->kind).has(Verb::Resize))
        return;
    SmartBounds wb;
    if (!boundsOf(*selected, wb))
        return;
    if (!lodOkPanel(wb)) {
        showManipUnavailable(wb);
        return;
    }
    m_finger.gesture = epaper::fingergesture::Kind::Resize;
    startLiveManip(selected, handle, world);
}

/** Toggle Keep size / Scale ink on the selected SmartGroup. */
void ToolCanvasItem::tapModeChip()
{
    using namespace epaper::document;
    const DocNode *selected = doc().find(m_selection.pickableId);
    if (!selected || !descriptorFor(selected->kind).has(Verb::SetInkScaleMode))
        return;
    SmartBounds wb;
    if (boundsOf(*selected, wb) && !lodOkPanel(wb)) {
        showManipUnavailable(wb);
        return;
    }
    const std::string next = selected->inkScaleMode == "fixedInk" ? "withBounds" : "fixedInk";
    static int seq = 0;
    doc().commitOp(makeSetInkScaleModeOp(std::string("ism-") + std::to_string(++seq),
                                              selected->id, next));
    if (m_session)
        m_session->noteDocumentMutated();
    refreshSelectionChrome();
    m_surface->notifyHistory();
    m_surface->flushWire();
}

/** Dirty Tool chrome during live manip. */
void ToolCanvasItem::redrawLiveManipRegion()
{
    using namespace epaper::document;
    SmartBounds wb;
    const DocNode *n = doc().find(m_manip.nodeId);
    QRectF liveBounds;
    QRectF next;
    if (n && boundsOf(*n, wb)) {
        liveBounds = worldBoundsToPanel(wb);
        next = liveBounds.adjusted(-12, -12, 12, 48);
    }
    const QRectF connLive = m_surface ? m_surface->boundConnectorsPanelUnion(m_manip.nodeId)
                                      : QRectF();
    if (!connLive.isEmpty())
        next = next.isEmpty() ? connLive : next.united(connLive);
    const QRectF toolDirty = m_liveDirtyPrev.isNull()
        ? next.united(m_originPanelRect)
        : m_liveDirtyPrev.united(next);
    m_liveDirtyPrev = next;
    m_selectionChromeDirty = m_originPanelRect;
    if (!liveBounds.isEmpty())
        m_selectionBoundsRect = liveBounds;
    const bool holdKnobs = m_manip.active && m_manip.resizing();
    if (!holdKnobs)
        m_handleCount = 0;
    m_modeChipVisible = false;
    emit selectionChromeChanged();
    syncToolCanvasPresence();
    damageToolChrome(toolDirty);
}

/** Commit transform op via applyManipIntent. */
void ToolCanvasItem::commitLiveManip()
{
    if (!m_manip.active)
        return;
    const std::string id = m_manip.nodeId;
    const epaper::manip::ManipResult r = m_manip.commit();
    // Clear live gesture before apply so RefreshChrome can restore settled knobs.
    m_selection.gesture = epaper::selection::Gesture::None;
    m_selection.pickableId = id;
    if (m_selection.ids.empty())
        m_selection.ids.push_back(id);
    applyManipIntent(r, /*restoreOrigin=*/!r.moved);
    m_manip.reset();
    m_liveDirtyPrev = QRectF();
}

/** Which of the 8 knobs contains the panel point. */
int ToolCanvasItem::handleIndexAtPanel(const PanelPt &panel, double hitDu) const
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

/** Surface publishManipPreview for live ghost on host. */
void ToolCanvasItem::sendManipPreviewToInfini()
{
    if (!m_surface)
        return;
    const epaper::document::SmartBounds *bptr =
        m_selection.gesture == epaper::selection::Gesture::Resize ? &m_manip.liveB : nullptr;
    m_surface->publishManipPreview(m_manip.nodeId, m_manip.liveT, bptr);
}

/** LOD refuse banner when manip is blocked. */
void ToolCanvasItem::showManipUnavailable(const epaper::document::SmartBounds &wb)
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

/**
 * =================================================================================================
 * Selection chrome overlay
 *
 * Q_PROPERTY knobs/enclose/mode-chip for Main.qml. paintToolChrome draws
 * marquee/lasso/AABB and live manip ghost; syncToolCanvasPresence shows the Mono overlay
 * only while needed.
 * =================================================================================================
 */

/** Recompute knob/enclose/mode-chip rects from selection. */
void ToolCanvasItem::refreshSelectionChrome()
{
    using namespace epaper::document;
    m_encloseRefuseReason.clear();
    SmartBounds unionB;
    const std::vector<std::string> &ids = m_selection.ids;
    QRectF bounds;
    if (!ids.empty() && unionAabbOfIds(doc(), ids, unionB)) {
        const PanelPt tl = worldToPanel(unionB.x, unionB.y);
        const PanelPt br = worldToPanel(unionB.x + unionB.width, unionB.y + unionB.height);
        bounds = QRectF(tl, br).normalized();
    }
    m_encloseVisible = isSelectionTool() && ids.size() >= 2 && !m_selection.isMarqueeOrLasso();
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
    if (isSelectionTool() && !ids.empty() && !bounds.isEmpty() && !m_selection.isMarqueeOrLasso()
        && !m_selection.isLiveManip()) {
        const DocNode *one = ids.size() == 1 ? doc().find(ids[0]) : nullptr;
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

/** Soft update union of previous and next chrome dirty. */
void ToolCanvasItem::damageToolChrome(const QRectF &next)
{
    QRectF u = m_toolChromePrev.isNull() ? next : m_toolChromePrev.united(next);
    m_toolChromePrev = next;
    if ( u.isEmpty())
        return;
    update(u.toAlignedRect().adjusted(-8, -8, 8, 8));
}

/** Expand dirty for a marquee/lasso segment. */
void ToolCanvasItem::damageToolChromeSegment(const QRectF &seg)
{
    m_toolChromePrev = m_toolChromePrev.united(seg);
    if ( seg.isEmpty())
        return;
    update(seg.toAlignedRect());
}

/** Show overlay for selection chrome / live manip / stroke. */
void ToolCanvasItem::syncToolCanvasPresence()
{
    const bool liveManip = m_selection.isLiveManip();
    const bool strokeChrome = m_selection.isMarqueeOrLasso();
    const bool settled = isSelectionTool() && !m_selection.ids.empty() && !liveManip && !strokeChrome;
    const bool on = isSelectionTool() && (strokeChrome || liveManip || settled);
    setVisible(on);
    if (on && !strokeChrome)
        setStrokeWaveform(false);
}

/** Ghost subtree + dotted bounds (+ knobs while resizing). */
void ToolCanvasItem::paintLiveManipOnToolCanvas(QPainter *painter)
{
    using namespace epaper::document;
    const DocNode *n = doc().find(m_manip.nodeId);
    if (!n)
        return;

    epaper::render::FrameProjector proj;
    proj.frame = &frame();
    epaper::render::RenderRequest req;
    req.sharp = true;
    epaper::render::QPainterPixelSink sink(painter);
    m_renderer.renderSubtree(doc(), proj, req, m_manip.nodeId, sink);

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

    // Knobs + mode chip only while resizing — move chrome is the bounds rect alone.
    if (!m_manip.resizing())
        return;

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

/** Marquee, lasso, settled AABB, or live manip chrome. */
void ToolCanvasItem::paintToolChrome(QPainter *painter)
{
    // @implements [SRS-EP-12] ovl.marquee / ovl.lasso / ovl.nodes_bounds
    if (!isSelectionTool())
        return;

    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (m_selection.gesture == epaper::selection::Gesture::Move || m_selection.gesture == epaper::selection::Gesture::Resize) {
        paintLiveManipOnToolCanvas(painter);
        painter->restore();
        return;
    }

    QPen dotted(Qt::black);
    dotted.setWidthF(3.0);
    dotted.setStyle(Qt::DotLine);
    painter->setBrush(Qt::NoBrush);

    if (m_selection.gesture == epaper::selection::Gesture::Marquee) {
        painter->setPen(dotted);
        painter->drawRect(QRectF(QPointF(m_selection.marqueeStart.x, m_selection.marqueeStart.y),
                                 QPointF(m_selection.marqueeEnd.x, m_selection.marqueeEnd.y))
                              .normalized());
        painter->restore();
        return;
    }
    if (m_selection.gesture == epaper::selection::Gesture::Lasso && m_selection.lasso.size() >= 2) {
        painter->setPen(dotted);
        QPainterPath path;
        path.moveTo(QPointF(m_selection.lasso.front().x, m_selection.lasso.front().y));
        for (size_t i = 1; i < m_selection.lasso.size(); ++i)
            path.lineTo(QPointF(m_selection.lasso[i].x, m_selection.lasso[i].y));
        painter->drawPath(path);
        painter->restore();
        return;
    }

    if (m_selection.ids.empty() && m_selection.pickableId.empty() && !selectionGestureActive()) {
        painter->restore();
        return;
    }

    using namespace epaper::document;
    std::vector<std::string> ids = m_selection.ids;
    if (ids.empty() && !m_selection.pickableId.empty())
        ids.push_back(m_selection.pickableId);

    SmartBounds unionB;
    QRectF r;
    if (unionAabbOfIds(doc(), ids, unionB)) {
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

