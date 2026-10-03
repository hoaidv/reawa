#pragma once

/**
 * ToolContext — Qt host ports only (ADR-0035). Overlay policy lives on Mode.
 * @implements [SRS-EP-12] @implements [ADR-0019] @implements [ADR-0035]
 */

#include "../../canvas_frame.hpp"
#include "document/manipulate.hpp"

#include <QPointF>
#include <QRectF>
#include <QSizeF>

namespace epaper {
namespace tools {

class ToolContext {
public:
    virtual ~ToolContext() = default;
    virtual void damageChrome(const QRectF &panelRect) = 0;
    virtual void damageChromeSegment(const QRectF &panelRect) = 0;
    virtual void syncOverlayPresence() = 0;
    virtual void setOverlayVisible(bool on) = 0;
    virtual void setStrokeWaveform(bool penInFlight) = 0;
    virtual void refreshChrome() = 0;

    virtual epaper::canvasframe::WorldPt panelToWorld(const QPointF &panel) const = 0;
    virtual QPointF worldToPanel(double wx, double wy) const = 0;
    virtual bool lodOkPanel(const epaper::document::SmartBounds &wb) const = 0;
    virtual QRectF worldBoundsToPanel(const epaper::document::SmartBounds &wb) const = 0;
    virtual double panelScale() const = 0;
    virtual QSizeF hostSize() const = 0;
};

} // namespace tools
} // namespace epaper
