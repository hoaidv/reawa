#pragma once

/**
 * NavigationOperation — one-finger empty pan + two-finger pan/pinch (HandTouch).
 * @implements [SRS-EP-21] @implements [SRS-EP-24]
 */

#include "../finger_host.hpp"
#include "../operation.hpp"

namespace epaper {
namespace tools {

class NavigationOperation final : public Operation, public RawPointerSink, public PinchSink {
public:
    explicit NavigationOperation(FingerHost host)
        : m_host(std::move(host))
    {
        m_desc.kind = OperationKind::Navigation;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 30;
        m_desc.acceptPen = false;
        m_desc.acceptFinger = true;
        m_pinchDesc = m_desc;
        m_pinchDesc.matchOn = StrategyKind::Pinch;
        m_pinchDesc.receive = StrategyKind::Pinch;
    }

    OperationKind kind() const override { return OperationKind::Navigation; }
    const OperationDescriptor &descriptor() const override
    {
        return m_pinchActive ? m_pinchDesc : m_desc;
    }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        (void)s;
        if (channel == StrategyKind::Pinch)
            return true;
        return channel == StrategyKind::RawPointer && !m_knobHit && !m_boxHit;
    }

    void setHitContext(bool knobHit, bool boxHit)
    {
        m_knobHit = knobHit;
        m_boxHit = boxHit;
    }

    void onDown(const PointerSample &s) override
    {
        if (!m_host.machine || !m_host.applyIntent)
            return;
        m_pinchActive = false;
        if (m_host.ensureLocalDrawingRegion)
            m_host.ensureLocalDrawingRegion();
        const auto r = m_host.machine->begin(s.panel.x(), s.panel.y(), false, false,
                                             m_host.drawingRegion ? m_host.drawingRegion()
                                                                  : epaper::canvasframe::WorldAabb{},
                                             m_host.panelToWorld ? m_host.panelToWorld(s.panel)
                                                                 : epaper::canvasframe::WorldPt{});
        m_host.applyIntent(r, s.panel);
    }

    void onMove(const PointerSample &s) override
    {
        if (!m_host.machine || !m_host.applyIntent || m_pinchActive)
            return;
        if (m_host.machine->isLiveManip())
            return;
        double nowX = 0;
        double nowY = 0;
        if (m_host.worldThroughPanOrigin)
            m_host.worldThroughPanOrigin(s.panel, &nowX, &nowY);
        using namespace epaper::fingergesture;
        const Kind before = m_host.machine->gesture;
        bool previewDue = m_host.previewDue ? m_host.previewDue() : true;
        if (before == Kind::EmptyPending)
            previewDue = true;
        const FingerResult r = m_host.machine->update(
            s.panel.x(), s.panel.y(), 1,
            m_host.follow ? m_host.follow() : epaper::handtouch::FollowDirection::None,
            previewDue, nowX, nowY);
        if (before == Kind::EmptyPending && m_host.machine->gesture == Kind::EmptyPan) {
            if (m_host.invalidatePanClock)
                m_host.invalidatePanClock();
        }
        if (has(r.intent, FingerIntent::PublishViewportLive) && m_host.restartPanClock)
            m_host.restartPanClock();
        m_host.applyIntent(r, s.panel);
    }

    void onUp(const PointerSample &s) override
    {
        if (m_pinchActive) {
            onPinchEnd();
            return;
        }
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
        m_pinchActive = false;
    }

    void onPinchBegin(const QPointF &centroid, qreal scale) override
    {
        (void)centroid;
        (void)scale;
        m_pinchActive = true;
    }

    void onPinchUpdate(const QPointF &centroid, qreal scale) override
    {
        (void)centroid;
        (void)scale;
    }

    void onPinchEnd() override { m_pinchActive = false; }

    /** Two-finger begin with panel contacts (called from hub, not PinchSink centroid). */
    bool beginTwoFinger(const QPointF &a, const QPointF &b)
    {
        if (!m_host.machine || !m_host.applyIntentBare)
            return false;
        m_pinchActive = true;
        if (m_host.ensureLocalDrawingRegion)
            m_host.ensureLocalDrawingRegion();
        const auto r = m_host.machine->beginTwo(
            a.x(), a.y(), b.x(), b.y(),
            m_host.drawingRegion ? m_host.drawingRegion() : epaper::canvasframe::WorldAabb{},
            m_host.uvPair ? m_host.uvPair(a, b) : epaper::handtouch::TwoFingerContacts{},
            m_host.follow ? m_host.follow() : epaper::handtouch::FollowDirection::None);
        if (!r.accepted) {
            m_pinchActive = false;
            return false;
        }
        if (m_host.invalidatePanClock)
            m_host.invalidatePanClock();
        m_host.applyIntentBare(r);
        return true;
    }

    void updateTwoFinger(const QPointF &a, const QPointF &b)
    {
        if (!m_host.machine || !m_host.applyIntentBare || !m_host.machine->isTwoFinger())
            return;
        const bool previewDue = m_host.previewDue ? m_host.previewDue() : true;
        const auto r = m_host.machine->updateTwo(
            a.x(), a.y(), b.x(), b.y(),
            m_host.uvPair ? m_host.uvPair(a, b) : epaper::handtouch::TwoFingerContacts{},
            previewDue);
        if (previewDue && epaper::fingergesture::has(r.intent,
                                                     epaper::fingergesture::FingerIntent::PublishViewportLive)
            && m_host.restartPanClock)
            m_host.restartPanClock();
        m_host.applyIntentBare(r);
    }

    void endTwoFinger()
    {
        if (!m_host.machine || !m_host.applyIntentBare)
            return;
        m_host.applyIntentBare(m_host.machine->endTwo());
        m_pinchActive = false;
    }

private:
    FingerHost m_host;
    OperationDescriptor m_desc;
    OperationDescriptor m_pinchDesc;
    bool m_knobHit = false;
    bool m_boxHit = false;
    bool m_pinchActive = false;
};

} // namespace tools
} // namespace epaper
