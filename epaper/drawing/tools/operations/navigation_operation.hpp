#pragma once

/**
 * NavigationOperation — owns pan/pinch state + camera via Viewport.
 * @implements [SRS-EP-21] @implements [SRS-EP-24]
 */

#include "../host_caps.hpp"
#include "../operation.hpp"
#include "../viewport.hpp"
#include "document/hand_touch.hpp"

#include <QElapsedTimer>
#include <QPointF>

namespace epaper {
namespace tools {

class NavigationOperation final : public Operation, public RawPointerSink, public PinchSink {
public:
    static constexpr qint64 kGhostMinIntervalMs = 200;

    NavigationOperation(HostCaps *caps, Viewport *viewport)
        : m_caps(caps)
        , m_viewport(viewport)
    {
        m_desc.kind = OperationKind::Navigation;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 30;
        m_desc.acceptPrimary = false;
        m_desc.acceptSecondary = true;
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
        if (channel == StrategyKind::Pinch) {
            if (!m_viewport)
                return false;
            return !epaper::handtouch::onLocalNav(m_viewport->follow()).blocked;
        }
        return channel == StrategyKind::RawPointer;
    }

    void onDown(const PointerSample &s) override
    {
        if (!m_viewport)
            return;
        m_pinchActive = false;
        m_clearedOnTap = false;
        m_pan = PanKind::Pending;
        m_viewport->ensureDrawingRegion();
        m_downPanel = {s.panel.x(), s.panel.y()};
        m_panOrigin = m_viewport->drawingRegion();
        const auto w = m_caps && m_caps->toolUi ? m_caps->toolUi->panelToWorld(s.panel)
                                                : epaper::canvasframe::WorldPt{};
        m_downWorld = {w.x, w.y};
    }

    void onMove(const PointerSample &s) override
    {
        if (!m_viewport || m_pinchActive)
            return;
        if (m_pan == PanKind::None)
            return;
        using namespace epaper::handtouch;
        const double dx = s.panel.x() - m_downPanel.x;
        const double dy = s.panel.y() - m_downPanel.y;
        if (m_pan == PanKind::Pending) {
            if (actionOnEmptyMove(travelDu(dx, dy)) != FingerAction::LocalPan)
                return;
            if (onLocalNav(m_viewport->follow()).blocked)
                return;
            m_pan = PanKind::Active;
            m_previewClock.invalidate();
        }
        if (m_pan != PanKind::Active)
            return;
        double nowX = 0;
        double nowY = 0;
        m_viewport->worldThroughPanOrigin(m_panOrigin, s.panel.x(), s.panel.y(), &nowX, &nowY);
        publishNav(panKeepWorldUnderFinger(m_panOrigin.box(), m_downWorld.x, m_downWorld.y, nowX,
                                           nowY),
                   /*settle=*/false);
    }

    void onUp(const PointerSample &s) override
    {
        if (m_pinchActive) {
            onPinchEnd();
            return;
        }
        if (!m_viewport) {
            m_pan = PanKind::None;
            return;
        }
        if (m_pan == PanKind::Active) {
            double nowX = 0;
            double nowY = 0;
            m_viewport->worldThroughPanOrigin(m_panOrigin, s.panel.x(), s.panel.y(), &nowX, &nowY);
            publishNav(epaper::handtouch::panKeepWorldUnderFinger(
                           m_panOrigin.box(), m_downWorld.x, m_downWorld.y, nowX, nowY),
                       /*settle=*/true);
        } else if (m_pan == PanKind::Pending && m_caps && m_caps->selection && m_caps->toolUi) {
            const double dx = s.panel.x() - m_downPanel.x;
            const double dy = s.panel.y() - m_downPanel.y;
            if (epaper::handtouch::emptyTapClearsSelection(epaper::handtouch::travelDu(dx, dy))) {
                m_caps->selection->clear();
                m_caps->toolUi->requestChromeRefresh();
                m_clearedOnTap = true;
            }
        }
        m_pan = PanKind::None;
    }

    void onCancel() override { cancel(); }

    void cancel() override
    {
        m_pan = PanKind::None;
        if (m_pinchActive)
            onPinchEnd();
        m_pinchActive = false;
    }

    void onPinchBegin(const QPointF &centroid, qreal scale) override
    {
        if (!m_viewport)
            return;
        if (epaper::handtouch::onLocalNav(m_viewport->follow()).blocked)
            return;
        m_pan = PanKind::None;
        m_pinchActive = true;
        m_pinchArm = 80.0;
        m_pinchScale0 = scale > 0.01 ? scale : 1.0;
        m_viewport->ensureDrawingRegion();
        m_panOrigin = m_viewport->drawingRegion();
        const QPointF a = armPoint(centroid, scale, true);
        const QPointF b = armPoint(centroid, scale, false);
        m_twoOrigin = m_viewport->uvPair(a.x(), a.y(), b.x(), b.y());
        m_twoCurrent = m_twoOrigin;
        m_previewClock.invalidate();
        publishNav(epaper::handtouch::applyTwoFingerPanPinch(m_panOrigin.box(), m_twoOrigin,
                                                             m_twoCurrent),
                   /*settle=*/false);
    }

    void onPinchUpdate(const QPointF &centroid, qreal scale) override
    {
        if (!m_viewport || !m_pinchActive)
            return;
        const QPointF a = armPoint(centroid, scale, true);
        const QPointF b = armPoint(centroid, scale, false);
        m_twoCurrent = m_viewport->uvPair(a.x(), a.y(), b.x(), b.y());
        publishNav(epaper::handtouch::applyTwoFingerPanPinch(m_panOrigin.box(), m_twoOrigin,
                                                             m_twoCurrent),
                   /*settle=*/false);
    }

    void onPinchEnd() override
    {
        if (m_pinchActive && m_viewport) {
            publishNav(epaper::handtouch::applyTwoFingerPanPinch(m_panOrigin.box(), m_twoOrigin,
                                                                 m_twoCurrent),
                       /*settle=*/true);
        }
        m_pinchActive = false;
        m_pan = PanKind::None;
    }

    bool didMutateSelection() const override { return m_clearedOnTap; }

private:
    enum class PanKind { None, Pending, Active };

    QPointF armPoint(const QPointF &centroid, qreal scale, bool positive) const
    {
        const qreal s0 = m_pinchScale0 > 0.01 ? m_pinchScale0 : 1.0;
        const qreal arm = m_pinchArm * (scale / s0);
        return QPointF(centroid.x() + (positive ? arm : -arm), centroid.y());
    }

    void publishNav(const epaper::handtouch::WorldAabb &region, bool settle)
    {
        if (!m_viewport)
            return;
        const bool previewDue =
            settle || !m_previewClock.isValid() || m_previewClock.elapsed() >= kGhostMinIntervalMs;
        if (!settle && !previewDue)
            return;
        m_viewport->applyCamera(region, false);
        m_viewport->publishViewport(settle);
        m_viewport->scheduleRasterize(settle);
        if (m_caps && m_caps->toolUi)
            m_caps->toolUi->requestChromeRefresh();
        if (previewDue)
            m_previewClock.restart();
    }

    HostCaps *m_caps = nullptr;
    Viewport *m_viewport = nullptr;
    OperationDescriptor m_desc;
    OperationDescriptor m_pinchDesc;
    PanKind m_pan = PanKind::None;
    bool m_pinchActive = false;
    bool m_clearedOnTap = false;
    epaper::canvasframe::PanelPt m_downPanel{};
    epaper::canvasframe::WorldPt m_downWorld{};
    epaper::canvasframe::WorldAabb m_panOrigin{};
    epaper::handtouch::TwoFingerContacts m_twoOrigin{};
    epaper::handtouch::TwoFingerContacts m_twoCurrent{};
    QElapsedTimer m_previewClock;
    qreal m_pinchArm = 80.0;
    qreal m_pinchScale0 = 1.0;
};

} // namespace tools
} // namespace epaper
