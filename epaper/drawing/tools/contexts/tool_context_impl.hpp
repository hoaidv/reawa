#pragma once

/**
 * ToolContextImpl — ToolContext adapter; host ports + overlay dirty-union.
 * Forwards paint / sync / refresh to the active Mode. Zero exclusive-id compares.
 * @implements [SRS-EP-12] @implements [ADR-0035]
 */

#include "tool_context.hpp"

#include <QPointF>
#include <QRectF>
#include <functional>

class QPainter;
class ToolCanvasItem;

namespace epaper {
namespace tools {

class SessionDocContext;
class InputHub;

class ToolContextImpl final : public ToolContext {
public:
    explicit ToolContextImpl(ToolCanvasItem *host);

    void setDoc(SessionDocContext *doc) { m_doc = doc; }
    void setHub(InputHub *hub) { m_hub = hub; }

    void setRepaint(std::function<void(const QRectF &)> fn) { m_repaint = std::move(fn); }
    void setSetVisible(std::function<void(bool)> fn) { m_setVisible = std::move(fn); }
    void setSetStrokeWaveform(std::function<void(bool)> fn)
    {
        m_setStrokeWaveform = std::move(fn);
    }

    void paintOverlay(QPainter *painter);

    void damageChrome(const QRectF &panelRect) override;
    void damageChromeSegment(const QRectF &panelRect) override;
    void syncOverlayPresence() override;
    void setOverlayVisible(bool on) override;
    void setStrokeWaveform(bool penInFlight) override;
    void refreshChrome() override;

    epaper::canvasframe::WorldPt panelToWorld(const QPointF &panel) const override;
    QPointF worldToPanel(double wx, double wy) const override;
    bool lodOkPanel(const epaper::document::SmartBounds &wb) const override;
    QRectF worldBoundsToPanel(const epaper::document::SmartBounds &wb) const override;
    double panelScale() const override;
    QSizeF hostSize() const override;

private:
    ToolCanvasItem *m_host = nullptr;
    SessionDocContext *m_doc = nullptr;
    InputHub *m_hub = nullptr;
    QRectF m_overlayPrev;

    std::function<void(const QRectF &)> m_repaint;
    std::function<void(bool)> m_setVisible;
    std::function<void(bool)> m_setStrokeWaveform;
};

} // namespace tools
} // namespace epaper
