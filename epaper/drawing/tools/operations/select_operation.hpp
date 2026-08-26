#pragma once

/**
 * SelectOperation — stationary finger tap (empty-tap deselect path).
 * @implements [SRS-EP-21] @implements [SRS-EP-23]
 */

#include "../finger_host.hpp"
#include "../operation.hpp"

namespace epaper {
namespace tools {

class SelectOperation final : public Operation, public TapSink {
public:
    explicit SelectOperation(FingerHost host)
        : m_host(std::move(host))
    {
        m_desc.kind = OperationKind::Select;
        m_desc.matchOn = StrategyKind::Tap;
        m_desc.receive = StrategyKind::Tap;
        m_desc.priority = 20;
        m_desc.acceptPen = false;
        m_desc.acceptFinger = true;
    }

    OperationKind kind() const override { return OperationKind::Select; }
    const OperationDescriptor &descriptor() const override { return m_desc; }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        (void)s;
        return channel == StrategyKind::Tap;
    }

    void onTap(const PointerSample &s) override
    {
        if (!m_host.machine || !m_host.applyIntent || !m_host.ensureLocalDrawingRegion)
            return;
        m_host.ensureLocalDrawingRegion();
        const auto down = m_host.machine->begin(s.panel.x(), s.panel.y(), false, false,
                                                m_host.drawingRegion ? m_host.drawingRegion()
                                                                     : epaper::canvasframe::WorldAabb{},
                                                m_host.panelToWorld ? m_host.panelToWorld(s.panel)
                                                                    : epaper::canvasframe::WorldPt{});
        if (!down.accepted)
            return;
        double nowX = 0;
        double nowY = 0;
        if (m_host.worldThroughPanOrigin)
            m_host.worldThroughPanOrigin(s.panel, &nowX, &nowY);
        m_host.applyIntent(m_host.machine->end(s.panel.x(), s.panel.y(), nowX, nowY), s.panel);
    }

    void cancel() override {}

private:
    FingerHost m_host;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
