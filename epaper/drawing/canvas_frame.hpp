#pragma once

/**
 * Canvas frame: orientation, camera region, panel⇄world transforms.
 * Qt-free so host tests can cover it without the ViewModel.
 *
 * Controllers mutate only this state and return FrameIntent; the canvas
 * alone turns intents into Qt effects (chip layout, rasterize, sync).
 */

#include "document/hand_touch.hpp"
#include "document/manipulate.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace epaper {
namespace canvasframe {

/** Panel pixels as doubles — canvas maps to/from QPointF at its edge. */
struct PanelPt {
    double x = 0;
    double y = 0;
};

struct WorldPt {
    double x = 0;
    double y = 0;
};

/** Normalized 0..1 inside the sync frame, with orientation applied. */
struct FrameUv {
    double u = 0;
    double v = 0;
};

/** Camera region in world space, plus "has the camera been established yet". */
struct WorldAabb {
    double minX = 0;
    double minY = 0;
    double maxX = 0;
    double maxY = 0;
    bool valid = false;

    bool nonEmpty() const { return maxX > minX && maxY > minY; }

    /**
     * Geometry alone, for the pure hand-touch helpers — those carry no validity
     * flag. Named both ways so no caller hand-rolls a positional 4-double init.
     * setBox() marks the camera established; override valid after it when the
     * geometry is only conditionally trustworthy.
     */
    handtouch::WorldAabb box() const { return {minX, minY, maxX, maxY}; }
    void setBox(const handtouch::WorldAabb &b)
    {
        minX = b.minX;
        minY = b.minY;
        maxX = b.maxX;
        maxY = b.maxY;
        valid = true;
    }
};

/**
 * What the canvas must do after a frame verb. Keep tiny — grow only when a
 * later controller needs a new notification.
 */
enum class FrameIntent {
    None = 0,
    CameraChanged = 1,
    OrientationChanged = 2,
};

inline FrameIntent operator|(FrameIntent a, FrameIntent b)
{
    return static_cast<FrameIntent>(static_cast<int>(a) | static_cast<int>(b));
}

inline FrameIntent &operator|=(FrameIntent &a, FrameIntent b)
{
    a = a | b;
    return a;
}

inline bool has(FrameIntent mask, FrameIntent bit)
{
    return (static_cast<int>(mask) & static_cast<int>(bit)) != 0;
}

inline std::string normalizeOrientation(const std::string &raw)
{
    if (raw == "portrait" || raw == "gutToLeft")
        return "gutToLeft";
    if (raw == "landscape" || raw == "gutOnTop")
        return "gutOnTop";
    if (raw == "gutAtBottom" || raw == "gutToRight")
        return raw;
    return "gutToLeft";
}

/**
 * Owns orientation + drawing region + panel size. Transforms are pure given
 * that state; mutators return FrameIntent instead of calling the canvas.
 */
struct CanvasFrame {
    std::string orientation = "gutToLeft";
    WorldAabb drawingRegion;
    mutable double panelW = 1.0;
    mutable double panelH = 1.0;

    /** Panel size is observed from the Qt item; mutable so const transforms stay current. */
    void setPanelSize(double w, double h) const
    {
        panelW = std::max(1.0, w);
        panelH = std::max(1.0, h);
    }

    FrameIntent setOrientation(const std::string &raw)
    {
        const std::string next = normalizeOrientation(raw);
        if (next == orientation)
            return FrameIntent::None;
        orientation = next;
        return FrameIntent::OrientationChanged;
    }

    /**
     * Replace camera from host/follow. When @p requireNonEmpty, valid tracks
     * non-degenerate geometry (viewport sync); otherwise setBox's valid=true.
     */
    FrameIntent applyDrawingRegion(const handtouch::WorldAabb &box, bool requireNonEmpty)
    {
        drawingRegion.setBox(box);
        if (requireNonEmpty)
            drawingRegion.valid = drawingRegion.nonEmpty();
        return FrameIntent::CameraChanged;
    }

    FrameIntent ensureLocalDrawingRegion()
    {
        if (drawingRegion.valid)
            return FrameIntent::None;
        drawingRegion.minX = 0;
        drawingRegion.minY = 0;
        drawingRegion.maxX = panelW;
        drawingRegion.maxY = panelH;
        drawingRegion.valid = true;
        return FrameIntent::CameraChanged;
    }

    bool landscape() const
    {
        return orientation == "gutOnTop" || orientation == "gutAtBottom"
            || orientation == "landscape";
    }

    bool invertX() const
    {
        return orientation == "gutAtBottom" || orientation == "gutToRight";
    }

    bool invertY() const
    {
        return orientation == "gutAtBottom" || orientation == "gutToRight";
    }

    FrameUv panelToFrameUv(PanelPt panel) const
    {
        double nx = 0;
        double ny = 0;
        if (landscape()) {
            nx = 1.0 - panel.y / panelH;
            ny = panel.x / panelW;
        } else {
            nx = panel.x / panelW;
            ny = panel.y / panelH;
        }
        if (invertX())
            nx = 1.0 - nx;
        if (invertY())
            ny = 1.0 - ny;
        return FrameUv{nx, ny};
    }

    PanelPt frameUvToPanel(FrameUv uv) const
    {
        double nx = uv.u;
        double ny = uv.v;
        if (invertX())
            nx = 1.0 - nx;
        if (invertY())
            ny = 1.0 - ny;
        if (landscape())
            return PanelPt{ny * panelW, (1.0 - nx) * panelH};
        return PanelPt{nx * panelW, ny * panelH};
    }

    WorldPt panelToWorld(PanelPt panel) const
    {
        if (!drawingRegion.valid)
            return WorldPt{panel.x, panel.y};
        const FrameUv uv = panelToFrameUv(panel);
        WorldPt w;
        handtouch::mapUvToWorld(drawingRegion.box(), uv.u, uv.v, &w.x, &w.y);
        return w;
    }

    PanelPt worldToPanel(double wx, double wy) const
    {
        if (!drawingRegion.valid)
            return PanelPt{wx, wy};
        const double rw = drawingRegion.maxX - drawingRegion.minX;
        const double rh = drawingRegion.maxY - drawingRegion.minY;
        if (rw <= 0 || rh <= 0)
            return PanelPt{};
        return frameUvToPanel(
            FrameUv{(wx - drawingRegion.minX) / rw, (wy - drawingRegion.minY) / rh});
    }

    PanelPt worldToPanel(WorldPt w) const { return worldToPanel(w.x, w.y); }

    double panelScale() const
    {
        if (!drawingRegion.valid)
            return 1.0;
        const double rw = drawingRegion.maxX - drawingRegion.minX;
        if (rw <= 0.0)
            return 1.0;
        return panelW / rw;
    }

    bool viewportZoomedOut() const
    {
        if (!drawingRegion.valid)
            return false;
        const double rw = drawingRegion.maxX - drawingRegion.minX;
        const double rh = drawingRegion.maxY - drawingRegion.minY;
        if (rw <= 0.0 || rh <= 0.0)
            return false;
        const double sx = panelW / rw;
        const double sy = panelH / rh;
        return std::min(sx, sy) < 1.0 - 1e-6;
    }

    /** Panel AABB of a world SmartBounds-shaped rect (x,y,w,h). */
    void worldBoundsToPanel(double x, double y, double w, double h, PanelPt *tl,
                            PanelPt *br) const
    {
        *tl = worldToPanel(x, y);
        *br = worldToPanel(x + w, y + h);
    }

    bool lodOkPanel(double x, double y, double w, double h) const
    {
        if (!viewportZoomedOut())
            return true;
        PanelPt tl;
        PanelPt br;
        worldBoundsToPanel(x, y, w, h, &tl, &br);
        const double pw = std::abs(br.x - tl.x);
        const double ph = std::abs(br.y - tl.y);
        return document::lodAllowsPanel(pw, ph);
    }
};

} // namespace canvasframe
} // namespace epaper
