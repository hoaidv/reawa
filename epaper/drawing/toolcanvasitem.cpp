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

void ToolCanvasItem::setStrokeWaveform(bool penInFlight)
{
    EpaperBridge::instance()->setOverlayStrokePen(penInFlight);
}

void ToolCanvasItem::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    if (!EpaperBridge::instance()->attachMonoModeRegion(this))
        qInfo() << "[tool-canvas] Mono attach failed — tight bbox fallback (ADR-0019)";
}

void ToolCanvasItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        EpaperBridge::instance()->attachMonoModeRegion(this);
}

void ToolCanvasItem::setSurface(TabletCanvasItem *surface)
{
    if (m_surface == surface)
        return;
    m_surface = surface;
    syncToolHost();
    emit surfaceChanged();
}

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

bool ToolCanvasItem::isSelectionTool() const
{
    if (!m_docCtx)
        return false;
    const QString m = m_docCtx->exclusiveTool();
    return m == QLatin1String("sel_rect") || m == QLatin1String("sel_freeform");
}

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

bool ToolCanvasItem::fingerHitsBox(const PanelPt &panel) const
{
    if (!m_docCtx)
        return false;
    const auto w = m_docCtx->panelToWorld(panel.x(), panel.y());
    return m_docCtx->fingerHitsBox(w.x, w.y);
}

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

void ToolCanvasItem::onFingerTap(qreal x, qreal y)
{
    if (m_finger.lockedUntilLift)
        return;
    const PanelPt panel(x, y);
    beginFingerTouch(panel);
    endFingerTouch(panel);
}

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

void ToolCanvasItem::onSecondContact()
{
    const bool manip = m_finger.isLiveManip() || m_manip.active;
    m_fingerApplier.apply(m_finger.secondContact(m_manip.active));
    qInfo().noquote() << QStringLiteral("[hand] second contact manip=%1").arg(manip ? 1 : 0);
}

void ToolCanvasItem::onContactsCleared()
{
    m_finger.contactsCleared();
}

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

void ToolCanvasItem::onPinchUpdate(qreal x, qreal y, qreal scale)
{
    if (m_pinchIgnore)
        return;
    updateTwoFingerTouch(pinchArmPoint(x, y, scale, true), pinchArmPoint(x, y, scale, false));
}

void ToolCanvasItem::onPinchEnd()
{
    if (!m_pinchIgnore)
        endTwoFingerTouch();
    m_pinchIgnore = false;
}

ToolCanvasItem::PanelPt ToolCanvasItem::pinchArmPoint(qreal x, qreal y, qreal scale,
                                                      bool positive) const
{
    const qreal s0 = m_pinchScale0 > 0.01 ? m_pinchScale0 : 1.0;
    const qreal arm = m_pinchArm * (scale / s0);
    return PanelPt(x + (positive ? arm : -arm), y);
}

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

void ToolCanvasItem::updateFingerTouch(const PanelPt &canvasPos, int fingerCount)
{
    if (m_finger.ignoresOneFingerUpdate())
        return;
    epaper::tools::PointerSample s;
    s.panel = canvasPos;
    s.device = epaper::tools::PointerDevice::Finger;
    m_hub.dispatchPointerMove(s, fingerCount);
}

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

bool ToolCanvasItem::beginTwoFingerTouch(const PanelPt &a, const PanelPt &b)
{
    epaper::tools::PinchContext ctx;
    ctx.contactA = a;
    ctx.contactB = b;
    ctx.centroid = PanelPt((a.x() + b.x()) * 0.5, (a.y() + b.y()) * 0.5);
    return m_hub.dispatchPinchBegin(ctx);
}

void ToolCanvasItem::updateTwoFingerTouch(const PanelPt &a, const PanelPt &b)
{
    epaper::tools::PinchContext ctx;
    ctx.contactA = a;
    ctx.contactB = b;
    m_hub.dispatchPinchUpdate(ctx);
}

void ToolCanvasItem::endTwoFingerTouch()
{
    epaper::tools::HandTouchCommitInfo commit;
    if (const epaper::tools::Operation *op = m_hub.lockedOperation())
        commit = makeHandTouchCommitInfo(op->kind());
    m_hub.dispatchPinchEnd(commit);
}

void ToolCanvasItem::toggleHandTouch()
{
    m_finger.setArmed(!m_finger.armed);
    m_hub.handTouch().setArmed(m_finger.armed);
    if (!m_finger.armed)
        cancelHandTouch();
    emit handTouchArmedChanged();
}

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
