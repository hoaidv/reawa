#pragma once

/**
 * ObjectErase — dotted freeform + AABB highlight for 80% candidates.
 * @implements [SRS-EP-54] erase_object exclusive
 * @implements [SRS-EP-58] object erase
 */

#include "../host_caps.hpp"
#include "../operation.hpp"
#include "document/erase_object.hpp"
#include "debug/ui_stall.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <string>
#include <vector>

namespace epaper {
namespace tools {

class ObjectEraseOperation final : public Operation, public RawPointerSink {
public:
    explicit ObjectEraseOperation(HostCaps *caps)
        : m_caps(caps)
    {
        m_desc.kind = OperationKind::ObjectErase;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 20;
        m_desc.acceptPrimary = true;
        m_desc.acceptSecondary = false;
    }

    OperationKind kind() const override { return OperationKind::ObjectErase; }
    const OperationDescriptor &descriptor() const override { return m_desc; }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        (void)s;
        if (channel != StrategyKind::RawPointer)
            return false;
        if (!m_caps || !m_caps->toolUi)
            return false;
        return m_caps->toolUi->exclusiveTool() == QLatin1String("erase_object");
    }

    void onDown(const PointerSample &s) override
    {
        m_pts.clear();
        m_pts.push_back(s.panel);
        refreshCandidates();
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
        refreshCandidates();
        m_caps->toolUi->damageChromeSegment(
            QRectF(prev, s.panel).normalized().adjusted(-16, -16, 16, 16));
    }
    void onUp(const PointerSample &) override { commit(); }
    void onCancel() override { cancel(); }
    void cancel() override
    {
        m_pts.clear();
        m_hitIds.clear();
        if (m_caps && m_caps->toolUi) {
            m_caps->toolUi->setStrokeWaveform(false);
            m_caps->toolUi->requestChromeRefresh();
        }
    }

    void paintOverlay(QPainter *painter) override
    {
        if (!painter)
            return;
        painter->save();
        QPen dotted(Qt::black);
        dotted.setWidthF(3.0);
        dotted.setStyle(Qt::DotLine);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(dotted);
        if (m_pts.size() == 1) {
            painter->drawEllipse(m_pts.front(), 2.0, 2.0);
        } else if (m_pts.size() > 1) {
            QPainterPath path;
            path.moveTo(m_pts.front());
            for (size_t i = 1; i < m_pts.size(); ++i)
                path.lineTo(m_pts[i]);
            painter->drawPath(path);
        }
        QPen thick(Qt::black);
        thick.setWidthF(5.0);
        thick.setStyle(Qt::DotLine);
        painter->setPen(thick);
        if (m_caps && m_caps->doc && m_caps->toolUi) {
            for (const auto &id : m_hitIds) {
                const epaper::document::DocNode *n = m_caps->doc->document().find(id);
                if (!n)
                    continue;
                epaper::document::SmartBounds wb;
                if (n->kind == epaper::document::NodeKind::Connector) {
                    const auto path = epaper::document::connectorWorldPath(*n);
                    if (path.empty())
                        continue;
                    wb = epaper::document::samplesAabb(path);
                } else if (!epaper::document::boundsOf(*n, wb)) {
                    continue;
                }
                const QRectF r = m_caps->toolUi->worldBoundsToPanel(wb);
                if (!r.isEmpty())
                    painter->drawRect(r);
            }
        }
        painter->restore();
    }

private:
    std::vector<epaper::document::ErasePt> worldPoly() const
    {
        std::vector<epaper::document::ErasePt> world;
        if (!m_caps || !m_caps->toolUi)
            return world;
        world.reserve(m_pts.size());
        for (const auto &p : m_pts) {
            const auto w = m_caps->toolUi->panelToWorld(p);
            world.push_back({w.x, w.y});
        }
        return world;
    }

    void refreshCandidates()
    {
        m_hitIds.clear();
        if (!m_caps || !m_caps->doc)
            return;
        m_hitIds = epaper::document::objectEraseCandidateIds(m_caps->doc->document(), worldPoly());
    }

    void commit()
    {
        using namespace epaper::document;
        std::vector<ErasePt> world = worldPoly();
        m_pts.clear();
        m_hitIds.clear();
        if (!m_caps || !m_caps->doc || !m_caps->toolUi)
            return;
        m_caps->toolUi->setStrokeWaveform(false);
        m_caps->toolUi->requestChromeRefresh();
        if (world.size() < 2)
            return;
        static int seq = 0;
        const std::string opId = std::string("erase-object-") + std::to_string(++seq);
        ApplyResult r;
        {
            epaper::UiStallSection stall("eraseObjectCommit");
            r = commitObjectErase(m_caps->doc->document(), opId, std::move(world));
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
    std::vector<std::string> m_hitIds;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
