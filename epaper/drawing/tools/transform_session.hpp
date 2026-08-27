#pragma once

/**
 * Live move/resize math in world space. Qt-free; TransformGesture applies ports.
 * @implements [SRS-EP-11]
 */

#include "../canvas_frame.hpp"
#include "document/manipulate.hpp"

#include <cmath>
#include <string>

namespace epaper {
namespace tools {

struct TransformResult {
    bool moved = false;
    bool resized = false;
};

struct TransformSession {
    bool active = false;
    std::string nodeId;
    document::ResizeHandle handle = document::ResizeHandle::None;
    canvasframe::WorldPt originWorld{};
    canvasframe::WorldPt currentWorld{};
    document::SmartTransform originT{};
    document::SmartTransform liveT{};
    document::SmartBounds originB{};
    document::SmartBounds liveB{};

    bool resizing() const { return handle != document::ResizeHandle::None; }

    void reset() { *this = TransformSession{}; }

    void begin(std::string id, document::ResizeHandle h, canvasframe::WorldPt world,
               document::SmartTransform t, document::SmartBounds b)
    {
        *this = TransformSession{};
        active = true;
        nodeId = std::move(id);
        handle = h;
        originWorld = world;
        currentWorld = world;
        originT = t;
        liveT = t;
        originB = b;
        liveB = b;
    }

    /** @p inkScaleMode — from the live node (withBounds / fixedInk). */
    void apply(canvasframe::WorldPt world, const std::string &inkScaleMode)
    {
        if (!active)
            return;
        currentWorld = world;
        const double dx = currentWorld.x - originWorld.x;
        const double dy = currentWorld.y - originWorld.y;
        if (handle == document::ResizeHandle::None) {
            liveT = originT;
            liveT.x = originT.x + dx;
            liveT.y = originT.y + dy;
            liveT.rotation = 0;
            liveB = originB;
        } else {
            const document::WorldBox origin = document::originWorldAabb(originB, originT);
            const document::WorldBox nw =
                document::resizeWorldAabbFromHandle(origin, handle, world.x, world.y);
            const auto mapped =
                document::smartTransformFromWorldAabb(nw, originB, originT, inkScaleMode);
            liveT = mapped.transform;
            liveB = mapped.bounds;
        }
    }

    /** Decide commit vs abort; fields stay readable until the gesture reset()s. */
    TransformResult commit()
    {
        TransformResult r;
        if (!active)
            return r;
        const double dx = liveT.x - originT.x;
        const double dy = liveT.y - originT.y;
        r.resized = handle != document::ResizeHandle::None;
        r.moved = r.resized || std::hypot(dx, dy) >= 2.0
            || std::hypot(liveT.scaleX - originT.scaleX, liveT.scaleY - originT.scaleY) > 0.01
            || std::abs(liveB.width - originB.width) > 0.5;
        return r;
    }
};

inline document::ResizeHandle handleFromIndex(int index)
{
    using document::ResizeHandle;
    static const ResizeHandle k[] = {ResizeHandle::Nw, ResizeHandle::N,  ResizeHandle::Ne,
                                     ResizeHandle::E,  ResizeHandle::Se, ResizeHandle::S,
                                     ResizeHandle::Sw, ResizeHandle::W};
    if (index < 0 || index >= 8)
        return ResizeHandle::None;
    return k[index];
}

} // namespace tools
} // namespace epaper
