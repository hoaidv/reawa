#pragma once

/**
 * InputHub — Interaction Router demux (Phase 0 skeleton).
 * Qt entry stays on ToolCanvasItem; hub will own match/lock/feed in later phases.
 * @implements [SRS-EP-04]
 */

#include "hand_touch_modifier.hpp"
#include "host_caps.hpp"
#include "mode.hpp"
#include "operation.hpp"
#include "strategy.hpp"

#include <memory>
#include <vector>

namespace epaper {
namespace tools {

class InputHub {
public:
    void setHostCaps(HostCaps caps) { m_caps = caps; }
    HostCaps &hostCaps() { return m_caps; }
    const HostCaps &hostCaps() const { return m_caps; }

    HandTouchModifier &handTouch() { return m_hand; }
    const HandTouchModifier &handTouch() const { return m_hand; }

    void setActiveMode(InteractionMode *mode) { m_activeMode = mode; }
    InteractionMode *activeMode() const { return m_activeMode; }

    Operation *lockedOperation() const { return m_locked.get(); }
    void clearLock() { m_locked.reset(); }

    void registerHitRegion(const HitRegion &r) { m_hits.push_back(r); }
    void clearHitRegions() { m_hits.clear(); }

    /**
     * Phase 0: classify only — no match/lock yet (ToolCanvasItem still routes).
     * Later phases: policy → match → lock → feed receive sink.
     */
    StrategyKind classifyPointer(bool pen) const
    {
        (void)pen;
        return StrategyKind::RawPointer;
    }

private:
    HostCaps m_caps;
    HandTouchModifier m_hand;
    InteractionMode *m_activeMode = nullptr;
    std::unique_ptr<Operation> m_locked;
    std::vector<HitRegion> m_hits;
};

} // namespace tools
} // namespace epaper
