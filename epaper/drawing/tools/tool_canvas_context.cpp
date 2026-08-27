#include "tool_canvas_context.hpp"

#include "../toolcanvasitem.h"
#include "input_hub.hpp"
#include "selection_manip_controller.hpp"
#include "session_doc_context.hpp"

#include <QPainter>

namespace epaper {
namespace tools {

ToolCanvasContext::ToolCanvasContext(ToolCanvasItem *host)
    : m_host(host)
{
}

void ToolCanvasContext::paintOverlay(QPainter *painter)
{
    if (!m_doc || !m_selection || !m_manip || !m_isSelectionTool)
        return;
    m_chrome.paint(painter, *m_selection, *m_manip, *m_doc, m_isSelectionTool());
}

void ToolCanvasContext::damageChrome(const QRectF &panelRect)
{
    m_chrome.damage(panelRect, m_repaint);
}

void ToolCanvasContext::damageChromeSegment(const QRectF &panelRect)
{
    m_chrome.damageSegment(panelRect, m_repaint);
}

void ToolCanvasContext::syncOverlayPresence()
{
    if (!m_selection || !m_isSelectionTool)
        return;
    m_chrome.syncPresence(*m_selection, m_isSelectionTool(), m_setVisible, m_setStrokeWaveform);
}

void ToolCanvasContext::setStrokeWaveform(bool penInFlight)
{
    if (m_host)
        m_host->setStrokeWaveform(penInFlight);
}

void ToolCanvasContext::requestChromeRefresh()
{
    if (!m_doc || !m_selection || !m_manip || !m_isSelectionTool)
        return;
    m_chrome.refresh(*m_selection, *m_manip, *m_doc, m_isSelectionTool());
    if (m_emitChanged)
        m_emitChanged();
    syncOverlayPresence();
    if (m_hub)
        m_chrome.syncHitTargets(*m_hub);
    damageChrome(m_chrome.state().selectionChromeDirty);
}

void ToolCanvasContext::resetTransientChromeFlags()
{
    m_chrome.resetTransientFlags();
}

void ToolCanvasContext::emitChromeChanged()
{
    if (m_emitChanged)
        m_emitChanged();
}

void ToolCanvasContext::redrawLiveManip()
{
    if (!m_doc || !m_selection || !m_manip)
        return;
    m_chrome.redrawLiveManip(*m_selection, *m_manip, *m_doc, m_repaint, m_emitChanged);
    syncOverlayPresence();
}

void ToolCanvasContext::sendManipPreview(bool resizeGesture)
{
    if (m_sendManipPreview)
        m_sendManipPreview(resizeGesture);
}

void ToolCanvasContext::setInteractionDebug(const std::string &line)
{
    if (m_setDebug)
        m_setDebug(line);
}

void ToolCanvasContext::clearOriginPanelRect()
{
    m_chrome.clearOriginPanelRect();
}

} // namespace tools
} // namespace epaper
