#pragma once

/**
 * InputHub — Interaction Router demux (ADR-0033).
 * Unified pointer/pinch dispatch; Operations own gesture bodies.
 * @implements [SRS-EP-04] @implements [SRS-EP-21]
 */

#include "hand_touch_modifier.hpp"
#include "host_caps.hpp"
#include "mode.hpp"
#include "operation.hpp"
#include "strategy.hpp"

#include <memory>
#include <unordered_map>
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

    Operation *lockedOperation() const { return m_lockedOp; }

    void setOperation(OperationKind kind, std::unique_ptr<Operation> op)
    {
        m_ops[static_cast<int>(kind)] = std::move(op);
    }

    void clearOperations() { m_ops.clear(); }

    void registerHitRegion(const HitRegion &r) { m_hits.push_back(r); }
    void clearHitRegions() { m_hits.clear(); }

    bool dispatchPointerDown(const PointerSample &s);
    bool dispatchPointerMove(const PointerSample &s);
    bool dispatchPointerUp(const PointerSample &s);
    void dispatchPointerCancel();
    void cancelAll();

    bool dispatchTap(const PointerSample &s);

    bool dispatchPinchBegin(qreal x, qreal y, qreal scale);
    bool dispatchPinchUpdate(qreal x, qreal y, qreal scale);
    void dispatchPinchEnd();

private:
    const HandTouchProfile *activeProfile() const;
    bool kindAllowed(OperationKind kind, PointerDevice device) const;
    Operation *opFor(OperationKind kind) const;
    Operation *matchOperation(StrategyKind channel, const PointerSample &s);

    void feedRawDown(Operation *op, const PointerSample &s);
    void feedRawMove(Operation *op, const PointerSample &s);
    void feedRawUp(Operation *op, const PointerSample &s);
    void feedRawCancel(Operation *op);
    void runCommitPostHandling();

    HostCaps m_caps;
    HandTouchModifier m_hand;
    InteractionMode *m_activeMode = nullptr;
    Operation *m_lockedOp = nullptr;
    std::unordered_map<int, std::unique_ptr<Operation>> m_ops;
    std::vector<HitRegion> m_hits;
};

} // namespace tools
} // namespace epaper
