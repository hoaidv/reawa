#pragma once

/**
 * ToolContext — SelectionOverlay chrome + manip preview host (ADR-0033).
 * @implements [SRS-EP-12] @implements [ADR-0019]
 */

#include "../canvas_frame.hpp"
#include "document/manipulate.hpp"

#include <QPointF>
#include <QRectF>
#include <QSizeF>
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
    virtual void redrawLiveManip(bool resizing = false) = 0;
    virtual void sendManipPreview(bool resizeGesture) = 0;
    virtual void setInteractionDebug(const std::string &line) = 0;
    virtual void clearOriginPanelRect() = 0;
    virtual void setOriginPanelRect(const QRectF &r) = 0;

    virtual epaper::canvasframe::WorldPt panelToWorld(const QPointF &panel) const = 0;
    virtual QPointF worldToPanel(double wx, double wy) const = 0;
    virtual bool lodOkPanel(const epaper::document::SmartBounds &wb) const = 0;
    virtual QRectF worldBoundsToPanel(const epaper::document::SmartBounds &wb) const = 0;
    virtual int handleIndexAtPanel(const QPointF &panel, double hitDu) const = 0;
    virtual QString exclusiveTool() const = 0;
    virtual bool isSelectionTool() const = 0;
    virtual QSizeF hostSize() const = 0;
    virtual void showManipUnavailable(const epaper::document::SmartBounds &wb) = 0;
    virtual void clearManipUnavailable() = 0;
};

} // namespace tools
} // namespace epaper
