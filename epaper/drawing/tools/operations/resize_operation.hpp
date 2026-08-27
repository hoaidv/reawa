#pragma once

/**
 * ResizeOperation — knob HitTarget match, RawPointer drag (pen + finger).
 * @implements [SRS-EP-11] @implements [SRS-EP-12]
 */

#include "../manip_host.hpp"
#include "../operation.hpp"

namespace epaper {
namespace tools {

class ResizeOperation final : public Operation, public RawPointerSink {
public:
    explicit ResizeOperation(ManipHost host)
        : m_host(std::move(host))
    {
        m_desc.kind = OperationKind::Resize;
        m_desc.matchOn = StrategyKind::HitTarget;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 60;
        m_desc.acceptPen = true;
        m_desc.acceptFinger = true;
    }

    OperationKind kind() const override { return OperationKind::Resize; }
    const OperationDescriptor &descriptor() const override { return m_desc; }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        if (!m_host.handleIndexAtPanel)
            return false;
        const double hitDu = s.device == PointerDevice::Pen ? m_host.penHandleHitDu
                                                            : m_host.fingerHandleHitDu;
        const int idx = m_host.handleIndexAtPanel(s.panel, hitDu);
        if (idx < 0)
            return false;
        if (channel == StrategyKind::HitTarget)
            return true;
        return channel == StrategyKind::RawPointer && m_knobHit;
    }

    void setHitContext(bool knobHit) { m_knobHit = knobHit; }

    void onDown(const PointerSample &s) override
    {
        if (!m_host.handleIndexAtPanel || !m_host.beginHandleDrag)
            return;
        const double hitDu = s.device == PointerDevice::Pen ? m_host.penHandleHitDu
                                                            : m_host.fingerHandleHitDu;
        const int idx = m_host.handleIndexAtPanel(s.panel, hitDu);
        if (idx < 0)
            return;
        m_host.beginHandleDrag(idx, s.panel);
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
    bool m_knobHit = false;
};

} // namespace tools
} // namespace epaper
