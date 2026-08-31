#include "tool_context_impl.hpp"

#include "../../toolcanvasitem.h"
#include "../input_hub.hpp"
#include "../mode.hpp"
#include "session_doc_context.hpp"

#include <QPainter>

namespace epaper {
namespace tools {

ToolContextImpl::ToolContextImpl(ToolCanvasItem *host)
    : m_host(host)
{
}

void ToolContextImpl::paintOverlay(QPainter *painter)
{
    if (m_hub && m_hub->activeMode())
        m_hub->activeMode()->paintOverlay(painter, m_hub->hostCaps(), *m_hub);
}

void ToolContextImpl::damageChrome(const QRectF &panelRect)
{
    const QRectF u = m_overlayPrev.isNull() ? panelRect : m_overlayPrev.united(panelRect);
    m_overlayPrev = panelRect;
    if (u.isEmpty() || !m_repaint)
        return;
    m_repaint(u.toAlignedRect().adjusted(-8, -8, 8, 8));
}

void ToolContextImpl::damageChromeSegment(const QRectF &panelRect)
{
    m_overlayPrev = m_overlayPrev.united(panelRect);
    if (panelRect.isEmpty() || !m_repaint)
        return;
    m_repaint(panelRect.toAlignedRect());
}

void ToolContextImpl::syncOverlayPresence()
{
    if (m_hub && m_hub->activeMode())
        m_hub->activeMode()->syncOverlay(m_hub->hostCaps(), *m_hub);
}

void ToolContextImpl::setOverlayVisible(bool on)
{
    if (m_setVisible)
        m_setVisible(on);
}

void ToolContextImpl::setStrokeWaveform(bool penInFlight)
{
    if (m_setStrokeWaveform)
        m_setStrokeWaveform(penInFlight);
    else if (m_host)
        m_host->setStrokeWaveform(penInFlight);
}

void ToolContextImpl::refreshChrome()
{
    if (m_hub && m_hub->activeMode())
        m_hub->activeMode()->refreshChrome(m_hub->hostCaps(), *m_hub);
}

epaper::canvasframe::WorldPt ToolContextImpl::panelToWorld(const QPointF &panel) const
{
    if (!m_doc)
        return {};
    return m_doc->panelToWorld(panel.x(), panel.y());
}

QPointF ToolContextImpl::worldToPanel(double wx, double wy) const
{
    return m_doc ? m_doc->worldToPanel(wx, wy) : QPointF();
}

bool ToolContextImpl::lodOkPanel(const epaper::document::SmartBounds &wb) const
{
    return m_doc && m_doc->lodOkPanel(wb);
}

QRectF ToolContextImpl::worldBoundsToPanel(const epaper::document::SmartBounds &wb) const
{
    return m_doc ? m_doc->worldBoundsToPanel(wb) : QRectF();
}

double ToolContextImpl::panelScale() const
{
    return m_doc ? m_doc->frame().panelScale() : 1.0;
}

QSizeF ToolContextImpl::hostSize() const
{
    return m_host ? QSizeF(m_host->width(), m_host->height()) : QSizeF();
}

} // namespace tools
} // namespace epaper
