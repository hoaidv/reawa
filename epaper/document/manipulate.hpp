#pragma once
/**
 * Live move/resize math + kind-agnostic gesture routing.
 * @implements [SRS-EP-11] selection hit-test move resize
 * @implements [SRS-EP-14] CHL-0004…0007 bars + capability conformance
 *
 * Port of infini/src/document/selection.ts world AABB mapping. Router never
 * branches on node kind — only on CapabilityDescriptor verbs.
 */

#include "capability.hpp"
#include "membership.hpp"
#include "surround_create.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>

namespace epaper {
namespace document {

constexpr double kHandleVisualDu = 28.0;
constexpr double kHandleHitDu = 56.0;
constexpr double kLodMinAxisDu = 96.0;

enum class ResizeHandle { None, Nw, N, Ne, E, Se, S, Sw, W };

enum class GestureKind { None, Marquee, SelectMove, Resize, ToggleMode, Deselect, Unavailable };

struct WorldBox {
    double minX = 0;
    double minY = 0;
    double maxX = 0;
    double maxY = 0;
};

inline WorldBox boxFromSmart(const SmartBounds &b)
{
    WorldBox w;
    w.minX = b.x;
    w.minY = b.y;
    w.maxX = b.x + b.width;
    w.maxY = b.y + b.height;
    return w;
}

inline SmartBounds smartFromBox(const WorldBox &w)
{
    SmartBounds b;
    b.x = w.minX;
    b.y = w.minY;
    b.width = w.maxX - w.minX;
    b.height = w.maxY - w.minY;
    return b;
}

inline bool lodAllows(double worldW, double worldH, double panelScale)
{
    const double ax = std::min(std::abs(worldW), std::abs(worldH)) * panelScale;
    return ax >= kLodMinAxisDu;
}

/** SRS-EP-11: smaller *on-panel* axis (after world→panel), not world×uniform X scale. */
inline bool lodAllowsPanel(double panelW, double panelH)
{
    return std::min(std::abs(panelW), std::abs(panelH)) >= kLodMinAxisDu;
}

/** Kind-agnostic bounds provider. */
inline bool boundsOf(const DocNode &n, SmartBounds &out) { return nodeWorldAabb(n, out); }

inline WorldBox originWorldAabb(const SmartBounds &local, const SmartTransform &t)
{
    DocNode fake;
    fake.kind = NodeKind::SmartGroup;
    fake.smartBounds = local;
    fake.transform = t;
    return boxFromSmart(smartGroupWorldBounds(fake));
}

inline WorldBox resizeWorldAabbFromHandle(WorldBox box, ResizeHandle handle, double px, double py)
{
    switch (handle) {
    case ResizeHandle::E:
        box.maxX = px;
        break;
    case ResizeHandle::W:
        box.minX = px;
        break;
    case ResizeHandle::S:
        box.maxY = py;
        break;
    case ResizeHandle::N:
        box.minY = py;
        break;
    case ResizeHandle::Se:
        box.maxX = px;
        box.maxY = py;
        break;
    case ResizeHandle::Sw:
        box.minX = px;
        box.maxY = py;
        break;
    case ResizeHandle::Ne:
        box.maxX = px;
        box.minY = py;
        break;
    case ResizeHandle::Nw:
        box.minX = px;
        box.minY = py;
        break;
    default:
        break;
    }
    if (box.minX > box.maxX)
        std::swap(box.minX, box.maxX);
    if (box.minY > box.maxY)
        std::swap(box.minY, box.maxY);
    if (box.maxX - box.minX < 1)
        box.maxX = box.minX + 1;
    if (box.maxY - box.minY < 1)
        box.maxY = box.minY + 1;
    return box;
}

struct MappedTransform {
    SmartTransform transform;
    SmartBounds bounds;
};

inline MappedTransform smartTransformFromWorldAabb(const WorldBox &world, const SmartBounds &localBounds,
                                                   const SmartTransform &base, const std::string &mode)
{
    // Frame always follows the handle via scale. `mode` only changes how *content* is drawn
    // (fixedInk: unscaled, top-left; withBounds: scaled). Boundary always uses this scale.
    (void)mode;
    const double w = std::max(1.0, world.maxX - world.minX);
    const double h = std::max(1.0, world.maxY - world.minY);
    MappedTransform out;
    out.transform = base;
    out.transform.rotation = 0;
    const double scaleX = w / std::max(1.0, localBounds.width);
    const double scaleY = h / std::max(1.0, localBounds.height);
    out.transform.x = world.minX - localBounds.x * scaleX;
    out.transform.y = world.minY - localBounds.y * scaleY;
    out.transform.scaleX = scaleX;
    out.transform.scaleY = scaleY;
    out.bounds = localBounds;
    return out;
}

inline GestureKind resolvePress(const CapabilityDescriptor &cap, bool lodOk, bool handleHit,
                                bool toggleHit, bool nodeHit)
{
    if (nodeHit && !lodOk)
        return GestureKind::Unavailable;
    if (toggleHit && cap.has(Verb::SetInkScaleMode) && lodOk)
        return GestureKind::ToggleMode;
    if (handleHit && cap.has(Verb::Resize) && lodOk)
        return GestureKind::Resize;
    if (nodeHit && cap.has(Verb::Move) && lodOk)
        return GestureKind::SelectMove;
    if (nodeHit && cap.has(Verb::Select) && lodOk)
        return GestureKind::SelectMove;
    if (!nodeHit)
        return GestureKind::Marquee;
    return GestureKind::None;
}

inline ResizeHandle hitResizeHandlePanel(const SmartBounds &worldAabb, double panelX, double panelY,
                                         double panelScale,
                                         const std::function<void(double, double, double *, double *)> &worldToPanel,
                                         double hitDu = kHandleHitDu)
{
    const double xs[8] = {worldAabb.x,
                          worldAabb.x + worldAabb.width * 0.5,
                          worldAabb.x + worldAabb.width,
                          worldAabb.x + worldAabb.width,
                          worldAabb.x + worldAabb.width,
                          worldAabb.x + worldAabb.width * 0.5,
                          worldAabb.x,
                          worldAabb.x};
    const double ys[8] = {worldAabb.y,
                          worldAabb.y,
                          worldAabb.y,
                          worldAabb.y + worldAabb.height * 0.5,
                          worldAabb.y + worldAabb.height,
                          worldAabb.y + worldAabb.height,
                          worldAabb.y + worldAabb.height,
                          worldAabb.y + worldAabb.height * 0.5};
    const ResizeHandle hs[8] = {ResizeHandle::Nw, ResizeHandle::N, ResizeHandle::Ne, ResizeHandle::E,
                                ResizeHandle::Se, ResizeHandle::S, ResizeHandle::Sw, ResizeHandle::W};
    const double half = hitDu * 0.5;
    (void)panelScale;
    for (int i = 0; i < 8; ++i) {
        double px = 0, py = 0;
        worldToPanel(xs[i], ys[i], &px, &py);
        if (std::abs(px - panelX) <= half && std::abs(py - panelY) <= half)
            return hs[i];
    }
    return ResizeHandle::None;
}

inline void applyLiveGeometry(DocNode &n, const SmartTransform &t, const SmartBounds &b)
{
    n.transform = t;
    n.smartBounds = b;
}

} // namespace document
} // namespace epaper
