#pragma once

/**
 * InputHub — Interaction Router demux (ADR-0033).
 * Phase 3: HandTouch match → allow-list → lock Operation → feed receive sink.
 * @implements [SRS-EP-04] @implements [SRS-EP-21]
 */

#include "hand_touch_modifier.hpp"
#include "host_caps.hpp"
#include "mode.hpp"
#include "operation.hpp"
#include "strategy.hpp"

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace epaper {
namespace tools {

struct FingerDownContext {
    PointerSample sample;
    bool knobHit = false;
    bool boxHit = false;
};

struct PinchContext {
    QPointF contactA;
    QPointF contactB;
    QPointF centroid;
    qreal scale = 1.0;
};

using OperationFactory = std::function<std::unique_ptr<Operation>(HostCaps &)>;

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
    void clearLock() { m_lockedOp = nullptr; }

    void registerFactory(OperationKind kind, OperationFactory factory)
    {
        m_factories[static_cast<int>(kind)] = std::move(factory);
    }

    /** Cached finger Operations — reused across gestures (no per-down allocation). */
    void setFingerOperation(OperationKind kind, std::unique_ptr<Operation> op)
    {
        m_fingerOps[static_cast<int>(kind)] = std::move(op);
    }

    void clearFingerOperations() { m_fingerOps.clear(); }

    void clearFactories() { m_factories.clear(); }

    void registerHitRegion(const HitRegion &r) { m_hits.push_back(r); }
    void clearHitRegions() { m_hits.clear(); }

    StrategyKind classifyPointer(bool pen) const
    {
        (void)pen;
        return StrategyKind::RawPointer;
    }

    /** HandTouch one-finger down: match among profile allow-list, lock, onDown. */
    bool dispatchFingerDown(const FingerDownContext &ctx);

    /** Feed locked RawPointer sink; returns false if nothing locked. */
    bool dispatchFingerMove(const PointerSample &s, int fingerCount);

    /** onUp + postHandling + clear lock. */
    bool dispatchFingerUp(const PointerSample &s, const HandTouchCommitInfo &commit);

    /** Cancel locked finger Operation. */
    void dispatchFingerCancel();

    /** Stationary tap → Tap strategy match (SelectOperation). */
    bool dispatchFingerTap(const PointerSample &s, const HandTouchCommitInfo &commit);

    /** Two-finger begin → lock NavigationOperation, beginTwoFinger. */
    bool dispatchPinchBegin(const PinchContext &ctx);

    bool dispatchPinchUpdate(const PinchContext &ctx);
    void dispatchPinchEnd(const HandTouchCommitInfo &commit);

    void runPostHandling(const HandTouchCommitInfo &info)
    {
        if (m_activeMode)
            m_hand.runPostHandling(m_activeMode->id(), m_caps, info);
    }

private:
    const HandTouchProfile *activeProfile() const;
    std::vector<OperationKind> allowedKinds() const;

    bool kindAllowed(OperationKind kind) const;
    bool shouldTryKind(OperationKind kind, const FingerDownContext *ctx) const;
    Operation *matchOperation(StrategyKind channel, const PointerSample &s,
                              const FingerDownContext *fingerCtx);

    void applyHitContext(Operation *op, const FingerDownContext &ctx);
    void feedRawDown(Operation *op, const PointerSample &s);
    void feedRawMove(Operation *op, const PointerSample &s, int fingerCount);
    void feedRawUp(Operation *op, const PointerSample &s);
    void feedRawCancel(Operation *op);

    HostCaps m_caps;
    HandTouchModifier m_hand;
    InteractionMode *m_activeMode = nullptr;
    Operation *m_lockedOp = nullptr;
    std::unordered_map<int, OperationFactory> m_factories;
    std::unordered_map<int, std::unique_ptr<Operation>> m_fingerOps;
    std::vector<HitRegion> m_hits;
};

} // namespace tools
} // namespace epaper
