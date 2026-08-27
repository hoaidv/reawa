#pragma once

/**
 * MoveOperation — pick-move; owns TransformGesture.
 * @implements [SRS-EP-11] @implements [SRS-EP-21]
 */

#include "../host_caps.hpp"
#include "../operation.hpp"
#include "transform_gesture.hpp"
#include "document/capability.hpp"
#include "document/manipulate.hpp"

namespace epaper {
namespace tools {

class MoveOperation final : public Operation, public RawPointerSink {
public:
    explicit MoveOperation(HostCaps *caps)
        : m_caps(caps)
    {
        m_desc.kind = OperationKind::Move;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 50;
        m_desc.acceptPrimary = true;
        m_desc.acceptSecondary = true;
    }

    OperationKind kind() const override { return OperationKind::Move; }
    const OperationDescriptor &descriptor() const override { return m_desc; }
    bool didMutateSelection() const override { return m_gesture.didMutateSelection(); }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        if (channel != StrategyKind::RawPointer || !m_caps || !m_caps->doc || !m_caps->toolUi)
            return false;
        const auto w = m_caps->toolUi->panelToWorld(s.panel);
        return m_caps->doc->hitMoveTarget(w.x, w.y) != nullptr;
    }

    void onDown(const PointerSample &s) override
    {
        if (!m_caps || !m_caps->doc || !m_caps->toolUi || !m_caps->selection)
            return;
        m_gesture.resetMutate();
        m_caps->toolUi->clearManipUnavailable();
        const auto w = m_caps->toolUi->panelToWorld(s.panel);
        const epaper::document::DocNode *hit = m_caps->doc->hitMoveTarget(w.x, w.y);
        if (!hit)
            return;

        epaper::document::CapabilityDescriptor cap = epaper::document::descriptorFor(hit->kind);
        bool lodOk = true;
        epaper::document::SmartBounds wb;
        if (epaper::document::boundsOf(*hit, wb))
            lodOk = m_caps->toolUi->lodOkPanel(wb);
        const epaper::document::GestureKind kind =
            epaper::document::resolvePress(cap, lodOk, false, false, true);
        if (kind == epaper::document::GestureKind::Unavailable) {
            m_caps->toolUi->showManipUnavailable(wb);
            return;
        }
        if (kind != epaper::document::GestureKind::SelectMove)
            return;
        m_gesture.begin(m_caps, hit, epaper::document::ResizeHandle::None, {w.x, w.y});
    }

    void onMove(const PointerSample &s) override
    {
        if (!m_caps || !m_caps->toolUi)
            return;
        const auto w = m_caps->toolUi->panelToWorld(s.panel);
        m_gesture.apply(m_caps, {w.x, w.y});
    }

    void onUp(const PointerSample &s) override
    {
        (void)s;
        m_gesture.commit(m_caps);
    }

    void onCancel() override { cancel(); }

    void cancel() override { m_gesture.abort(m_caps); }

private:
    HostCaps *m_caps = nullptr;
    TransformGesture m_gesture;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
