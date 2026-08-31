#pragma once

/**
 * MarqueeOperation — owns rect corners; containment via DocContext document.
 * @implements [SRS-EP-11] @implements [SRS-EP-12]
 */

#include "../host_caps.hpp"
#include "../operation.hpp"
#include "../contexts/doc_context.hpp"
#include "../contexts/tool_context.hpp"
#include "../ui/selection_overlay.hpp"
#include "document/surround_create.hpp"

#include <QLatin1String>

#include <QPainter>
#include <QPen>
#include <QPointF>
#include <algorithm>
#include <cmath>

namespace epaper {
namespace tools {

class MarqueeOperation final : public Operation, public RawPointerSink {
public:
    static constexpr double kMinGesture = 8.0;

    explicit MarqueeOperation(HostCaps *caps)
        : m_caps(caps)
    {
        m_desc.kind = OperationKind::Marquee;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 40;
        m_desc.acceptPrimary = true;
        m_desc.acceptSecondary = true;
    }

    OperationKind kind() const override { return OperationKind::Marquee; }
    const OperationDescriptor &descriptor() const override { return m_desc; }
    bool didMutateSelection() const override { return m_didMutate; }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        (void)s;
        if (channel != StrategyKind::RawPointer)
            return false;
        if (!m_caps || !m_caps->doc)
            return false;
        return m_caps->doc->exclusiveTool() == QLatin1String("sel_rect");
    }

    void onDown(const PointerSample &s) override
    {
        if (!m_caps || !m_caps->toolUi || !m_caps->selection)
            return;
        m_didMutate = false;
        m_start = s.panel;
        m_end = s.panel;
        m_live = true;
        m_caps->selection->setPhase(SelectionPhase::Selecting);
        if (m_caps->overlay)
            m_caps->overlay->resetTransientFlags();
        if (m_caps->emitChromeChanged)
            m_caps->emitChromeChanged();
        m_caps->toolUi->syncOverlayPresence();
        m_caps->toolUi->setStrokeWaveform(true);
        m_caps->toolUi->damageChrome(QRectF(s.panel, s.panel).adjusted(-8, -8, 8, 8));
    }

    void onMove(const PointerSample &s) override
    {
        if (!m_caps || !m_caps->toolUi || !m_live)
            return;
        m_end = s.panel;
        m_caps->toolUi->damageChrome(
            QRectF(m_start, m_end).normalized().adjusted(-8, -8, 8, 8));
    }

    void onUp(const PointerSample &s) override
    {
        (void)s;
        finish();
    }

    void onCancel() override { cancel(); }

    void cancel() override
    {
        m_live = false;
        if (!m_caps || !m_caps->toolUi || !m_caps->selection)
            return;
        m_caps->selection->setPhase(m_caps->selection->ids().empty() ? SelectionPhase::Idle
                                                                     : SelectionPhase::Selected);
        m_caps->toolUi->setStrokeWaveform(false);
        m_caps->toolUi->refreshChrome();
    }

    void paintOverlay(QPainter *painter) override
    {
        if (!painter || !m_live)
            return;
        painter->save();
        QPen dotted(Qt::black);
        dotted.setWidthF(3.0);
        dotted.setStyle(Qt::DotLine);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(dotted);
        painter->drawRect(QRectF(m_start, m_end).normalized());
        painter->restore();
    }

private:
    void finish()
    {
        if (!m_caps || !m_caps->doc || !m_caps->toolUi || !m_caps->selection)
            return;
        m_live = false;
        m_caps->toolUi->setStrokeWaveform(false);
        const double gestureSize = std::hypot(m_end.x() - m_start.x(), m_end.y() - m_start.y());
        if (gestureSize < kMinGesture) {
            m_caps->selection->clear();
            m_caps->selection->setPhase(SelectionPhase::Idle);
            if (m_caps->doc)
                m_caps->doc->setInteractionDebug("sel=0 (tap)");
            m_caps->toolUi->refreshChrome();
            return;
        }
        const auto a = m_caps->toolUi->panelToWorld(m_start);
        const auto b = m_caps->toolUi->panelToWorld(m_end);
        epaper::document::SmartBounds rect;
        rect.x = std::min(a.x, b.x);
        rect.y = std::min(a.y, b.y);
        rect.width = std::abs(a.x - b.x);
        rect.height = std::abs(a.y - b.y);
        const auto hit = epaper::document::selectByRect(m_caps->doc->document(), rect);
        m_caps->selection->setIds(hit);
        m_caps->selection->setPhase(hit.empty() ? SelectionPhase::Idle : SelectionPhase::Selected);
        m_didMutate = true;
        std::string debug = hit.empty() ? "sel=0 (no nodes ≥80% inside)"
                                        : ("sel=" + std::to_string(hit.size()));
        for (const auto &id : hit)
            debug += " " + id;
        if (m_caps->doc)
            m_caps->doc->setInteractionDebug(debug);
        m_caps->toolUi->refreshChrome();
    }

    HostCaps *m_caps = nullptr;
    QPointF m_start;
    QPointF m_end;
    bool m_live = false;
    bool m_didMutate = false;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
