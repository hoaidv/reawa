#pragma once

/**
 * MoveOperation — pick-move via ManipSession (pen + finger HandTouch).
 * @implements [SRS-EP-11] @implements [SRS-EP-21]
 */

#include "../manip_host.hpp"
#include "../operation.hpp"

namespace epaper {
namespace tools {

class MoveOperation final : public Operation, public RawPointerSink {
public:
    explicit MoveOperation(ManipHost host)
        : m_host(std::move(host))
    {
        m_desc.kind = OperationKind::Move;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 50;
        m_desc.acceptPen = true;
        m_desc.acceptFinger = true;
    }

    OperationKind kind() const override { return OperationKind::Move; }
    const OperationDescriptor &descriptor() const override { return m_desc; }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        (void)s;
        return channel == StrategyKind::RawPointer && m_boxHit;
    }

    void setHitContext(bool boxHit) { m_boxHit = boxHit; }

    void onDown(const PointerSample &s) override
    {
        if (!m_host.beginMoveFromPanel)
            return;
        const bool arm = s.device == PointerDevice::Finger;
        m_host.beginMoveFromPanel(s.panel, arm);
    }

    void onMove(const PointerSample &s) override
    {
        if (m_host.applyDragFromPanel)
            m_host.applyDragFromPanel(s.panel);
    }

    void onUp(const PointerSample &s) override
    {
        (void)s;
        if (m_host.commitTransform)
            m_host.commitTransform();
    }

    void onCancel() override { cancel(); }

    void cancel() override
    {
        if (m_host.abortTransform)
            m_host.abortTransform();
    }

private:
    ManipHost m_host;
    OperationDescriptor m_desc;
    bool m_boxHit = false;
};

} // namespace tools
} // namespace epaper
