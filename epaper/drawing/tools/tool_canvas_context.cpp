#include "tool_canvas_context.hpp"

#include "../toolcanvasitem.h"

namespace epaper {
namespace tools {

ToolCanvasContext::ToolCanvasContext(ToolCanvasItem *host)
    : m_host(host)
{
}

void ToolCanvasContext::damageChrome(const QRectF &panelRect)
{
    if (m_host)
        m_host->damageToolChromeForContext(panelRect);
}

void ToolCanvasContext::damageChromeSegment(const QRectF &panelRect)
{
    if (m_host)
        m_host->damageToolChromeSegmentForContext(panelRect);
}

void ToolCanvasContext::syncOverlayPresence()
{
    if (m_host)
        m_host->syncToolCanvasPresence();
}

void ToolCanvasContext::setStrokeWaveform(bool penInFlight)
{
    if (m_host)
        m_host->setStrokeWaveform(penInFlight);
}

void ToolCanvasContext::requestChromeRefresh()
{
    if (m_host)
        m_host->refreshSelectionChrome();
}

void ToolCanvasContext::resetTransientChromeFlags()
{
    if (m_host)
        m_host->resetTransientChromeFlagsForContext();
}

void ToolCanvasContext::emitChromeChanged()
{
    if (m_host)
        m_host->emitSelectionChromeChangedForContext();
}

void ToolCanvasContext::redrawLiveManip()
{
    if (m_host)
        m_host->redrawLiveManipRegion();
}

void ToolCanvasContext::sendManipPreview(bool resizeGesture)
{
    if (m_host)
        m_host->sendManipPreviewForContext(resizeGesture);
}

void ToolCanvasContext::setInteractionDebug(const std::string &line)
{
    if (m_host)
        m_host->setInteractionDebugForContext(line);
}

void ToolCanvasContext::clearOriginPanelRect()
{
    if (m_host)
        m_host->clearOriginPanelRectForContext();
}

} // namespace tools
} // namespace epaper
