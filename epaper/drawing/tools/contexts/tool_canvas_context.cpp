#include "tool_canvas_context.hpp"

#include "../../tabletcanvasitem.h"
#include "../../toolcanvasitem.h"
#include "../input_hub.hpp"
#include "../operation.hpp"
#include "../ui/selection_context_bar.hpp"
#include "selection_context.hpp"
#include "session_doc_context.hpp"
#include "document/erase_clip.hpp"
#include "../../canvas_session.h"

#include <QPainter>
#include <QPen>
#include <string>
#include <vector>

namespace epaper {
namespace tools {

ToolCanvasContext::ToolCanvasContext(ToolCanvasItem *host)
    : m_host(host)
{
}

void ToolCanvasContext::paintOverlay(QPainter *painter)
{
    if (m_hub && m_hub->lockedOperation())
        m_hub->lockedOperation()->paintOverlay(painter);
    paintEraseHover(painter);
    if (!m_doc || !m_selection)
        return;
    m_chrome.paint(painter, *m_selection, *m_doc, isSelectionTool());
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
    if (!m_selection)
        return;
    const bool penWaveform =
        (isSelectionTool() && exclusiveTool() == QLatin1String("sel_freeform")
         && m_selection->phase() != SelectionPhase::Selected
         && m_selection->phase() != SelectionPhase::Transforming)
        || exclusiveTool() == QLatin1String("erase_brush");
    m_chrome.syncPresence(*m_selection, isSelectionTool() || isEraserTool(), penWaveform, m_setVisible,
                          m_setStrokeWaveform);
}

void ToolCanvasContext::setStrokeWaveform(bool penInFlight)
{
    if (m_host)
        m_host->setStrokeWaveform(penInFlight);
}

void ToolCanvasContext::requestChromeRefresh()
{
    if (!m_doc || !m_selection)
        return;
    m_chrome.refresh(*m_selection, *m_doc, isSelectionTool());
    if (m_hub && m_bar)
        m_bar->refresh(m_hub->hostCaps(), m_chrome.state());
    if (m_emitChanged)
        m_emitChanged();
    syncOverlayPresence();
    if (m_hub)
        m_chrome.publishOverlayHits(*m_hub);
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

void ToolCanvasContext::redrawLiveManip(bool resizing)
{
    if (!m_doc || !m_selection)
        return;
    m_chrome.redrawLiveManip(*m_selection, *m_doc, resizing, m_repaint, nullptr);
    syncOverlayPresence();
}

void ToolCanvasContext::sendManipPreview(bool resizeGesture)
{
    if (!m_doc || !m_selection || !m_doc->surface())
        return;
    const auto &id = m_selection->pickableId();
    const epaper::document::DocNode *n = m_doc->document().find(id);
    if (!n)
        return;
    const epaper::document::SmartBounds *bptr = resizeGesture ? &n->smartBounds : nullptr;
    m_doc->surface()->publishManipPreview(n->id, n->transform, bptr);
}

void ToolCanvasContext::setInteractionDebug(const std::string &line)
{
    if (m_doc && m_doc->surface())
        m_doc->surface()->setInteractionDebug(QString::fromStdString(line));
}

void ToolCanvasContext::clearOriginPanelRect()
{
    m_chrome.clearOriginPanelRect();
}

void ToolCanvasContext::setOriginPanelRect(const QRectF &r)
{
    m_chrome.state().originPanelRect = r;
}

epaper::canvasframe::WorldPt ToolCanvasContext::panelToWorld(const QPointF &panel) const
{
    if (!m_doc)
        return {};
    return m_doc->panelToWorld(panel.x(), panel.y());
}

QPointF ToolCanvasContext::worldToPanel(double wx, double wy) const
{
    return m_doc ? m_doc->worldToPanel(wx, wy) : QPointF();
}

bool ToolCanvasContext::lodOkPanel(const epaper::document::SmartBounds &wb) const
{
    return m_doc && m_doc->lodOkPanel(wb);
}

QRectF ToolCanvasContext::worldBoundsToPanel(const epaper::document::SmartBounds &wb) const
{
    return m_doc ? m_doc->worldBoundsToPanel(wb) : QRectF();
}

QString ToolCanvasContext::exclusiveTool() const
{
    return m_doc ? m_doc->exclusiveTool() : QStringLiteral("pen");
}

bool ToolCanvasContext::isSelectionTool() const
{
    const QString m = exclusiveTool();
    return m == QLatin1String("sel_rect") || m == QLatin1String("sel_freeform");
}

bool ToolCanvasContext::isEraserTool() const
{
    const QString m = exclusiveTool();
    return m.startsWith(QLatin1String("erase_"));
}

double ToolCanvasContext::panelScale() const
{
    return m_doc ? m_doc->frame().panelScale() : 1.0;
}

bool ToolCanvasContext::eraseHoverEnabled() const
{
    return m_doc && m_doc->session() && m_doc->session()->chip.eraseBrushHover;
}

void ToolCanvasContext::setEraseHoverPanel(const QPointF &panel)
{
    if (!eraseHoverEnabled() || exclusiveTool() != QLatin1String("erase_brush")) {
        clearEraseHover();
        return;
    }
    const double scale = panelScale();
    const double d = epaper::document::eraseMmToWorld(epaper::document::kEraseBrushDiameterMm) * scale;
    const double stroke = epaper::document::eraseMmToWorld(epaper::document::kEraseHoverStrokeMm) * scale;
    const double pad = d * 0.5 + stroke + 2.0;
    const QRectF next = QRectF(panel, panel).adjusted(-pad, -pad, pad, pad);
    QRectF dirty = next;
    const bool wasValid = m_eraseHoverValid;
    if (wasValid)
        dirty = dirty.united(m_eraseHoverDirty);
    m_eraseHoverPanel = panel;
    m_eraseHoverValid = true;
    m_eraseHoverDirty = next;
    if (!wasValid)
        syncOverlayPresence();
    damageChrome(dirty);
}

void ToolCanvasContext::clearEraseHover()
{
    if (!m_eraseHoverValid)
        return;
    const QRectF dirty = m_eraseHoverDirty;
    m_eraseHoverValid = false;
    m_eraseHoverDirty = QRectF();
    damageChrome(dirty);
}

void ToolCanvasContext::paintEraseHover(QPainter *painter)
{
    if (!painter || !m_eraseHoverValid || !eraseHoverEnabled())
        return;
    if (exclusiveTool() != QLatin1String("erase_brush"))
        return;
    if (m_hub && m_hub->lockedOperation())
        return;
    const double scale = panelScale();
    const double d = epaper::document::eraseMmToWorld(epaper::document::kEraseBrushDiameterMm) * scale;
    const double stroke =
        std::max(1.0, epaper::document::eraseMmToWorld(epaper::document::kEraseHoverStrokeMm) * scale);
    painter->save();
    QPen pen(Qt::black);
    pen.setWidthF(stroke);
    painter->setPen(pen);
    painter->setBrush(Qt::white);
    painter->drawEllipse(m_eraseHoverPanel, d * 0.5, d * 0.5);
    painter->restore();
}

QSizeF ToolCanvasContext::hostSize() const
{
    return m_host ? QSizeF(m_host->width(), m_host->height()) : QSizeF();
}

void ToolCanvasContext::showManipUnavailable(const epaper::document::SmartBounds &wb)
{
    if (!m_doc)
        return;
    const QSizeF host = hostSize();
    m_chrome.showManipUnavailable(wb, *m_doc, host.width(), host.height(), m_repaint, m_emitChanged);
}

void ToolCanvasContext::clearManipUnavailable()
{
    m_chrome.state().encloseRefuseReason.clear();
    m_chrome.state().manipUnavailable.clear();
    m_chrome.state().manipUnavailableRect = QRectF();
}

void ToolCanvasContext::setRefuseReason(const QString &reason)
{
    m_chrome.state().encloseRefuseReason = reason;
    if (m_hub && m_bar)
        m_bar->refresh(m_hub->hostCaps(), m_chrome.state());
    if (m_emitChanged)
        m_emitChanged();
    damageChrome(m_chrome.state().selectionChromeDirty);
}

void ToolCanvasContext::onDocumentOrCameraChanged()
{
    if (!m_doc || !m_selection)
        return;
    std::vector<std::string> keep;
    keep.reserve(m_selection->ids().size());
    for (const std::string &id : m_selection->ids()) {
        if (m_doc->document().find(id))
            keep.push_back(id);
    }
    m_selection->setIds(keep);
    requestChromeRefresh();
}

} // namespace tools
} // namespace epaper
