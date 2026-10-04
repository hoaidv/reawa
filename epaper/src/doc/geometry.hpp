#pragma once
#include <algorithm>
#include <cstdint>

/**
 * Distinct from world point. This can be passed anywhere without worrying about misunderstanding.
 * Why?
 * A using WorldPt = Pt would compile a world point passed where a local point is required. 
 * This distinction make us cautious when passing these around.
 */
struct Pt { double x = 0, y = 0; };

/**
 * Document local space
 */
struct WorldPt { double x = 0, y = 0; };

/**
 * Panel pixels, origin top-left
 */
struct PanelPt { double x = 0, y = 0; };


/**
 * [0, 1] inside the oriented sync frame
 */
struct FrameUv { double u = 0, v = 0; };


struct Aabb { double minX = 0, minY = 0, maxX = 0, maxY = 0; };

/** Axis-aligned bounding box, in the document local space */
struct WorldAabb {  double minX = 0, minY = 0, maxX = 0, maxY = 0; };

/** Transformation on a node */
struct Transform {
    double  
        sx = 1, sy = 1, 
        rotation = 0, 
        tx = 0, ty = 0;
};

struct ResizeMap {
    double 
        sx = 1, sy = 1, 
        tx = 0, ty = 0;
};



enum class Knob : uint8_t { Nw, N, Ne, E, Se, S, Sw, W };

enum class Side : uint8_t { Top, Right, Bottom, Left };


// ==== Boxes ==========================================================================
// Aabb and WorldAabb share the rule, so one template covers both. 
// A box is empty when max <= min on either axis. The zero box {0, 0, 0, 0} is empty.
// =====================================================================================
template <class Box>
bool empty(const Box& b)
{
    return b.maxX <= b.minX || b.maxY <= b.minY;
}

template <class Box>
Box unite(Box a, Box b)
{
    if (empty(a))
        return b;
    if (empty(b))
        return a;
    return Box{
        std::min(a.minX, b.minX), std::min(a.minY, b.minY),
        std::max(a.maxX, b.maxX), std::max(a.maxY, b.maxY)};
}

template <class Box>
Box intersect(Box a, Box b)
{
    if (empty(a) || empty(b))
        return Box{};
    Box r{
        std::max(a.minX, b.minX), std::max(a.minY, b.minY),
        std::min(a.maxX, b.maxX), std::min(a.maxY, b.maxY)};
    return empty(r) ? Box{} : r;
}



// ==== ResizeMap ==========================================================================
// A knob drag first builds the new rectangle, with the opposite side left where it was. 
// The dragged edge stops 1 world unit short of that side, so the map never flips. 
// Then the map is sx = w' / w, tx = x' − x · sx, and the same on y. 
// The old box must already be non-empty.
// =====================================================================================


inline constexpr double kMinSize = 1;

/**
 * px,py are the pointer's current position in the same space as the box, not a delta.
 * The dragged edge/corner is set to that coordinate.
 * The opposite edge stays where it was.
 */
inline Aabb resizedBox(Aabb box, Knob knob, double px, double py)
{
    switch (knob) {
    case Knob::E:
        box.maxX = std::max(px, box.minX + kMinSize);
        break;
    case Knob::W:
        box.minX = std::min(px, box.maxX - kMinSize);
        break;
    case Knob::S:
        box.maxY = std::max(py, box.minY + kMinSize);
        break;
    case Knob::N:
        box.minY = std::min(py, box.maxY - kMinSize);
        break;
    case Knob::Se:
        box.maxX = std::max(px, box.minX + kMinSize);
        box.maxY = std::max(py, box.minY + kMinSize);
        break;
    case Knob::Sw:
        box.minX = std::min(px, box.maxX - kMinSize);
        box.maxY = std::max(py, box.minY + kMinSize);
        break;
    case Knob::Ne:
        box.maxX = std::max(px, box.minX + kMinSize);
        box.minY = std::min(py, box.maxY - kMinSize);
        break;
    case Knob::Nw:
        box.minX = std::min(px, box.maxX - kMinSize);
        box.minY = std::min(py, box.maxY - kMinSize);
        break;
    }
    return box;
}

/**
 * For any box, its ResizeMap when moving a knob is:
 * resizeMap(box, resizedBox(box, knob, px, py))
 */
inline ResizeMap resizeMap(Aabb from, Aabb to)
{
    const double w = from.maxX - from.minX;
    const double h = from.maxY - from.minY;
    ResizeMap m;
    m.sx = (to.maxX - to.minX) / w;
    m.sy = (to.maxY - to.minY) / h;
    m.tx = to.minX - from.minX * m.sx;
    m.ty = to.minY - from.minY * m.sy;
    return m;
}

inline Pt apply(ResizeMap m, Pt p)
{
    return Pt{p.x * m.sx + m.tx, p.y * m.sy + m.ty};
}


// ==== Panel map ==========================================================================
// 
// =====================================================================================

enum class Orientation : uint8_t { GutToLeft, GutOnTop, GutAtBottom, GutToRight };

struct FrameMap {
    Orientation orientation = Orientation::GutToLeft;
    double panelW = 1;
    double panelH = 1;
    WorldAabb region{};
};

inline bool landscape(Orientation o)
{
    return o == Orientation::GutOnTop || o == Orientation::GutAtBottom;
}

inline bool inverted(Orientation o)
{
    return o == Orientation::GutAtBottom || o == Orientation::GutToRight;
}

inline FrameUv panelToFrameUv(const FrameMap& f, PanelPt panel)
{
    double u = 0;
    double v = 0;
    if (landscape(f.orientation)) {
        u = 1.0 - panel.y / f.panelH;
        v = panel.x / f.panelW;
    } else {
        u = panel.x / f.panelW;
        v = panel.y / f.panelH;
    }
    if (inverted(f.orientation)) {
        u = 1.0 - u;
        v = 1.0 - v;
    }
    return FrameUv{u, v};
}

inline PanelPt frameUvToPanel(const FrameMap& f, FrameUv uv)
{
    double u = uv.u;
    double v = uv.v;
    if (inverted(f.orientation)) {
        u = 1.0 - u;
        v = 1.0 - v;
    }
    if (landscape(f.orientation))
        return PanelPt{v * f.panelW, (1.0 - u) * f.panelH};
    return PanelPt{u * f.panelW, v * f.panelH};
}

inline WorldPt panelToWorld(const FrameMap& f, PanelPt panel)
{
    if (empty(f.region))
        return WorldPt{panel.x, panel.y};
    const FrameUv uv = panelToFrameUv(f, panel);
    const double rw = f.region.maxX - f.region.minX;
    const double rh = f.region.maxY - f.region.minY;
    return WorldPt{f.region.minX + uv.u * rw, f.region.minY + uv.v * rh};
}

inline PanelPt worldToPanel(const FrameMap& f, WorldPt w)
{
    if (empty(f.region))
        return PanelPt{w.x, w.y};
    const double rw = f.region.maxX - f.region.minX;
    const double rh = f.region.maxY - f.region.minY;
    return frameUvToPanel(f, FrameUv{
        (w.x - f.region.minX) / rw,
        (w.y - f.region.minY) / rh});
}

inline WorldAabb panelAabbToWorld(const FrameMap& f, Aabb panel)
{
    if (empty(panel))
        return WorldAabb{};
    const PanelPt corners[4] = {
        {panel.minX, panel.minY}, {panel.maxX, panel.minY},
        {panel.minX, panel.maxY}, {panel.maxX, panel.maxY}};
    WorldPt w = panelToWorld(f, corners[0]);
    WorldAabb out{w.x, w.y, w.x, w.y};
    for (int i = 1; i < 4; ++i) {
        w = panelToWorld(f, corners[i]);
        out.minX = std::min(out.minX, w.x);
        out.minY = std::min(out.minY, w.y);
        out.maxX = std::max(out.maxX, w.x);
        out.maxY = std::max(out.maxY, w.y);
    }
    return out;
}