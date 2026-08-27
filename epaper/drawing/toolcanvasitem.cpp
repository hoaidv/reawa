#include "toolcanvasitem.h"

#include "canvas_session.h"
#include "epaperbridge.h"
#include "tabletcanvasitem.h"
#include "tools/finger_host.hpp"
#include "tools/manip_host.hpp"
#include "tools/selection_stroke_host.hpp"
#include "tools/operations/move_operation.hpp"
#include "tools/operations/navigation_operation.hpp"
#include "tools/operations/resize_operation.hpp"
#include "tools/operations/select_operation.hpp"

#include <QDebug>
#include <QEvent>

/**
 * ToolCanvasItem — Qt entry + interaction router (ADR-0033).
 *
 * Owns session bags (selection, manip, finger machine) and delegates effects to
 * DocContext / ToolContext / InputHub. Q_PROPERTY getters below are pure forwarders
 * to ToolChrome state and are intentionally uncommented.
 */

/** Constructor — QQuickPaintedItem overlay setup; attach SelectionContext; sync HandTouch armed.
 *  Simplify: no — Qt item policy belongs here until a thin QML shell wraps a C++ router. */
ToolCanvasItem::ToolCanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    m_selCtx.attach(&m_selection);
    m_hub.handTouch().setArmed(m_finger.armed);
    setAntialiasing(false);
    setRenderTarget(QQuickPaintedItem::Image);
    setOpaquePainting(false);
    setFillColor(Qt::transparent);
    setAcceptedMouseButtons(Qt::NoButton);
    setAcceptTouchEvents(false);
    setAcceptHoverEvents(false);
    setVisible(false);
}

void ToolCanvasItem::paint(QPainter *painter)
{
    if (m_toolCtx)
        m_toolCtx->paintOverlay(painter);
}

/** ADR-0019 waveform — toggles Pen vs Mono overlay mode on EPScreen via EpaperBridge.
 *  Simplify: no — bridge is device/Qt integration, not ToolContext chrome. */
void ToolCanvasItem::setStrokeWaveform(bool penInFlight)
{
    EpaperBridge::instance()->setOverlayStrokePen(penInFlight);
}

/** ADR-0019 — register this item as the Mono-mode damage region for lasso/marquee waveform.
 *  Simplify: no — EpaperBridge attachment is Qt/device glue. */
void ToolCanvasItem::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    if (!EpaperBridge::instance()->attachMonoModeRegion(this))
        qInfo() << "[tool-canvas] Mono attach failed — tight bbox fallback (ADR-0019)";
}

/** Re-attach Mono region when panel geometry changes (Tablet owns frame panel size).
 *  Simplify: no — same as componentComplete; stays on Qt item. */
void ToolCanvasItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        EpaperBridge::instance()->attachMonoModeRegion(this);
}

/** Inject Tablet Surface (ink/rasterize/wire) and rebuild tool host wiring.
 *  Simplify: partially — could become `m_hostBinder.onSurfaceChanged(surface)` one-liner. */
void ToolCanvasItem::setSurface(TabletCanvasItem *surface)
{
    if (m_surface == surface)
        return;
    m_surface = surface;
    syncToolHost();
    emit surfaceChanged();
}

/** Inject CanvasSession; connect document/camera/tool signals; rebuild host wiring.
 *  Simplify: partially — signal connects are host-lifecycle; extract to ToolHostBinder. */
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
    if (m_session) {
        m_docConn = connect(m_session, &CanvasSession::documentMutated, this,
                            &ToolCanvasItem::onDocumentOrCameraChanged);
        m_camConn = connect(m_session, &CanvasSession::cameraChanged, this,
                            &ToolCanvasItem::onDocumentOrCameraChanged);
        m_toolConn = connect(m_session, &CanvasSession::exclusiveToolChanged, this, [this]() {
            syncActiveMode();
            if (m_toolCtx)
                m_toolCtx->syncOverlayPresence();
            if (m_toolCtx)
                m_toolCtx->requestChromeRefresh();
        });
    }
    syncToolHost();
    syncActiveMode();
    emit sessionChanged();
}

/** Composition root — wire DocContext, ToolContext, appliers, SelectionManip, HostCaps, finger ops.
 *  Simplify: yes — prime candidate for `ToolHostBinder::sync(session, surface, …)`; keep one-liner here. */
void ToolCanvasItem::syncToolHost()
{
    if (!m_docCtx)
        m_docCtx = std::make_unique<epaper::tools::SessionDocContext>(m_session, m_surface);
    else {
        m_docCtx->setSession(m_session);
        m_docCtx->setSurface(m_surface);
    }
    if (!m_toolCtx)
        m_toolCtx = std::make_unique<epaper::tools::ToolCanvasContext>(this);

    m_toolCtx->setDoc(m_docCtx.get());
    m_toolCtx->setSelection(&m_selection);
    m_toolCtx->setManip(&m_manip);
    m_toolCtx->setHub(&m_hub);
    m_toolCtx->setSelectionManip(&m_selManip);
    m_toolCtx->setIsSelectionTool([this]() { return isSelectionTool(); });
    m_toolCtx->setRepaint([this](const QRectF &r) { update(r.toAlignedRect()); });
    m_toolCtx->setSetVisible([this](bool on) { setVisible(on); });
    m_toolCtx->setEmitChromeChanged([this]() { emit selectionChromeChanged(); });
    m_toolCtx->setSetStrokeWaveform([this](bool w) { setStrokeWaveform(w); });
    m_toolCtx->setSendManipPreview([this](bool resizeGesture) {
        if (!m_surface)
            return;
        const epaper::document::SmartBounds *bptr = resizeGesture ? &m_manip.liveB : nullptr;
        m_surface->publishManipPreview(m_manip.nodeId, m_manip.liveT, bptr);
    });
    m_toolCtx->setSetInteractionDebug([this](const std::string &line) {
        if (m_surface)
            m_surface->setInteractionDebug(QString::fromStdString(line));
    });

    m_manipApplier.setDoc(m_docCtx.get());
    m_manipApplier.setTool(m_toolCtx.get());
    m_selApplier.setTool(m_toolCtx.get());
    m_selApplier.setResetManip([this]() { m_manip.reset(); });
    m_selApplier.setInteractionDebug([this](const std::string &line) {
        if (m_surface)
            m_surface->setInteractionDebug(QString::fromStdString(line));
    });

    m_selManip.setDoc(m_docCtx.get());
    m_selManip.setTool(m_toolCtx.get());
    m_selManip.setChrome(&m_toolCtx->chrome());
    m_selManip.setHub(&m_hub);
    m_selManip.setSelection(&m_selection);
    m_selManip.setManip(&m_manip);
    m_selManip.setFinger(&m_finger);
    m_selManip.setSelApplier(&m_selApplier);
    m_selManip.setManipApplier(&m_manipApplier);
    m_selManip.setGhostClock(&m_selectionGhostClock);
    m_selManip.setToolIntentSeq(&m_toolIntentSeq);
    m_selManip.setIsSelectionTool([this]() { return isSelectionTool(); });
    m_selManip.setMakeStrokeHost([this]() { return makeSelectionStrokeHost(); });
    m_selManip.setSelectStroke([this]() { return m_selectStroke.get(); },
                               [this](std::unique_ptr<epaper::tools::Operation> op) {
                                   m_selectStroke = std::move(op);
                               });
    m_selManip.setFeedSelectStroke([this](QEvent::Type type, const PanelPt &panel) {
        feedSelectStroke(type, panel);
    });
    m_selManip.setOriginPanelRect([this]() { return &m_toolCtx->chrome().state().originPanelRect; });
    m_selManip.setLiveDirtyPrev([this]() { return &m_toolCtx->chrome().state().liveDirtyPrev; });
    m_selManip.setHostSize([this]() { return QSizeF(width(), height()); });

    m_fingerApplier.setDoc(m_docCtx.get());
    m_fingerApplier.setTool(m_toolCtx.get());
    m_fingerApplier.setSelectionManip(&m_selManip);
    m_fingerApplier.setAbortManip([this]() { m_selManip.abortFingerManip(); });

    m_inkSink = std::make_unique<epaper::tools::TabletInkSink>(m_surface);
    epaper::tools::HostCaps caps;
    caps.ink = m_inkSink.get();
    caps.doc = m_docCtx.get();
    caps.toolUi = m_toolCtx.get();
    caps.selection = &m_selCtx;
    caps.setExclusiveTool = [this](const QString &id) {
        if (m_surface)
            m_surface->setToolMode(id);
    };
    m_hub.setHostCaps(caps);
    m_inkStroke.reset();
    syncHandTouchFactories();
}

/** Register cached HandTouch Operations (Navigation/Move/Resize/Select) on InputHub.
 *  Simplify: yes — move into ToolHostBinder or InputHub::registerDefaultFingerOps. */
void ToolCanvasItem::syncHandTouchFactories()
{
    using namespace epaper::tools;
    m_hub.clearFingerOperations();
    const FingerHost fingerHost = makeFingerHost();
    const ManipHost manipHost = makeManipHost();
    m_hub.setFingerOperation(OperationKind::Navigation,
                             std::make_unique<NavigationOperation>(fingerHost));
    m_hub.setFingerOperation(OperationKind::Move, std::make_unique<MoveOperation>(manipHost));
    m_hub.setFingerOperation(OperationKind::Resize, std::make_unique<ResizeOperation>(manipHost));
    m_hub.setFingerOperation(OperationKind::Select, std::make_unique<SelectOperation>(fingerHost));
}

/** Map exclusive chip → PenMode or SelectionMode; activate/deactivate on InputHub.
 *  Simplify: yes — belongs in InputHub or a ModeRegistry; host calls `m_hub.syncActiveMode(id)`. */
void ToolCanvasItem::syncActiveMode()
{
    const bool wantPen = m_docCtx && m_docCtx->exclusiveTool() == QLatin1String("pen");
    const bool wantSel = isSelectionTool();
    epaper::tools::InteractionMode *want = nullptr;
    if (wantPen)
        want = &m_penMode;
    else if (wantSel)
        want = &m_selectionMode;

    if (m_hub.activeMode() == want)
        return;
    if (m_hub.activeMode())
        m_hub.activeMode()->deactivate(m_hub, m_hub.handTouch());
    m_hub.setActiveMode(nullptr);
    if (want) {
        want->activate(m_hub.hostCaps(), m_hub, m_hub.handTouch());
        m_hub.setActiveMode(want);
    }
}

/** Factory bag for Marquee/Lasso Operations — session + applier + DocContext frame helpers.
 *  Simplify: yes — collapse into ToolHostBinder; not repeated per call site. */
epaper::tools::SelectionStrokeHost ToolCanvasItem::makeSelectionStrokeHost()
{
    epaper::tools::SelectionStrokeHost host;
    host.session = &m_selection;
    host.minGesture = epaper::tools::SelectionManipController::kMinMarqueeGesture;
    host.applyIntent = [this](const epaper::selection::SelectionResult &r) {
        m_selApplier.apply(r);
    };
    host.document = [this]() -> const epaper::document::DeviceDocument & {
        return m_docCtx->document();
    };
    host.panelToWorld = [this](double px, double py, double *wx, double *wy) {
        const auto w = m_docCtx->panelToWorld(px, py);
        *wx = w.x;
        *wy = w.y;
    };
    return host;
}

/** Factory bag for finger Operations — FingerGestureMachine + DocContext + applier lambdas.
 *  Simplify: yes — largest wiring block; move to ToolHostBinder / FingerHostBuilder. */
epaper::tools::FingerHost ToolCanvasItem::makeFingerHost()
{
    epaper::tools::FingerHost host;
    host.machine = &m_finger;
    host.applyIntent = [this](const epaper::fingergesture::FingerResult &r, const PanelPt &p) {
        m_fingerApplier.apply(r, p);
    };
    host.applyIntentBare = [this](const epaper::fingergesture::FingerResult &r) {
        m_fingerApplier.apply(r);
    };
    host.ensureLocalDrawingRegion = [this]() {
        if (m_surface)
            m_surface->ensureLocalDrawingRegion();
    };
    host.drawingRegion = [this]() { return m_docCtx->frame().drawingRegion; };
    host.panelToWorld = [this](QPointF p) {
        const auto w = m_docCtx->panelToWorld(p.x(), p.y());
        return epaper::canvasframe::WorldPt{w.x, w.y};
    };
    host.uvPair = [this](QPointF a, QPointF b) { return m_docCtx->uvPair(a.x(), a.y(), b.x(), b.y()); };
    host.follow = [this]() { return m_docCtx->followDirection(); };
    host.worldThroughPanOrigin = [this](QPointF p, double *wx, double *wy) {
        m_docCtx->worldThroughPanOrigin(m_finger.panOrigin, p.x(), p.y(), wx, wy);
    };
    host.previewDue = [this]() {
        return !m_fingerPanClock.isValid()
            || m_fingerPanClock.elapsed() >= epaper::tools::SelectionManipController::kGhostMinIntervalMs;
    };
    host.markPreviewPublished = [this]() { m_fingerPanClock.restart(); };
    host.invalidatePanClock = [this]() { m_fingerPanClock.invalidate(); };
    host.restartPanClock = [this]() { m_fingerPanClock.restart(); };
    host.beginSelectionGesture = [this](QPointF p) { m_selManip.beginSelectionGesture(p); };
    host.updateSelectionGesture = [this](QPointF p) { m_selManip.updateSelectionGesture(p); };
    host.endSelectionGesture = [this]() { m_selManip.endSelectionGesture(); };
    host.tryBeginHandleAtPanel = [this](QPointF p, double hitDu) {
        return m_selManip.tryBeginHandleAtPanel(p, hitDu);
    };
    host.manipActive = [this]() { return m_manip.active; };
    return host;
}

/** Factory bag for Move/Resize Operations — ManipSession + chrome hit-test + SelectionManip.
 *  Simplify: yes — move to ToolHostBinder / ManipHostBuilder. */
epaper::tools::ManipHost ToolCanvasItem::makeManipHost()
{
    epaper::tools::ManipHost host;
    host.manip = &m_manip;
    host.selection = &m_selection;
    host.penHandleHitDu = epaper::document::kHandleHitDu;
    host.fingerHandleHitDu = epaper::handtouch::kFingerHandleHitDu;
    host.handleIndexAtPanel = [this](QPointF p, double hitDu) {
        return m_toolCtx->chrome().handleIndexAtPanel(p, hitDu);
    };
    host.beginHandleDrag = [this](int idx, QPointF panel) {
        const auto w = m_docCtx->panelToWorld(panel.x(), panel.y());
        m_selManip.beginHandleDrag(idx, w.x, w.y);
    };
    host.beginMoveFromPanel = [this](QPointF panel, bool arm) {
        return m_selManip.beginMoveFromPanel(panel, arm);
    };
    host.applyDragFromPanel = [this](QPointF panel) {
        const auto w = m_docCtx->panelToWorld(panel.x(), panel.y());
        m_selManip.applyDragWorld(w.x, w.y);
    };
    host.commitTransform = [this]() { m_selManip.commitLiveManip(); };
    host.abortTransform = [this]() { m_selManip.abortFingerManip(); };
    return host;
}

/** Build HandTouchCommitInfo for profile postHandling (SwitchMode after select/move).
 *  Simplify: yes — move to InputHub or HandTouchModifier; host shouldn't know commit shape. */
epaper::tools::HandTouchCommitInfo ToolCanvasItem::makeHandTouchCommitInfo(
    epaper::tools::OperationKind kind) const
{
    epaper::tools::HandTouchCommitInfo info;
    info.kind = kind;
    info.selectionNonEmpty = !m_selection.ids.empty();
    info.didMutateSelection =
        info.selectionNonEmpty
        && (kind == epaper::tools::OperationKind::Move
            || kind == epaper::tools::OperationKind::Resize
            || kind == epaper::tools::OperationKind::Select);
    return info;
}

/** Pen ink path — lazily create InkStrokeOperation and map Qt tablet phase → onDown/Move/Up.
 *  Simplify: partially — could be `m_hub.dispatchPenInk(type, sample)`; keep thin adapter here. */
void ToolCanvasItem::feedInkStroke(QEvent::Type type, const PanelPt &panel, qreal pressure)
{
    if (!m_inkSink)
        syncToolHost();
    if (!m_inkStroke)
        m_inkStroke = std::make_unique<epaper::tools::InkStrokeOperation>(&m_hub.hostCaps());
    epaper::tools::PointerSample s;
    s.panel = panel;
    s.pressure = pressure;
    s.device = epaper::tools::PointerDevice::Pen;
    switch (type) {
    case QEvent::TabletPress:
        m_inkStroke->onDown(s);
        break;
    case QEvent::TabletMove:
        m_inkStroke->onMove(s);
        break;
    case QEvent::TabletRelease:
        m_inkStroke->onUp(s);
        m_inkStroke.reset();
        break;
    default:
        break;
    }
}

/** Marquee/Lasso stroke path — map Qt phase → locked RawPointerSink on m_selectStroke.
 *  Simplify: partially — could live on SelectionManipController; host holds op unique_ptr today. */
void ToolCanvasItem::feedSelectStroke(QEvent::Type type, const PanelPt &panel)
{
    auto *sink = dynamic_cast<epaper::tools::RawPointerSink *>(m_selectStroke.get());
    if (!sink)
        return;
    epaper::tools::PointerSample s;
    s.panel = panel;
    s.pressure = 1.0;
    s.device = epaper::tools::PointerDevice::Pen;
    switch (type) {
    case QEvent::TabletPress:
        sink->onDown(s);
        break;
    case QEvent::TabletMove:
        sink->onMove(s);
        break;
    case QEvent::TabletRelease:
        sink->onUp(s);
        m_selectStroke.reset();
        break;
    default:
        break;
    }
}

void ToolCanvasItem::onDocumentOrCameraChanged()
{
    m_selManip.onDocumentOrCameraChanged();
}

void ToolCanvasItem::cancelInteraction()
{
    onPointerCancel();
}

/** True when exclusive chip is sel_rect or sel_freeform.
 *  Simplify: yes — `return m_docCtx->isSelectionTool()` one-liner on SessionDocContext. */
bool ToolCanvasItem::isSelectionTool() const
{
    if (!m_docCtx)
        return false;
    const QString m = m_docCtx->exclusiveTool();
    return m == QLatin1String("sel_rect") || m == QLatin1String("sel_freeform");
}

/** Pen selection press — build hit context (knob/box) and try InputHub Move/Resize lock first.
 *  Simplify: yes — move to InputHub::dispatchPenSelectionDown; host passes panel only. */
bool ToolCanvasItem::tryDispatchSelectionPointer(const PanelPt &panel, qreal pressure)
{
    epaper::tools::FingerDownContext ctx;
    ctx.sample.panel = panel;
    ctx.sample.pressure = pressure;
    ctx.sample.device = epaper::tools::PointerDevice::Pen;
    ctx.knobHit = m_toolCtx->chrome().handleIndexAtPanel(panel, epaper::document::kHandleHitDu) >= 0;
    ctx.boxHit = fingerHitsBox(panel);
    return m_hub.dispatchSelectionPointerDown(ctx);
}

/** Panel → world pick test for finger/pen Move Operation matching.
 *  Simplify: yes — inline at call sites via DocContext::fingerHitsBox(panel) helper. */
bool ToolCanvasItem::fingerHitsBox(const PanelPt &panel) const
{
    if (!m_docCtx)
        return false;
    const auto w = m_docCtx->panelToWorld(panel.x(), panel.y());
    return m_docCtx->fingerHitsBox(w.x, w.y);
}

/** Primary pointer-down router (ToolCanvas.qml). Pen: hub lock → selection gesture → ink.
 *  Finger: hub finger down. This is core routing — keep here or move wholesale to InputHub.
 *  Simplify: partially — extract pen/finger branches to InputHub::onPointerStart if hub owns policy. */
void ToolCanvasItem::onPointerStart(qreal x, qreal y, qreal pressure, bool pen)
{
    if (!m_surface)
        return;
    const PanelPt panel(x, y);
    if (pen) {
        if (isSelectionTool()) {
            if (tryDispatchSelectionPointer(panel, pressure))
                return;
            m_selManip.beginSelectionGesture(panel);
            return;
        }
        feedInkStroke(QEvent::TabletPress, panel, pressure);
        return;
    }
    if (m_finger.lockedUntilLift)
        return;
    beginFingerTouch(panel);
}

/** Primary pointer-move router. Pen: locked op → selection gesture → ink. Finger: hub move.
 *  Simplify: partially — same as onPointerStart; policy table belongs in InputHub long-term. */
void ToolCanvasItem::onPointerMove(qreal x, qreal y, qreal pressure, bool pen)
{
    if (!m_surface)
        return;
    if (m_finger.isTwoFinger())
        return;
    const PanelPt panel(x, y);
    if (pen) {
        if (m_hub.lockedOperation()) {
            epaper::tools::PointerSample s;
            s.panel = panel;
            s.pressure = pressure;
            s.device = epaper::tools::PointerDevice::Pen;
            if (m_hub.dispatchPointerMove(s))
                return;
        }
        if (selectionGestureActive()) {
            m_selManip.updateSelectionGesture(panel);
            return;
        }
        feedInkStroke(QEvent::TabletMove, panel, pressure);
        return;
    }
    updateFingerTouch(panel, 1);
}

/** Primary pointer-up router. Pen: locked op / selection gesture / ink. Finger: hub up.
 *  Simplify: partially — mirror of onPointerMove; consolidate trio into InputHub dispatch. */
void ToolCanvasItem::onPointerEnd(qreal x, qreal y, bool pen)
{
    if (!m_surface)
        return;
    const PanelPt panel(x, y);
    if (pen) {
        if (m_hub.lockedOperation()) {
            epaper::tools::PointerSample s;
            s.panel = panel;
            s.device = epaper::tools::PointerDevice::Pen;
            m_hub.dispatchPointerUp(s, {});
            m_surface->clearStash();
            return;
        }
        if (selectionGestureActive()) {
            m_selManip.endSelectionGesture();
            m_surface->clearStash();
            return;
        }
        feedInkStroke(QEvent::TabletRelease, panel, 0);
        return;
    }
    if (m_finger.isTwoFinger())
        return;
    if (m_finger.lockedUntilLift)
        return;
    endFingerTouch(panel);
}

/** Stationary tap — synthesize finger down+up (DragHandler tap path).
 *  Simplify: yes — `m_hub.dispatchFingerTap(panel)` once hub accepts Tap strategy end-to-end. */
void ToolCanvasItem::onFingerTap(qreal x, qreal y)
{
    if (m_finger.lockedUntilLift)
        return;
    const PanelPt panel(x, y);
    beginFingerTouch(panel);
    endFingerTouch(panel);
}

/** Escape hatch — priority cancel: ink stroke → tablet stroke → locked op → select stroke →
 *  selection gesture → hand touch. Clears stash and finger lock.
 *  Simplify: partially — belongs in InputHub::cancelAll(); host triggers from QML pen-near. */
void ToolCanvasItem::onPointerCancel()
{
    if (m_inkStroke) {
        m_inkStroke->cancel();
        m_inkStroke.reset();
    } else if (m_surface && m_surface->strokeActive())
        m_surface->cancelActiveStroke();
    else if (m_hub.lockedOperation())
        m_hub.dispatchPointerCancel();
    else if (m_selectStroke) {
        if (auto *sink = dynamic_cast<epaper::tools::RawPointerSink *>(m_selectStroke.get()))
            sink->onCancel();
        m_selectStroke.reset();
    } else if (selectionGestureActive())
        m_selManip.endSelectionGesture();
    cancelHandTouch();
    if (m_surface)
        m_surface->clearStash();
    m_finger.lockedUntilLift = false;
}

/** Second capacitive contact — abort one-finger manip via FingerGestureMachine → applier.
 *  Simplify: yes — InputHub::onSecondContact or HandTouchModifier; bypasses hub today. */
void ToolCanvasItem::onSecondContact()
{
    const bool manip = m_finger.isLiveManip() || m_manip.active;
    m_fingerApplier.apply(m_finger.secondContact(m_manip.active));
    qInfo().noquote() << QStringLiteral("[hand] second contact manip=%1").arg(manip ? 1 : 0);
}

/** All contacts lifted — clear FingerGestureMachine lock state (Main.qml contactCount).
 *  Simplify: no — thin state reset; could move with m_finger into HandTouchModifier. */
void ToolCanvasItem::onContactsCleared()
{
    m_finger.contactsCleared();
}

/** PinchHandler start — arming guard, abort live manip, synthetic two-finger contacts → hub.
 *  Simplify: partially — pinch arm math + policy could move to NavigationOperation/PinchSink setup. */
void ToolCanvasItem::onPinchStart(qreal x, qreal y, qreal scale)
{
    if (!m_finger.armed || m_finger.gesture == epaper::fingergesture::Kind::Chip) {
        m_pinchIgnore = true;
        return;
    }
    if (m_finger.isLiveManip() || m_manip.active)
        m_selManip.abortFingerManip();
    m_finger.clearGestureForPinch();
    m_pinchArm = 80.0;
    m_pinchScale0 = scale > 0.01 ? scale : 1.0;
    m_pinchIgnore = !beginTwoFingerTouch(pinchArmPoint(x, y, scale, true),
                                         pinchArmPoint(x, y, scale, false));
}

/** PinchHandler update — forward synthetic contact pair to hub unless ignored.
 *  Simplify: yes — collapse with onPinchEnd into InputHub::dispatchPinch*. */
void ToolCanvasItem::onPinchUpdate(qreal x, qreal y, qreal scale)
{
    if (m_pinchIgnore)
        return;
    updateTwoFingerTouch(pinchArmPoint(x, y, scale, true), pinchArmPoint(x, y, scale, false));
}

/** PinchHandler end — settle two-finger navigation.
 *  Simplify: yes — InputHub::dispatchPinchEnd from QML directly. */
void ToolCanvasItem::onPinchEnd()
{
    if (!m_pinchIgnore)
        endTwoFingerTouch();
    m_pinchIgnore = false;
}

/** Synthetic contact offset around pinch centroid for two-finger UV math.
 *  Simplify: yes — move to PinchContext builder inside InputHub or NavigationOperation. */
ToolCanvasItem::PanelPt ToolCanvasItem::pinchArmPoint(qreal x, qreal y, qreal scale,
                                                      bool positive) const
{
    const qreal s0 = m_pinchScale0 > 0.01 ? m_pinchScale0 : 1.0;
    const qreal arm = m_pinchArm * (scale / s0);
    return PanelPt(x + (positive ? arm : -arm), y);
}

/** One-finger down — hit prefilter (knob/box) then InputHub match→lock→Operation onDown.
 *  Simplify: yes — InputHub::dispatchFingerDown should accept panel and resolve hits internally. */
bool ToolCanvasItem::beginFingerTouch(const PanelPt &canvasPos)
{
    if (!m_finger.armed)
        return false;
    epaper::tools::FingerDownContext ctx;
    ctx.sample.panel = canvasPos;
    ctx.sample.device = epaper::tools::PointerDevice::Finger;
    ctx.knobHit = m_toolCtx->chrome().handleIndexAtPanel(canvasPos, epaper::handtouch::kFingerHandleHitDu) >= 0;
    ctx.boxHit = fingerHitsBox(canvasPos);
    return m_hub.dispatchFingerDown(ctx);
}

/** One-finger move — respect FingerGestureMachine ignore gate, then hub dispatch.
 *  Simplify: yes — gate belongs in HandTouchModifier once machine moves off host. */
void ToolCanvasItem::updateFingerTouch(const PanelPt &canvasPos, int fingerCount)
{
    if (m_finger.ignoresOneFingerUpdate())
        return;
    epaper::tools::PointerSample s;
    s.panel = canvasPos;
    s.device = epaper::tools::PointerDevice::Finger;
    m_hub.dispatchPointerMove(s, fingerCount);
}

/** One-finger up — attach HandTouchCommitInfo for profile postHandling, hub dispatch.
 *  Simplify: yes — InputHub builds commit info from locked op; host passes sample only. */
void ToolCanvasItem::endFingerTouch(const PanelPt &canvasPos)
{
    epaper::tools::PointerSample s;
    s.panel = canvasPos;
    s.device = epaper::tools::PointerDevice::Finger;
    epaper::tools::HandTouchCommitInfo commit;
    if (const epaper::tools::Operation *op = m_hub.lockedOperation())
        commit = makeHandTouchCommitInfo(op->kind());
    m_hub.dispatchPointerUp(s, commit);
}

/** Two-finger down — build PinchContext and dispatch NavigationOperation lock.
 *  Simplify: yes — InputHub::dispatchPinchBegin from onPinchStart directly. */
bool ToolCanvasItem::beginTwoFingerTouch(const PanelPt &a, const PanelPt &b)
{
    epaper::tools::PinchContext ctx;
    ctx.contactA = a;
    ctx.contactB = b;
    ctx.centroid = PanelPt((a.x() + b.x()) * 0.5, (a.y() + b.y()) * 0.5);
    return m_hub.dispatchPinchBegin(ctx);
}

/** Two-finger move — hub pinch update.
 *  Simplify: yes — merge into onPinchUpdate → hub one-liner. */
void ToolCanvasItem::updateTwoFingerTouch(const PanelPt &a, const PanelPt &b)
{
    epaper::tools::PinchContext ctx;
    ctx.contactA = a;
    ctx.contactB = b;
    m_hub.dispatchPinchUpdate(ctx);
}

/** Two-finger up — hub pinch end with commit info for postHandling.
 *  Simplify: yes — InputHub builds commit; host one-liner. */
void ToolCanvasItem::endTwoFingerTouch()
{
    epaper::tools::HandTouchCommitInfo commit;
    if (const epaper::tools::Operation *op = m_hub.lockedOperation())
        commit = makeHandTouchCommitInfo(op->kind());
    m_hub.dispatchPinchEnd(commit);
}

/** ToolChip hand-touch toggle — sync armed on machine + HandTouchModifier; cancel if off.
 *  Simplify: partially — UI action stays Q_INVOKABLE; core could be HandTouchModifier::toggle(). */
void ToolCanvasItem::toggleHandTouch()
{
    m_finger.setArmed(!m_finger.armed);
    m_hub.handTouch().setArmed(m_finger.armed);
    if (!m_finger.armed)
        cancelHandTouch();
    emit handTouchArmedChanged();
}

/** Pen-near / disarm escape — two-finger end, hub cancel, or machine cancel → applier.
 *  Simplify: partially — unify with onPointerCancel via InputHub::cancelHandTouch(). */
void ToolCanvasItem::cancelHandTouch()
{
    if (m_finger.isTwoFinger()) {
        endTwoFingerTouch();
        return;
    }
    if (m_hub.lockedOperation())
        m_hub.dispatchPointerCancel();
    else
        m_fingerApplier.apply(m_finger.cancel(m_manip.active));
}

void ToolCanvasItem::encloseSelection()
{
    m_selManip.encloseSelection();
}

void ToolCanvasItem::tapModeChip()
{
    m_selManip.tapModeChip();
}

QRectF ToolCanvasItem::encloseCtaRect() const
{
    return m_toolCtx ? m_toolCtx->chrome().state().encloseCtaRect : QRectF();
}

bool ToolCanvasItem::encloseVisible() const
{
    return m_toolCtx && m_toolCtx->chrome().state().encloseVisible;
}

QString ToolCanvasItem::encloseRefuseReason() const
{
    return m_toolCtx ? m_toolCtx->chrome().state().encloseRefuseReason : QString();
}

QRectF ToolCanvasItem::selectionBoundsRect() const
{
    return m_toolCtx ? m_toolCtx->chrome().state().selectionBoundsRect : QRectF();
}

int ToolCanvasItem::handleCount() const
{
    return m_toolCtx ? m_toolCtx->chrome().state().handleCount : 0;
}

qreal ToolCanvasItem::handleSize() const
{
    return m_toolCtx ? m_toolCtx->chrome().state().handleSize : 16.0;
}

bool ToolCanvasItem::modeChipVisible() const
{
    return m_toolCtx && m_toolCtx->chrome().state().modeChipVisible;
}

QString ToolCanvasItem::modeChipLabel() const
{
    return m_toolCtx ? m_toolCtx->chrome().state().modeChipLabel : QString();
}

QRectF ToolCanvasItem::modeChipRectProp() const
{
    return m_toolCtx ? m_toolCtx->chrome().state().modeChipRect : QRectF();
}

QString ToolCanvasItem::manipulationUnavailable() const
{
    return m_toolCtx ? m_toolCtx->chrome().state().manipUnavailable : QString();
}

QRectF ToolCanvasItem::manipulationUnavailableRect() const
{
    return m_toolCtx ? m_toolCtx->chrome().state().manipUnavailableRect : QRectF();
}
