#include "input_hub.hpp"
#include "contexts/selection_context.hpp"

#include <climits>

namespace epaper {
namespace tools {

void inputHubLinkAnchor() {}

bool InputHub::stampRole(PointerSample *s) const
{
    return s && m_devices.tryRole(s->device, &s->role);
}

bool InputHub::kindAllowed(OperationKind kind, PointerRole role) const
{
    if (!m_activeMode)
        return false;
    const std::vector<OperationKind> &list =
        role == PointerRole::Primary ? m_activeMode->primaryOps() : m_activeMode->secondaryOps();
    for (OperationKind k : list) {
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

StylusHoverSink *InputHub::matchHoverSink(const PointerSample &s) const
{
    if (!m_activeMode || m_lockedOp)
        return nullptr;
    StylusHoverSink *best = nullptr;
    int bestPriority = INT_MIN;
    for (OperationKind kind : m_activeMode->primaryOps()) {
        Operation *op = opFor(kind);
        auto *hover = dynamic_cast<StylusHoverSink *>(op);
        if (!hover)
            continue;
        if (!op->match(StrategyKind::StylusHover, s))
            continue;
        if (op->descriptor().priority > bestPriority) {
            bestPriority = op->descriptor().priority;
            best = hover;
        }
    }
    return best;
}

void InputHub::endHover()
{
    if (!m_hoverSink)
        return;
    m_hoverSink->onHoverLeave();
    m_hoverSink = nullptr;
}

void InputHub::dispatchHoverMove(const PointerSample &in)
{
    PointerSample s = in;
    s.role = PointerRole::Primary;
    StylusHoverSink *next = matchHoverSink(s);
    if (m_hoverSink && m_hoverSink != next)
        endHover();
    if (!next)
        return;
    if (m_hoverSink != next) {
        m_hoverSink = next;
        m_hoverSink->onHoverEnter(s);
        return;
    }
    m_hoverSink->onHoverMove(s);
}

void InputHub::dispatchHoverLeave()
{
    endHover();
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
    if (s.role == PointerRole::Secondary && !m_secondary.armed())
        return nullptr;
    if (channel == StrategyKind::HitTarget && !overlayHitAt(s.panel))
        return nullptr;

    Operation *best = nullptr;
    int bestPriority = INT_MIN;

    const std::vector<OperationKind> &list =
        s.role == PointerRole::Primary ? m_activeMode->primaryOps() : m_activeMode->secondaryOps();

    for (OperationKind kind : list) {
        if (!kindAllowed(kind, s.role))
            continue;
        Operation *op = opFor(kind);
        if (!op)
            continue;
        const OperationDescriptor &d = op->descriptor();
        if (s.role == PointerRole::Primary && !d.acceptPrimary)
            continue;
        if (s.role == PointerRole::Secondary && !d.acceptSecondary)
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

void InputHub::runSecondaryCommit()
{
    if (!m_activeMode)
        return;
    SecondaryCommitInfo info;
    if (m_caps.selection)
        info.selectionNonEmpty = !m_caps.selection->ids().empty();
    if (m_lockedOp)
        info.didMutateSelection = m_lockedOp->didMutateSelection();
    m_activeMode->onSecondaryCommit(m_caps, info);
}

bool InputHub::dispatchPointerDown(const PointerSample &in)
{
    PointerSample s = in;
    if (!stampRole(&s))
        return false;
    if (s.role == PointerRole::Secondary) {
        if (m_secondary.lockedUntilLift() || !m_secondary.armed())
            return false;
    }
    endHover();
    if (m_lockedOp) {
        feedRawDown(m_lockedOp, s);
        return true;
    }

    Operation *winner = matchOperation(StrategyKind::HitTarget, s);
    if (!winner)
        winner = matchOperation(StrategyKind::RawPointer, s);
    if (!winner)
        return false;
    m_lockedOp = winner;
    feedRawDown(m_lockedOp, s);
    return true;
}

bool InputHub::dispatchPointerMove(const PointerSample &in)
{
    if (!m_lockedOp)
        return false;
    PointerSample s = in;
    stampRole(&s);
    feedRawMove(m_lockedOp, s);
    return true;
}

bool InputHub::dispatchPointerUp(const PointerSample &in)
{
    if (!m_lockedOp)
        return false;
    PointerSample s = in;
    stampRole(&s);
    feedRawUp(m_lockedOp, s);
    if (s.role == PointerRole::Secondary)
        runSecondaryCommit();
    m_lockedOp = nullptr;
    return true;
}

void InputHub::dispatchPointerCancel()
{
    cancelAll();
}

void InputHub::cancelAll()
{
    endHover();
    if (!m_lockedOp)
        return;
    feedRawCancel(m_lockedOp);
    m_lockedOp = nullptr;
}

bool InputHub::dispatchTap(const PointerSample &in)
{
    PointerSample s = in;
    if (!stampRole(&s))
        return false;
    if (s.role == PointerRole::Secondary) {
        if (m_secondary.lockedUntilLift() || !m_secondary.armed())
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
    if (s.role == PointerRole::Secondary)
        runSecondaryCommit();
    m_lockedOp = nullptr;
    return true;
}

bool InputHub::dispatchPinchBegin(qreal x, qreal y, qreal scale)
{
    if (!m_secondary.armed())
        return false;
    endHover();
    if (m_lockedOp)
        cancelAll();

    Operation *nav = opFor(OperationKind::Navigation);
    auto *pinch = dynamic_cast<PinchSink *>(nav);
    if (!nav || !pinch)
        return false;
    PointerSample s;
    s.panel = QPointF(x, y);
    s.device = PointerDevice::Finger;
    stampRole(&s);
    if (!nav->match(StrategyKind::Pinch, s))
        return false;
    m_lockedOp = nav;
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
    PointerSample s;
    s.device = PointerDevice::Finger;
    if (stampRole(&s) && s.role == PointerRole::Secondary)
        runSecondaryCommit();
    m_secondary.setLockedUntilLift(true);
    m_lockedOp = nullptr;
}

} // namespace tools
} // namespace epaper
