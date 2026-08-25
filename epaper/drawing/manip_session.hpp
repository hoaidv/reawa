#pragma once

/**
 * Live move/resize session in world space. Qt-free replacement for field-poking
 * around ManipDrag. Canvas maps panel→world once at the handler edge.
 *
 * @implements [SRS-EP-11] selection hit-test move resize
 */

#include "canvas_frame.hpp"
#include "document/manipulate.hpp"

#include <cmath>
#include <string>

namespace epaper {
namespace manip {

enum class ManipIntent : int {
    None = 0,
    BeginGesture = 1 << 0,
    ApplyLiveGeometry = 1 << 1,
    RefreshBoundConnectors = 1 << 2,
    PreviewFrame = 1 << 3,
    SendPreview = 1 << 4,
    Redraw = 1 << 5,
    RefreshAllConnectors = 1 << 6,
    CaptureOriginPunches = 1 << 7,
    RefreshChrome = 1 << 8,
    AbortGesture = 1 << 9,
    CommitTransform = 1 << 10,
    ScheduleRasterize = 1 << 11,
    NotifyHistory = 1 << 12,
    FlushWire = 1 << 13,
    UpdatePunch = 1 << 14,
};

inline ManipIntent operator|(ManipIntent a, ManipIntent b)
{
    return static_cast<ManipIntent>(static_cast<int>(a) | static_cast<int>(b));
}
inline ManipIntent &operator|=(ManipIntent &a, ManipIntent b)
{
    a = a | b;
    return a;
}
inline bool has(ManipIntent mask, ManipIntent bit)
{
    return (static_cast<int>(mask) & static_cast<int>(bit)) != 0;
}

struct ManipResult {
    ManipIntent intent = ManipIntent::None;
    bool moved = false;
    bool resized = false;
};

struct ManipSession {
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

    void clearNodeId() { nodeId.clear(); }

    void reset() { *this = ManipSession{}; }

    ManipResult begin(std::string id, document::ResizeHandle h, canvasframe::WorldPt world,
                      document::SmartTransform t, document::SmartBounds b)
    {
        *this = ManipSession{};
        active = true;
        nodeId = std::move(id);
        handle = h;
        originWorld = world;
        currentWorld = world;
        originT = t;
        liveT = t;
        originB = b;
        liveB = b;
        ManipResult r;
        r.intent = ManipIntent::BeginGesture | ManipIntent::RefreshAllConnectors
            | ManipIntent::CaptureOriginPunches | ManipIntent::RefreshChrome | ManipIntent::Redraw;
        return r;
    }

    /**
     * @p inkScaleMode — from the live node (withBounds / fixedInk).
     * @p previewDue — canvas ghost-clock said a preview/redraw is due.
     */
    ManipResult apply(canvasframe::WorldPt world, const std::string &inkScaleMode, bool previewDue)
    {
        ManipResult r;
        if (!active)
            return r;
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
        r.intent = ManipIntent::ApplyLiveGeometry | ManipIntent::RefreshBoundConnectors
            | ManipIntent::PreviewFrame;
        if (previewDue)
            r.intent |= ManipIntent::SendPreview | ManipIntent::Redraw;
        return r;
    }

    /** Decide commit vs abort; leaves fields readable until canvas applies then reset(). */
    ManipResult commit()
    {
        ManipResult r;
        if (!active)
            return r;
        const double dx = liveT.x - originT.x;
        const double dy = liveT.y - originT.y;
        r.resized = handle != document::ResizeHandle::None;
        r.moved = r.resized || std::hypot(dx, dy) >= 2.0
            || std::hypot(liveT.scaleX - originT.scaleX, liveT.scaleY - originT.scaleY) > 0.01
            || std::abs(liveB.width - originB.width) > 0.5;
        if (!r.moved) {
            r.intent = ManipIntent::ApplyLiveGeometry | ManipIntent::AbortGesture
                | ManipIntent::UpdatePunch | ManipIntent::RefreshChrome | ManipIntent::NotifyHistory;
            // Restore origin geometry on apply — canvas uses originT/B before reset.
        } else {
            r.intent = ManipIntent::CommitTransform | ManipIntent::RefreshAllConnectors
                | ManipIntent::ScheduleRasterize | ManipIntent::RefreshChrome
                | ManipIntent::NotifyHistory | ManipIntent::FlushWire;
        }
        return r;
    }

    ManipResult abort()
    {
        ManipResult r;
        if (!active)
            return r;
        r.intent = ManipIntent::ApplyLiveGeometry | ManipIntent::RefreshBoundConnectors
            | ManipIntent::AbortGesture | ManipIntent::RefreshAllConnectors
            | ManipIntent::UpdatePunch | ManipIntent::RefreshChrome;
        return r;
    }
};

inline document::ResizeHandle handleFromIndex(int index)
{
    using document::ResizeHandle;
    static const ResizeHandle k[] = {ResizeHandle::Nw, ResizeHandle::N, ResizeHandle::Ne,
                                     ResizeHandle::E,  ResizeHandle::Se, ResizeHandle::S,
                                     ResizeHandle::Sw, ResizeHandle::W};
    if (index < 0 || index >= 8)
        return ResizeHandle::None;
    return k[index];
}

} // namespace manip
} // namespace epaper
