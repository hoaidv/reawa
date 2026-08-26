#pragma once

/**
 * ToolContext — SelectionOverlay Tool UI host (stroke chrome + context QML props).
 * Not primary ToolChip; not document blit.
 * @implements [SRS-EP-12] @implements [ADR-0019]
 */

#include <QPainter>
#include <QRectF>

namespace epaper {
namespace tools {

class ToolContext {
public:
    virtual ~ToolContext() = default;
    virtual void damageChrome(const QRectF &panelRect) = 0;
    virtual void syncOverlayPresence() = 0;
    virtual void setStrokeWaveform(bool penInFlight) = 0;
    /** Settled + live chrome paint entry (host may delegate to Mode/Op). */
    virtual void requestChromeRefresh() = 0;
};

} // namespace tools
} // namespace epaper
