#pragma once

/**
 * AreaErase — dotted freeform; even-odd Ink clip + fully-inside remove.
 * @implements [SRS-EP-54] erase_area exclusive
 * @implements [SRS-EP-57] area erase
 */

#include "../host_caps.hpp"
#include "../operation.hpp"
#include "document/erase_area.hpp"
#include "debug/ui_stall.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <string>
#include <vector>

namespace epaper {
namespace tools {

class AreaEraseOperation final : public Operation, public RawPointerSink {
public:
    explicit AreaEraseOperation(HostCaps *caps)
        : m_caps(caps)
    {
        m_desc.kind = OperationKind::AreaErase;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 20;
        m_desc.acceptPrimary = true;
        m_desc.acceptSecondary = false;
    }

    OperationKind kind() const override { return OperationKind::AreaErase; }
    const OperationDescriptor &descriptor() const override { return m_desc; }
    bool paintsIdleOverlay() const override { return true; }
    bool wantsPenWaveform() const override
    {
        return m_caps && m_caps->toolUi
            && m_caps->toolUi->exclusiveTool() == QLatin1String("erase_area");
    }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        (void)s;
        if (channel != StrategyKind::RawPointer)
            return false;
        if (!m_caps || !m_caps->toolUi)
            return false;
        return m_caps->toolUi->exclusiveTool() == QLatin1String("erase_area");
    }

    void onDown(const PointerSample &s) override
    {
        m_pts.clear();
        m_pts.push_back(s.panel);
        if (m_caps && m_caps->toolUi) {
            m_caps->toolUi->syncOverlayPresence();
            m_caps->toolUi->setStrokeWaveform(true);
            m_caps->toolUi->damageChrome(QRectF(s.panel, s.panel).adjusted(-12, -12, 12, 12));
        }
    }
    void onMove(const PointerSample &s) override
    {
        if (!m_caps || !m_caps->toolUi || m_pts.empty())
            return;
        const QPointF prev = m_pts.back();
        m_pts.push_back(s.panel);
        m_caps->toolUi->damageChromeSegment(
            QRectF(prev, s.panel).normalized().adjusted(-8, -8, 8, 8));
    }
    void onUp(const PointerSample &) override { commit(); }
    void onCancel() override { cancel(); }
    void cancel() override
    {
        m_pts.clear();
        if (m_caps && m_caps->toolUi) {
            m_caps->toolUi->setStrokeWaveform(false);
            m_caps->toolUi->requestChromeRefresh();
        }
    }

    void paintOverlay(QPainter *painter) override
    {
        if (!painter || m_pts.empty())
            return;
        painter->save();
        QPen dotted(Qt::black);
        dotted.setWidthF(3.0);
        dotted.setStyle(Qt::DotLine);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(dotted);
        if (m_pts.size() == 1) {
            painter->drawEllipse(m_pts.front(), 2.0, 2.0);
        } else {
            QPainterPath path;
            path.moveTo(m_pts.front());
            for (size_t i = 1; i < m_pts.size(); ++i)
                path.lineTo(m_pts[i]);
            painter->drawPath(path);
        }
        painter->restore();
    }

private:
    void commit()
    {
        using namespace epaper::document;
        std::vector<QPointF> pts = m_pts;
        m_pts.clear();
        if (!m_caps || !m_caps->doc || !m_caps->toolUi)
            return;
        m_caps->toolUi->setStrokeWaveform(false);
        m_caps->toolUi->requestChromeRefresh();
        if (pts.size() < 2)
            return;
        std::vector<ErasePt> world;
        world.reserve(pts.size());
        for (const auto &p : pts) {
            const auto w = m_caps->toolUi->panelToWorld(p);
            world.push_back({w.x, w.y});
        }
        static int seq = 0;
        const std::string opId = std::string("erase-area-") + std::to_string(++seq);
        ApplyResult r;
        {
            epaper::UiStallSection stall("eraseAreaCommit");
            r = commitAreaErase(m_caps->doc->document(), opId, std::move(world));
        }
        if (r.applied && r.reason != "noop") {
            m_caps->doc->noteDocumentMutated();
            m_caps->doc->notifyHistory();
            m_caps->doc->flushWire();
        }
        m_caps->toolUi->requestChromeRefresh();
    }

    HostCaps *m_caps = nullptr;
    std::vector<QPointF> m_pts;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
