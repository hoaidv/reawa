#pragma once

/**
 * ToolCanvasContext — ToolContext adapter; owns SelectionOverlay chrome logic.
 * @implements [SRS-EP-12]
 */

#include "tool_chrome.hpp"
#include "tool_context.hpp"

#include <functional>

class QPainter;
class ToolCanvasItem;

namespace epaper {
namespace tools {

class SessionDocContext;
class InputHub;
class SelectionContext;
class SelectionContextBar;

class ToolCanvasContext final : public ToolContext {
public:
    explicit ToolCanvasContext(ToolCanvasItem *host);

    ToolChrome &chrome() { return m_chrome; }
    const ToolChrome &chrome() const { return m_chrome; }

    void setDoc(SessionDocContext *doc) { m_doc = doc; }
    void setSelection(SelectionContext *sel) { m_selection = sel; }
    void setHub(InputHub *hub) { m_hub = hub; }
    void setSelectionBar(SelectionContextBar *bar) { m_bar = bar; }

    void setRepaint(std::function<void(const QRectF &)> fn) { m_repaint = std::move(fn); }
    void setSetVisible(std::function<void(bool)> fn) { m_setVisible = std::move(fn); }
    void setEmitChromeChanged(std::function<void()> fn) { m_emitChanged = std::move(fn); }
    void setSetStrokeWaveform(std::function<void(bool)> fn)
    {
        m_setStrokeWaveform = std::move(fn);
    }

    void paintOverlay(QPainter *painter);

    void damageChrome(const QRectF &panelRect) override;
    void damageChromeSegment(const QRectF &panelRect) override;
    void syncOverlayPresence() override;
    void setStrokeWaveform(bool penInFlight) override;
    void requestChromeRefresh() override;
    void resetTransientChromeFlags() override;
    void emitChromeChanged() override;
    void redrawLiveManip(bool resizing = false) override;
    void sendManipPreview(bool resizeGesture) override;
    void setInteractionDebug(const std::string &line) override;
    void clearOriginPanelRect() override;
    void setOriginPanelRect(const QRectF &r) override;

    epaper::canvasframe::WorldPt panelToWorld(const QPointF &panel) const override;
    QPointF worldToPanel(double wx, double wy) const override;
    bool lodOkPanel(const epaper::document::SmartBounds &wb) const override;
    QRectF worldBoundsToPanel(const epaper::document::SmartBounds &wb) const override;
    QString exclusiveTool() const override;
    bool isSelectionTool() const override;
    QSizeF hostSize() const override;
    void showManipUnavailable(const epaper::document::SmartBounds &wb) override;
    void clearManipUnavailable() override;
    void setRefuseReason(const QString &reason) override;
    void onDocumentOrCameraChanged() override;

private:
    ToolCanvasItem *m_host = nullptr;
    ToolChrome m_chrome;
    SessionDocContext *m_doc = nullptr;
    SelectionContext *m_selection = nullptr;
    InputHub *m_hub = nullptr;
    SelectionContextBar *m_bar = nullptr;

    std::function<void(const QRectF &)> m_repaint;
    std::function<void(bool)> m_setVisible;
    std::function<void()> m_emitChanged;
    std::function<void(bool)> m_setStrokeWaveform;
};

} // namespace tools
} // namespace epaper
