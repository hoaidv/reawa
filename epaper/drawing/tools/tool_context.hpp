#pragma once

/**
 * ToolContext — SelectionOverlay chrome + manip preview host (ADR-0033).
 * @implements [SRS-EP-12] @implements [ADR-0019]
 */

#include <QRectF>
#include <QString>

#include <string>

namespace epaper {
namespace tools {

class ToolContext {
public:
    virtual ~ToolContext() = default;
    virtual void damageChrome(const QRectF &panelRect) = 0;
    virtual void damageChromeSegment(const QRectF &panelRect) = 0;
    virtual void syncOverlayPresence() = 0;
    virtual void setStrokeWaveform(bool penInFlight) = 0;
    virtual void requestChromeRefresh() = 0;
    virtual void resetTransientChromeFlags() = 0;
    virtual void emitChromeChanged() = 0;
    virtual void redrawLiveManip() = 0;
    virtual void sendManipPreview(bool resizeGesture) = 0;
    virtual void setInteractionDebug(const std::string &line) = 0;
    virtual void clearOriginPanelRect() = 0;
};

} // namespace tools
} // namespace epaper
