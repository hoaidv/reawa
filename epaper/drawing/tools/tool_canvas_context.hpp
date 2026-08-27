#pragma once

/**
 * ToolCanvasContext — ToolContext adapter for SelectionOverlay chrome.
 * @implements [SRS-EP-12]
 */

#include "tool_context.hpp"

class ToolCanvasItem;

namespace epaper {
namespace tools {

class ToolCanvasContext final : public ToolContext {
public:
    explicit ToolCanvasContext(ToolCanvasItem *host);

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

    void setHost(ToolCanvasItem *host) { m_host = host; }

private:
    ToolCanvasItem *m_host = nullptr;
};

} // namespace tools
} // namespace epaper
