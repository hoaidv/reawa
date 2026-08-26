#pragma once

/**
 * MoveOperation — finger pick-move on selection box (HandTouch).
 * Full ManipSession extraction deferred to Phase 4; host callbacks preserve behavior.
 * @implements [SRS-EP-11] @implements [SRS-EP-21]
 */

#include "../finger_host.hpp"
#include "../operation.hpp"

namespace epaper {
namespace tools {

class MoveOperation final : public Operation, public RawPointerSink {
public:
    explicit MoveOperation(FingerHost host)
        : m_host(std::move(host))
    {
        m_desc.kind = OperationKind::Move;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 50;
        m_desc.acceptPen = false;
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
        if (!m_host.machine || !m_host.applyIntent)
            return;
        if (m_host.ensureLocalDrawingRegion)
            m_host.ensureLocalDrawingRegion();
        const auto r = m_host.machine->begin(s.panel.x(), s.panel.y(), false, true,
                                             m_host.drawingRegion ? m_host.drawingRegion()
                                                                  : epaper::canvasframe::WorldAabb{},
                                             m_host.panelToWorld ? m_host.panelToWorld(s.panel)
                                                                 : epaper::canvasframe::WorldPt{});
        m_host.applyIntent(r, s.panel);
    }

    void onMove(const PointerSample &s) override
    {
        if (m_host.updateSelectionGesture)
            m_host.updateSelectionGesture(s.panel);
    }

    void onUp(const PointerSample &s) override
    {
        if (!m_host.machine || !m_host.applyIntent)
            return;
        double nowX = 0;
        double nowY = 0;
        if (m_host.worldThroughPanOrigin)
            m_host.worldThroughPanOrigin(s.panel, &nowX, &nowY);
        m_host.applyIntent(m_host.machine->end(s.panel.x(), s.panel.y(), nowX, nowY), s.panel);
    }

    void onCancel() override { cancel(); }

    void cancel() override
    {
        if (!m_host.machine || !m_host.applyIntentBare)
            return;
        m_host.applyIntentBare(m_host.machine->cancel(m_host.manipActive && m_host.manipActive()));
    }

private:
    FingerHost m_host;
    OperationDescriptor m_desc;
    bool m_boxHit = false;
};

} // namespace tools
} // namespace epaper
