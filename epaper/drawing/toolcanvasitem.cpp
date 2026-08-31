#include "toolcanvasitem.h"

#include "canvas_session.h"
#include "epaperbridge.h"
#include "tabletcanvasitem.h"
#include "tools/interventions.hpp"
#include "tools/operations/ink_stroke_operation.hpp"
#include "tools/operations/lasso_operation.hpp"
#include "tools/operations/marquee_operation.hpp"
#include "tools/operations/move_operation.hpp"
#include "tools/operations/navigation_operation.hpp"
#include "tools/operations/resize_operation.hpp"
#include "tools/operations/select_operation.hpp"
#include "tools/operations/brush_erase_operation.hpp"
#include "tools/operations/area_erase_operation.hpp"
#include "tools/operations/object_erase_operation.hpp"

#include "debug/ink_path_probe.hpp"

#include <QDebug>
#include <QPainter>
#include <QStringList>

/**
 * ToolCanvasItem — Qt entry; InputHub owns match/lock/feed (ADR-0033).
 */

ToolCanvasItem::ToolCanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
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
    epaper::inkpath::Span span("toolPaint");
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
    if (m_recogConn)
        disconnect(m_recogConn);
    m_session = session;
    if (m_session) {
        m_docConn = connect(m_session, &CanvasSession::documentMutated, this, [this] {
            if (m_docCtx)
                m_selCtx.retainExisting(m_docCtx->document());
            if (m_toolCtx)
                m_toolCtx->refreshChrome();
        });
        m_camConn = connect(m_session, &CanvasSession::cameraChanged, this, [this] {
            if (m_docCtx)
                m_selCtx.retainExisting(m_docCtx->document());
            if (m_toolCtx)
                m_toolCtx->refreshChrome();
        });
        m_toolConn = connect(m_session, &CanvasSession::exclusiveToolChanged, this, [this]() {
            m_hub.dispatchHoverLeave();
            m_emphasis.clearStrokeStamp(m_hub.hostCaps());
            m_emphasis.hideAllAabbs(m_hub.hostCaps());
            syncActiveMode();
            if (m_toolCtx)
                m_toolCtx->syncOverlayPresence();
            if (m_toolCtx)
                m_toolCtx->refreshChrome();
        });
        m_recogConn = connect(m_session, &CanvasSession::recogChrome, this,
            [this](int kind, const QStringList &ids) {
                std::vector<std::string> v;
                v.reserve(size_t(ids.size()));
                for (const auto &s : ids)
                    v.push_back(s.toStdString());
                epaper::tools::HostCaps &caps = m_hub.hostCaps();
                if (kind == 0)
                    m_emphasis.clearStrokeStamp(caps);
                else if (kind == 1 || kind == 2)
                    m_emphasis.blink(caps, v);
                else if (kind == 3)
                    m_emphasis.setStrokeStamp(caps, v, epaper::tools::StrokeStamp::Bold);
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
        m_toolCtx = std::make_unique<epaper::tools::ToolContextImpl>(this);

    m_toolCtx->setDoc(m_docCtx.get());
    m_toolCtx->setHub(&m_hub);
    m_toolCtx->setRepaint([this](const QRectF &r) { update(r.toAlignedRect()); });
    m_toolCtx->setSetVisible([this](bool on) { setVisible(on); });
    m_toolCtx->setSetStrokeWaveform([this](bool w) { setStrokeWaveform(w); });
    m_emphasis.setTimerHost(this);

    m_inkSink = std::make_unique<epaper::tools::TabletInkSink>(m_surface);
    epaper::tools::HostCaps caps;
    caps.ink = m_inkSink.get();
    caps.doc = m_docCtx.get();
    caps.toolUi = m_toolCtx.get();
    caps.selection = &m_selCtx;
    caps.overlay = &m_overlay;
    caps.bar = &m_selBar;
    caps.emphasis = &m_emphasis;
    caps.emitChromeChanged = [this]() { emit selectionChromeChanged(); };
    caps.setExclusiveTool = [this](const QString &id) {
        if (m_surface)
            m_surface->setToolMode(id);
    };
    m_hub.setHostCaps(caps);
    registerOperations();
    registerInterventions();
}

void ToolCanvasItem::registerOperations()
{
    using namespace epaper::tools;
    HostCaps &caps = m_hub.hostCaps();
    m_hub.clearOperations();
    m_hub.setOperation(OperationKind::InkStroke, std::make_unique<InkStrokeOperation>(&caps));
    m_hub.setOperation(OperationKind::Lasso, std::make_unique<LassoOperation>(&caps));
    m_hub.setOperation(OperationKind::Marquee, std::make_unique<MarqueeOperation>(&caps));
    m_hub.setOperation(OperationKind::Move, std::make_unique<MoveOperation>(&caps));
    m_hub.setOperation(OperationKind::Resize, std::make_unique<ResizeOperation>(&caps, &m_hub));
    m_hub.setOperation(OperationKind::Select, std::make_unique<SelectOperation>(&caps));
    m_hub.setOperation(OperationKind::Navigation,
                       std::make_unique<NavigationOperation>(&caps, m_docCtx.get()));
    m_hub.setOperation(OperationKind::BrushErase, std::make_unique<BrushEraseOperation>(&caps));
    m_hub.setOperation(OperationKind::AreaErase, std::make_unique<AreaEraseOperation>(&caps));
    m_hub.setOperation(OperationKind::ObjectErase, std::make_unique<ObjectEraseOperation>(&caps));
}

void ToolCanvasItem::registerInterventions()
{
    using namespace epaper::tools;
    m_hub.clearInterventions();
    m_hub.registerIntervention({InterventionGate::PenProximity, {}, [this] { m_hub.cancelAll(); }});
    m_hub.registerIntervention({InterventionGate::SecondContact,
                                [this] { return m_hub.lockedOperation() != nullptr; },
                                [this] {
                                    m_hub.cancelAll();
                                    m_hub.secondary().setLockedUntilLift(true);
                                }});
}

void ToolCanvasItem::syncActiveMode()
{
    const QString ex = m_docCtx ? m_docCtx->exclusiveTool() : QStringLiteral("pen");
    epaper::tools::InteractionMode *want = nullptr;
    if (ex.startsWith(QLatin1String("erase_")))
        want = &m_eraserMode;
    else if (ex == QLatin1String("sel_rect") || ex == QLatin1String("sel_freeform"))
        want = &m_selectionMode;
    else
        want = &m_inkMode;

    if (m_hub.activeMode() == want)
        return;
    if (m_hub.activeMode())
        m_hub.activeMode()->deactivate(m_hub);
    m_hub.setActiveMode(nullptr);
    if (want) {
        want->activate(m_hub.hostCaps(), m_hub);
        m_hub.setActiveMode(want);
    }
}

epaper::tools::PointerSample ToolCanvasItem::sample(qreal x, qreal y, qreal pressure, bool pen,
                                                    bool eraserNib) const
{
    epaper::tools::PointerSample s;
    s.panel = QPointF(x, y);
    s.pressure = pressure;
    s.device = pen ? epaper::tools::PointerDevice::Pen : epaper::tools::PointerDevice::Finger;
    s.eraserNib = eraserNib;
    return s;
}

void ToolCanvasItem::cancelInteraction()
{
    onPointerCancel();
}

void ToolCanvasItem::onPointerStart(qreal x, qreal y, qreal pressure, bool pen, bool eraserNib)
{
    const int ink = m_session ? m_session->document.inkCount() : 0;
    const int nodes = m_session ? m_session->document.nodeCount() : 0;
    epaper::inkpath::Sample probe(epaper::inkpath::Event::Down, ink, nodes);
    // Erase only: deferring rasterize on move/resize leaves the origin node on
    // TabletCanvas (suppress punch never runs). Nib erase starts as pen exclusive.
    if (m_surface && (eraserNib || (m_hub.activeMode() &&
                                    m_hub.activeMode()->id() == epaper::tools::ModeId::Eraser)))
        m_surface->setErasePointerActive(true);
    if (eraserNib && m_session && !m_nibArmed)
        m_nibArmed = m_session->beginNibErase();
    m_hub.dispatchPointerDown(sample(x, y, pressure, pen, eraserNib));
}

void ToolCanvasItem::onPointerMove(qreal x, qreal y, qreal pressure, bool pen, bool eraserNib)
{
    const int ink = m_session ? m_session->document.inkCount() : 0;
    const int nodes = m_session ? m_session->document.nodeCount() : 0;
    epaper::inkpath::Sample probe(epaper::inkpath::Event::Move, ink, nodes);
    m_hub.dispatchPointerMove(sample(x, y, pressure, pen, eraserNib));
}

void ToolCanvasItem::onPointerEnd(qreal x, qreal y, bool pen, bool eraserNib)
{
    const int ink = m_session ? m_session->document.inkCount() : 0;
    const int nodes = m_session ? m_session->document.nodeCount() : 0;
    epaper::inkpath::Sample probe(epaper::inkpath::Event::Up, ink, nodes);
    m_hub.dispatchPointerUp(sample(x, y, 0, pen, eraserNib));
    if (m_nibArmed && m_session) {
        m_session->endNibErase();
        m_nibArmed = false;
    }
    if (m_surface) {
        m_surface->clearStash();
        m_surface->setErasePointerActive(false);
    }
}

void ToolCanvasItem::onHoverMove(qreal x, qreal y)
{
    m_hub.dispatchHoverMove(sample(x, y, 0, true));
}

void ToolCanvasItem::onHoverLeave()
{
    m_hub.dispatchHoverLeave();
}

void ToolCanvasItem::onFingerTap(qreal x, qreal y)
{
    m_hub.dispatchTap(sample(x, y, 1.0, false));
}

void ToolCanvasItem::onPointerCancel()
{
    m_hub.cancelAll();
    if (m_nibArmed && m_session) {
        m_session->endNibErase();
        m_nibArmed = false;
    }
    if (m_surface) {
        m_surface->clearStash();
        m_surface->setErasePointerActive(false);
    }
}

void ToolCanvasItem::onSecondContact()
{
    m_hub.dispatchIntervention(epaper::tools::InterventionGate::SecondContact);
}

void ToolCanvasItem::onContactsCleared()
{
    m_hub.secondary().setLockedUntilLift(false);
}

void ToolCanvasItem::onPinchStart(qreal x, qreal y, qreal scale)
{
    m_hub.dispatchPinchBegin(x, y, scale);
}

void ToolCanvasItem::onPinchUpdate(qreal x, qreal y, qreal scale)
{
    m_hub.dispatchPinchUpdate(x, y, scale);
}

void ToolCanvasItem::onPinchEnd()
{
    m_hub.dispatchPinchEnd();
}

void ToolCanvasItem::toggleHandTouch()
{
    m_hub.secondary().setArmed(!m_hub.secondary().armed());
    if (!m_hub.secondary().armed())
        cancelHandTouch();
    emit handTouchArmedChanged();
}

void ToolCanvasItem::cancelHandTouch()
{
    m_hub.dispatchIntervention(epaper::tools::InterventionGate::PenProximity);
}
