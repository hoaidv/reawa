#include "input_hub.hpp"
#include "selection_context.hpp"

#include <climits>

namespace epaper {
namespace tools {

void inputHubLinkAnchor() {}

namespace {

constexpr OperationKind kMatchOrder[] = {
    OperationKind::Resize,
    OperationKind::Move,
    OperationKind::Lasso,
    OperationKind::Marquee,
    OperationKind::Navigation,
    OperationKind::Select,
    OperationKind::InkStroke,
    OperationKind::Rotate,
};

} // namespace

const HandTouchProfile *InputHub::activeProfile() const
{
    if (!m_activeMode)
        return nullptr;
    return m_hand.profileFor(m_activeMode->id());
}

bool InputHub::kindAllowed(OperationKind kind, PointerDevice device) const
{
    if (!m_activeMode)
        return false;
    if (device == PointerDevice::Pen) {
        for (OperationKind k : m_activeMode->penOperations()) {
            if (k == kind)
                return true;
        }
        return false;
    }
    const HandTouchProfile *p = activeProfile();
    if (!p)
        return false;
    for (OperationKind k : p->allowedOperations) {
        if (k == kind)
            return true;
    }
    return false;
}

Operation *InputHub::opFor(OperationKind kind) const
{
    const auto it = m_ops.find(static_cast<int>(kind));
    return it == m_ops.end() ? nullptr : it->second.get();
}

const HitRegion *InputHub::overlayHitAt(const QPointF &panel) const
{
    const HitRegion *best = nullptr;
    int bestPriority = INT_MIN;
    for (const HitRegion &r : m_hits) {
        if (!r.panelRect.contains(panel))
            continue;
        if (r.priority > bestPriority) {
            bestPriority = r.priority;
            best = &r;
        }
    }
    return best;
}

void InputHub::dispatchIntervention(InterventionGate gate)
{
    for (const Intervention &iv : m_interventions) {
        if (iv.gate != gate)
            continue;
        if (iv.match && !iv.match())
            continue;
        if (iv.apply)
            iv.apply();
    }
}

Operation *InputHub::matchOperation(StrategyKind channel, const PointerSample &s)
{
    if (!m_activeMode)
        return nullptr;
    if (s.device == PointerDevice::Finger && !m_hand.armed())
        return nullptr;
    if (channel == StrategyKind::HitTarget && !overlayHitAt(s.panel))
        return nullptr;

    Operation *best = nullptr;
    int bestPriority = INT_MIN;

    for (OperationKind kind : kMatchOrder) {
        if (!kindAllowed(kind, s.device))
            continue;
        Operation *op = opFor(kind);
        if (!op)
            continue;
        const OperationDescriptor &d = op->descriptor();
        if (s.device == PointerDevice::Pen && !d.acceptPen)
            continue;
        if (s.device == PointerDevice::Finger && !d.acceptFinger)
            continue;
        if (!op->match(channel, s))
            continue;
        if (d.priority > bestPriority) {
            bestPriority = d.priority;
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

void InputHub::feedRawMove(Operation *op, const PointerSample &s)
{
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
    else
        op->cancel();
}

void InputHub::runCommitPostHandling()
{
    HandTouchCommitInfo info;
    if (m_caps.selection)
        info.selectionNonEmpty = !m_caps.selection->ids().empty();
    if (m_lockedOp)
        info.didMutateSelection = m_lockedOp->didMutateSelection();
    if (m_activeMode)
        m_hand.runPostHandling(m_activeMode->id(), m_caps, info);
}

bool InputHub::dispatchPointerDown(const PointerSample &s)
{
    if (s.device == PointerDevice::Finger) {
        if (m_hand.lockedUntilLift())
            return false;
        if (!m_hand.armed())
            return false;
    }
    if (m_lockedOp) {
        feedRawDown(m_lockedOp, s);
        return true;
    }

    Operation *winner = nullptr;
    if (s.device == PointerDevice::Pen) {
        winner = matchOperation(StrategyKind::HitTarget, s);
        if (!winner)
            winner = matchOperation(StrategyKind::RawPointer, s);
    } else {
        winner = matchOperation(StrategyKind::HitTarget, s);
        if (!winner)
            winner = matchOperation(StrategyKind::RawPointer, s);
    }
    if (!winner)
        return false;
    m_lockedOp = winner;
    feedRawDown(m_lockedOp, s);
    return true;
}

bool InputHub::dispatchPointerMove(const PointerSample &s)
{
    if (!m_lockedOp)
        return false;
    feedRawMove(m_lockedOp, s);
    return true;
}

bool InputHub::dispatchPointerUp(const PointerSample &s)
{
    if (!m_lockedOp)
        return false;
    feedRawUp(m_lockedOp, s);
    if (s.device == PointerDevice::Finger)
        runCommitPostHandling();
    m_lockedOp = nullptr;
    return true;
}

void InputHub::dispatchPointerCancel()
{
    cancelAll();
}

void InputHub::cancelAll()
{
    if (!m_lockedOp)
        return;
    feedRawCancel(m_lockedOp);
    m_lockedOp = nullptr;
}

bool InputHub::dispatchTap(const PointerSample &s)
{
    if (s.device == PointerDevice::Finger) {
        if (m_hand.lockedUntilLift() || !m_hand.armed())
            return false;
    }
    if (m_lockedOp)
        return false;
    Operation *winner = matchOperation(StrategyKind::Tap, s);
    if (!winner)
        return false;
    m_lockedOp = winner;
    if (auto *tap = dynamic_cast<TapSink *>(winner))
        tap->onTap(s);
    if (s.device == PointerDevice::Finger)
        runCommitPostHandling();
    m_lockedOp = nullptr;
    return true;
}

bool InputHub::dispatchPinchBegin(qreal x, qreal y, qreal scale)
{
    if (!m_hand.armed())
        return false;
    if (m_lockedOp)
        cancelAll();

    PointerSample s;
    s.panel = QPointF(x, y);
    s.device = PointerDevice::Finger;
    Operation *winner = matchOperation(StrategyKind::Pinch, s);
    auto *pinch = dynamic_cast<PinchSink *>(winner);
    if (!pinch)
        return false;
    m_lockedOp = winner;
    pinch->onPinchBegin(QPointF(x, y), scale);
    return true;
}

bool InputHub::dispatchPinchUpdate(qreal x, qreal y, qreal scale)
{
    if (!m_lockedOp)
        return false;
    auto *pinch = dynamic_cast<PinchSink *>(m_lockedOp);
    if (!pinch)
        return false;
    pinch->onPinchUpdate(QPointF(x, y), scale);
    return true;
}

void InputHub::dispatchPinchEnd()
{
    if (!m_lockedOp)
        return;
    if (auto *pinch = dynamic_cast<PinchSink *>(m_lockedOp))
        pinch->onPinchEnd();
    runCommitPostHandling();
    m_hand.setLockedUntilLift(true);
    m_lockedOp = nullptr;
}

} // namespace tools
} // namespace epaper
