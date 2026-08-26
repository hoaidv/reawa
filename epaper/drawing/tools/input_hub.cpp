#include "input_hub.hpp"

#include "operations/finger_resize_operation.hpp"
#include "operations/move_operation.hpp"
#include "operations/navigation_operation.hpp"
#include "operations/select_operation.hpp"

#include <climits>

namespace epaper {
namespace tools {

void inputHubLinkAnchor() {}

namespace {

/** Global priority order — highest first for early-exit when only one can match. */
constexpr OperationKind kMatchOrder[] = {
    OperationKind::Resize,
    OperationKind::Move,
    OperationKind::Navigation,
    OperationKind::Select,
    OperationKind::Lasso,
    OperationKind::Marquee,
    OperationKind::Rotate,
};

} // namespace

const HandTouchProfile *InputHub::activeProfile() const
{
    if (!m_activeMode)
        return nullptr;
    return m_hand.profileFor(m_activeMode->id());
}

std::vector<OperationKind> InputHub::allowedKinds() const
{
    const HandTouchProfile *p = activeProfile();
    if (!p)
        return {};
    return p->allowedOperations;
}

bool InputHub::kindAllowed(OperationKind kind) const
{
    for (OperationKind k : allowedKinds()) {
        if (k == kind)
            return true;
    }
    return false;
}

bool InputHub::shouldTryKind(OperationKind kind, const FingerDownContext *ctx) const
{
    if (!ctx)
        return true;
    switch (kind) {
    case OperationKind::Resize:
        return ctx->knobHit;
    case OperationKind::Move:
        return ctx->boxHit && !ctx->knobHit;
    case OperationKind::Navigation:
    case OperationKind::Select:
        return !ctx->knobHit && !ctx->boxHit;
    default:
        return true;
    }
}

void InputHub::applyHitContext(Operation *op, const FingerDownContext &ctx)
{
    if (auto *nav = dynamic_cast<NavigationOperation *>(op))
        nav->setHitContext(ctx.knobHit, ctx.boxHit);
    else if (auto *mv = dynamic_cast<MoveOperation *>(op))
        mv->setHitContext(ctx.boxHit);
    else if (auto *rs = dynamic_cast<FingerResizeOperation *>(op))
        rs->setHitContext(ctx.knobHit);
}

Operation *InputHub::matchOperation(StrategyKind channel, const PointerSample &s,
                                    const FingerDownContext *fingerCtx)
{
    if (!m_hand.armed() || !m_activeMode)
        return nullptr;

    Operation *best = nullptr;
    int bestPriority = INT_MIN;

    for (OperationKind kind : kMatchOrder) {
        if (!kindAllowed(kind) || !shouldTryKind(kind, fingerCtx))
            continue;
        const auto it = m_fingerOps.find(static_cast<int>(kind));
        if (it == m_fingerOps.end())
            continue;
        Operation *op = it->second.get();
        if (!op->descriptor().acceptFinger)
            continue;
        if (fingerCtx)
            applyHitContext(op, *fingerCtx);
        if (!op->match(channel, s))
            continue;
        const int pri = op->descriptor().priority;
        if (pri > bestPriority) {
            bestPriority = pri;
            best = op;
        }
    }

    return best;
}

void InputHub::feedRawDown(Operation *op, const PointerSample &s)
{
    if (auto *sink = dynamic_cast<RawPointerSink *>(op))
        sink->onDown(s);
}

void InputHub::feedRawMove(Operation *op, const PointerSample &s, int fingerCount)
{
    (void)fingerCount;
    if (auto *sink = dynamic_cast<RawPointerSink *>(op))
        sink->onMove(s);
}

void InputHub::feedRawUp(Operation *op, const PointerSample &s)
{
    if (auto *sink = dynamic_cast<RawPointerSink *>(op))
        sink->onUp(s);
}

void InputHub::feedRawCancel(Operation *op)
{
    if (auto *sink = dynamic_cast<RawPointerSink *>(op))
        sink->onCancel();
}

bool InputHub::dispatchFingerDown(const FingerDownContext &ctx)
{
    if (!m_hand.armed())
        return false;
    if (m_lockedOp) {
        feedRawDown(m_lockedOp, ctx.sample);
        return true;
    }

    Operation *winner = matchOperation(StrategyKind::RawPointer, ctx.sample, &ctx);
    if (!winner)
        return false;
    m_lockedOp = winner;
    feedRawDown(m_lockedOp, ctx.sample);
    return true;
}

bool InputHub::dispatchFingerMove(const PointerSample &s, int fingerCount)
{
    if (!m_lockedOp)
        return false;
    feedRawMove(m_lockedOp, s, fingerCount);
    return true;
}

bool InputHub::dispatchFingerUp(const PointerSample &s, const HandTouchCommitInfo &commit)
{
    if (!m_lockedOp)
        return false;
    feedRawUp(m_lockedOp, s);
    runPostHandling(commit);
    m_lockedOp = nullptr;
    return true;
}

void InputHub::dispatchFingerCancel()
{
    if (!m_lockedOp)
        return;
    feedRawCancel(m_lockedOp);
    m_lockedOp = nullptr;
}

bool InputHub::dispatchFingerTap(const PointerSample &s, const HandTouchCommitInfo &commit)
{
    (void)commit;
    (void)s;
    // TapHandler path: use the same down/up cycle as DragHandler so knob/box hit-test runs.
    // SelectOperation (Tap-only, box=false) cannot pick nodes — see toolcanvasitem onFingerTap.
    return false;
}

bool InputHub::dispatchPinchBegin(const PinchContext &ctx)
{
    if (!m_hand.armed())
        return false;
    m_lockedOp = nullptr;

    PointerSample s;
    s.panel = ctx.centroid;
    s.device = PointerDevice::Finger;

    FingerDownContext fingerCtx;
    fingerCtx.sample = s;
    Operation *winner = matchOperation(StrategyKind::Pinch, s, &fingerCtx);
    if (!winner)
        return false;

    auto *nav = dynamic_cast<NavigationOperation *>(winner);
    if (!nav)
        return false;

    m_lockedOp = winner;
    return nav->beginTwoFinger(ctx.contactA, ctx.contactB);
}

bool InputHub::dispatchPinchUpdate(const PinchContext &ctx)
{
    if (!m_lockedOp)
        return false;
    auto *nav = dynamic_cast<NavigationOperation *>(m_lockedOp);
    if (!nav)
        return false;
    nav->updateTwoFinger(ctx.contactA, ctx.contactB);
    return true;
}

void InputHub::dispatchPinchEnd(const HandTouchCommitInfo &commit)
{
    if (!m_lockedOp)
        return;
    if (auto *nav = dynamic_cast<NavigationOperation *>(m_lockedOp))
        nav->endTwoFinger();
    runPostHandling(commit);
    m_lockedOp = nullptr;
}

} // namespace tools
} // namespace epaper
