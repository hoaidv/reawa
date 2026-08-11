#pragma once
/**
 * Viewport map for Epaper input→world.
 * @implements [SRS-EP-02] viewport map apply before next pen sample
 * @implements [SRS-EP-03] map-before-refresh coherency inputs
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
    double scale = 1.0; // uniform > 0
    Aabb drawingRegion{};
    int seq = 0;
    bool valid = false;
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
 * screen/device local → world using Infini formula:
 * world = local / scale - translate  (when local is in the same space as Infini screen)
 * For Epaper panel samples already in panel px mapped to drawing region, we treat
 * local as normalized into the drawing-region local frame then map to world.
 */
inline Vec2 panelToWorld(const ViewportMap &map, double localX, double localY)
{
    // Map panel-local into world via drawingRegion + Infini viewport:
    // Use: world = local / scale - translate (same as Infini screenToWorld).
    return Vec2{localX / map.scale - map.translate.x,
                localY / map.scale - map.translate.y};
}

class ViewportMapStore {
public:
    const ViewportMap &current() const { return m_map; }

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
