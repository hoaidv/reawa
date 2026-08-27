#pragma once

/**
 * LassoOperation — owns polyline; containment via DocContext document.
 * @implements [SRS-EP-11] @implements [SRS-EP-12]
 */

#include "../host_caps.hpp"
#include "../operation.hpp"
#include "document/surround_create.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <algorithm>
#include <cmath>
#include <vector>

namespace epaper {
namespace tools {

class LassoOperation final : public Operation, public RawPointerSink {
public:
    static constexpr double kMinGesture = 8.0;

    explicit LassoOperation(HostCaps *caps)
        : m_caps(caps)
    {
        m_desc.kind = OperationKind::Lasso;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 40;
        m_desc.acceptPrimary = true;
        m_desc.acceptSecondary = true;
    }

    OperationKind kind() const override { return OperationKind::Lasso; }
    const OperationDescriptor &descriptor() const override { return m_desc; }
    bool didMutateSelection() const override { return m_didMutate; }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        (void)s;
        if (channel != StrategyKind::RawPointer)
            return false;
        if (!m_caps || !m_caps->toolUi)
            return false;
        return m_caps->toolUi->exclusiveTool() == QLatin1String("sel_freeform");
    }

    void onDown(const PointerSample &s) override
    {
        if (!m_caps || !m_caps->toolUi || !m_caps->selection)
            return;
        m_didMutate = false;
        m_pts.clear();
        m_pts.push_back(s.panel);
        m_caps->selection->setPhase(SelectionPhase::Selecting);
        m_caps->toolUi->resetTransientChromeFlags();
        m_caps->toolUi->emitChromeChanged();
        m_caps->toolUi->syncOverlayPresence();
        m_caps->toolUi->setStrokeWaveform(true);
        m_caps->toolUi->damageChrome(QRectF(s.panel, s.panel).adjusted(-12, -12, 12, 12));
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

    void onUp(const PointerSample &s) override
    {
        (void)s;
        finish();
    }

    void onCancel() override { cancel(); }

    void cancel() override
    {
        m_pts.clear();
        if (!m_caps || !m_caps->toolUi || !m_caps->selection)
            return;
        m_caps->selection->setPhase(m_caps->selection->ids().empty() ? SelectionPhase::Idle
                                                                     : SelectionPhase::Selected);
        m_caps->toolUi->setStrokeWaveform(false);
        m_caps->toolUi->requestChromeRefresh();
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
    void finish()
    {
        if (!m_caps || !m_caps->doc || !m_caps->toolUi || !m_caps->selection)
            return;
        m_caps->toolUi->setStrokeWaveform(false);
        double pathLen = 0;
        double minX = 0, maxX = 0, minY = 0, maxY = 0;
        if (!m_pts.empty()) {
            minX = maxX = m_pts[0].x();
            minY = maxY = m_pts[0].y();
            for (size_t i = 1; i < m_pts.size(); ++i) {
                const double dx = m_pts[i].x() - m_pts[i - 1].x();
                const double dy = m_pts[i].y() - m_pts[i - 1].y();
                pathLen += std::sqrt(dx * dx + dy * dy);
                minX = std::min(minX, m_pts[i].x());
                maxX = std::max(maxX, m_pts[i].x());
                minY = std::min(minY, m_pts[i].y());
                maxY = std::max(maxY, m_pts[i].y());
            }
        }
        const double diag = std::hypot(maxX - minX, maxY - minY);
        const double gestureSize = std::max(pathLen, diag);
        if (gestureSize < kMinGesture) {
            m_caps->selection->clear();
            m_caps->selection->setPhase(SelectionPhase::Idle);
            m_caps->toolUi->setInteractionDebug("sel=0 (tap)");
            m_pts.clear();
            m_caps->toolUi->requestChromeRefresh();
            return;
        }
        std::vector<epaper::document::InkSample> poly;
        poly.reserve(m_pts.size());
        for (const QPointF &p : m_pts) {
            const auto w = m_caps->toolUi->panelToWorld(p);
            epaper::document::InkSample s;
            s.x = w.x;
            s.y = w.y;
            poly.push_back(s);
        }
        const auto hit = epaper::document::selectByFreeform(m_caps->doc->document(), poly);
        m_caps->selection->setIds(hit);
        m_caps->selection->setPhase(hit.empty() ? SelectionPhase::Idle : SelectionPhase::Selected);
        m_didMutate = true;
        std::string debug = hit.empty() ? "sel=0 (no nodes ≥80% inside)"
                                        : ("sel=" + std::to_string(hit.size()));
        for (const auto &id : hit)
            debug += " " + id;
        m_caps->toolUi->setInteractionDebug(debug);
        m_pts.clear();
        m_caps->toolUi->requestChromeRefresh();
    }

    HostCaps *m_caps = nullptr;
    std::vector<QPointF> m_pts;
    bool m_didMutate = false;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
