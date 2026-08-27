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
class SelectionManipController;

class ToolCanvasContext final : public ToolContext {
public:
    explicit ToolCanvasContext(ToolCanvasItem *host);

    ToolChrome &chrome() { return m_chrome; }
    const ToolChrome &chrome() const { return m_chrome; }

    void setDoc(SessionDocContext *doc) { m_doc = doc; }
    void setSelection(epaper::selection::SelectionSession *sel) { m_selection = sel; }
    void setManip(epaper::manip::ManipSession *manip) { m_manip = manip; }
    void setHub(InputHub *hub) { m_hub = hub; }
    void setSelectionManip(SelectionManipController *ctrl) { m_selManip = ctrl; }

    void setIsSelectionTool(std::function<bool()> fn) { m_isSelectionTool = std::move(fn); }
    void setRepaint(std::function<void(const QRectF &)> fn) { m_repaint = std::move(fn); }
    void setSetVisible(std::function<void(bool)> fn) { m_setVisible = std::move(fn); }
    void setEmitChromeChanged(std::function<void()> fn) { m_emitChanged = std::move(fn); }
    void setSetStrokeWaveform(std::function<void(bool)> fn)
    {
        m_setStrokeWaveform = std::move(fn);
    }
    void setSendManipPreview(std::function<void(bool)> fn) { m_sendManipPreview = std::move(fn); }
    void setSetInteractionDebug(std::function<void(const std::string &)> fn)
    {
        m_setDebug = std::move(fn);
    }

    void paintOverlay(QPainter *painter);

    void damageChrome(const QRectF &panelRect) override;
    void damageChromeSegment(const QRectF &panelRect) override;
    void syncOverlayPresence() override;
    void setStrokeWaveform(bool penInFlight) override;
    void requestChromeRefresh() override;
    void resetTransientChromeFlags() override;
    void emitChromeChanged() override;
    void redrawLiveManip() override;
    void sendManipPreview(bool resizeGesture) override;
    void setInteractionDebug(const std::string &line) override;
    void clearOriginPanelRect() override;

private:
    ToolCanvasItem *m_host = nullptr;
    ToolChrome m_chrome;
    SessionDocContext *m_doc = nullptr;
    epaper::selection::SelectionSession *m_selection = nullptr;
    epaper::manip::ManipSession *m_manip = nullptr;
    InputHub *m_hub = nullptr;
    SelectionManipController *m_selManip = nullptr;

    std::function<bool()> m_isSelectionTool;
    std::function<void(const QRectF &)> m_repaint;
    std::function<void(bool)> m_setVisible;
    std::function<void()> m_emitChanged;
    std::function<void(bool)> m_setStrokeWaveform;
    std::function<void(bool)> m_sendManipPreview;
    std::function<void(const std::string &)> m_setDebug;
};

} // namespace tools
} // namespace epaper
