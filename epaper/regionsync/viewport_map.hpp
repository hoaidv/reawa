#pragma once
/**
 * Viewport map for Epaper input→world.
 * @implements [SRS-EP-02] viewport map apply before next pen sample
 * @implements [SRS-EP-03] map-before-refresh coherency inputs
 * @implements [ADR-0012] panel→drawingRegion normalize + stroke panel width
 */

#include <cmath>
#include <optional>
#include <string>

namespace epaper::regionsync {

struct Vec2 {
    double x = 0;
    double y = 0;
};

struct Aabb {
    double minX = 0;
    double minY = 0;
    double maxX = 0;
    double maxY = 0;
};

struct ViewportMap {
    Vec2 translate{};
    double scale = 1.0; // uniform > 0 (Infini canvas scale; informational)
    Aabb drawingRegion{};
    int seq = 0;
    bool valid = false;
    double panelW = 0;
    double panelH = 0;
};

struct ViewportMessage {
    Vec2 translate{};
    double scale = 1.0;
    Aabb drawingRegion{};
    int seq = 0;
};

inline bool viewportMessageOk(const ViewportMessage &m)
{
    return m.scale > 0.0 && std::isfinite(m.scale)
        && std::isfinite(m.translate.x) && std::isfinite(m.translate.y)
        && m.drawingRegion.maxX >= m.drawingRegion.minX
        && m.drawingRegion.maxY >= m.drawingRegion.minY;
}

/**
 * Panel local → world via normalized mapping into drawingRegion (SRS-EP-02).
 * Does NOT use Infini screen formula (local/scale - translate).
 */
inline Vec2 panelToWorld(const ViewportMap &map, double localX, double localY)
{
    const double pw = map.panelW > 0 ? map.panelW : 1.0;
    const double ph = map.panelH > 0 ? map.panelH : 1.0;
    const double u = localX / pw;
    const double v = localY / ph;
    return Vec2{
        map.drawingRegion.minX + u * (map.drawingRegion.maxX - map.drawingRegion.minX),
        map.drawingRegion.minY + v * (map.drawingRegion.maxY - map.drawingRegion.minY),
    };
}

/** Uniform panel px per world unit (X extent). @implements [ADR-0012] */
inline double panelScaleFromRegion(const ViewportMap &map)
{
    const double worldW = map.drawingRegion.maxX - map.drawingRegion.minX;
    const double pw = map.panelW > 0 ? map.panelW : 1.0;
    if (worldW <= 0.0)
        return 1.0;
    return pw / worldW;
}

inline double strokePanelWidth(double strokeWidthWorld, const ViewportMap &map)
{
    return strokeWidthWorld * panelScaleFromRegion(map);
}

class ViewportMapStore {
public:
    const ViewportMap &current() const { return m_map; }

    void setPanelSize(double w, double h)
    {
        m_map.panelW = w;
        m_map.panelH = h;
    }

    /** Apply Infini viewport immediately; keep last-good on bad message. */
    bool applyViewport(const ViewportMessage &msg, std::string *err = nullptr)
    {
        if (!viewportMessageOk(msg)) {
            if (err)
                *err = "viewport missing/invalid fields; keeping last-good map";
            return false;
        }
        m_map.translate = msg.translate;
        m_map.scale = msg.scale;
        m_map.drawingRegion = msg.drawingRegion;
        m_map.seq = msg.seq;
        m_map.valid = true;
        return true;
    }

private:
    ViewportMap m_map{};
};

} // namespace epaper::regionsync
