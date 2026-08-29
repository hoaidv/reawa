#pragma once

/**
 * BrushErase — capsule clip of Ink along the primary stroke.
 * @implements [SRS-EP-56] brush erase
 * @implements [SRS-EP-54] erase_brush exclusive
 */

#include "../host_caps.hpp"
#include "../operation.hpp"
#include "document/erase_clip.hpp"
#include "document/erase_commit.hpp"
#include "debug/ui_stall.hpp"

#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QString>
#include <cmath>
#include <string>
#include <vector>

namespace epaper {
namespace tools {

class BrushEraseOperation final : public Operation, public RawPointerSink {
public:
    explicit BrushEraseOperation(HostCaps *caps)
        : m_caps(caps)
    {
        m_desc.kind = OperationKind::BrushErase;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 20;
        m_desc.acceptPrimary = true;
        m_desc.acceptSecondary = false;
    }

    OperationKind kind() const override { return OperationKind::BrushErase; }
    const OperationDescriptor &descriptor() const override { return m_desc; }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        (void)s;
        if (channel != StrategyKind::RawPointer)
            return false;
        if (!m_caps || !m_caps->toolUi)
            return false;
        return m_caps->toolUi->exclusiveTool() == QLatin1String("erase_brush");
    }

    void onDown(const PointerSample &s) override
    {
        m_world.clear();
        m_panel.clear();
        append(s);
        if (m_caps && m_caps->toolUi) {
            m_caps->toolUi->syncOverlayPresence();
            m_caps->toolUi->damageChrome(ghostDirty(s.panel));
        }
    }

    void onMove(const PointerSample &s) override
    {
        const QPointF prev = m_panel.empty() ? s.panel : m_panel.back();
        append(s);
        if (m_caps && m_caps->toolUi) {
            const double pad = ghostPad();
            m_caps->toolUi->damageChromeSegment(
                QRectF(prev, s.panel).normalized().adjusted(-pad, -pad, pad, pad));
        }
    }

    void onUp(const PointerSample &s) override
    {
        append(s);
        commit();
    }

    void onCancel() override { cancel(); }

    void cancel() override
    {
        m_world.clear();
        m_panel.clear();
        if (m_caps && m_caps->toolUi)
            m_caps->toolUi->requestChromeRefresh();
    }

    void paintOverlay(QPainter *painter) override
    {
        if (!painter || m_panel.size() < 1 || !m_caps || !m_caps->toolUi)
            return;
        painter->save();
        QPen pen(Qt::white);
        pen.setWidthF(std::max(1.0, epaper::document::eraseMmToWorld(
                                        epaper::document::kEraseBrushDiameterMm) *
                                        m_caps->toolUi->panelScale()));
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        if (m_panel.size() == 1) {
            painter->drawPoint(m_panel.front());
        } else {
            QPainterPath path;
            path.moveTo(m_panel.front());
            for (size_t i = 1; i < m_panel.size(); ++i)
                path.lineTo(m_panel[i]);
            painter->drawPath(path);
        }
        painter->restore();
    }

private:
    void append(const PointerSample &s)
    {
        m_panel.push_back(s.panel);
        if (!m_caps || !m_caps->toolUi)
            return;
        const auto w = m_caps->toolUi->panelToWorld(s.panel);
        epaper::document::ErasePt p{w.x, w.y};
        if (!m_world.empty()) {
            const auto &last = m_world.back();
            if (std::hypot(p.x - last.x, p.y - last.y) < 1e-6)
                return;
        }
        m_world.push_back(p);
    }

    double ghostPad() const
    {
        if (!m_caps || !m_caps->toolUi)
            return 12.0;
        return epaper::document::eraseMmToWorld(epaper::document::kEraseBrushDiameterMm) *
                   m_caps->toolUi->panelScale() +
               4.0;
    }

    QRectF ghostDirty(const QPointF &p) const
    {
        const double pad = ghostPad();
        return QRectF(p, p).adjusted(-pad, -pad, pad, pad);
    }

    void commit()
    {
        using namespace epaper::document;
        std::vector<ErasePt> path = m_world;
        m_world.clear();
        m_panel.clear();
        if (!m_caps || !m_caps->doc || !m_caps->toolUi)
            return;
        m_caps->toolUi->requestChromeRefresh();
        if (path.empty())
            return;
        static int seq = 0;
        const std::string opId = std::string("erase-brush-") + std::to_string(++seq);
        const ClipRegion region =
            capsuleRegion(std::move(path), eraseMmToWorld(kEraseBrushRadiusMm));
        ApplyResult r;
        {
            epaper::UiStallSection stall("eraseCommit");
            r = commitEraseRegion(m_caps->doc->document(), opId, region);
        }
        qInfo() << "[erase] commit" << QString::fromStdString(opId)
                << "path" << int(region.path.size()) << "radius" << region.radius
                << "inks" << int(m_caps->doc->document().inkCount())
                << "reason" << QString::fromStdString(r.reason);
        if (r.applied && r.reason != "noop") {
            m_caps->doc->noteDocumentMutated();
            m_caps->doc->notifyHistory();
            m_caps->doc->flushWire();
        }
        m_caps->toolUi->requestChromeRefresh();
    }

    HostCaps *m_caps = nullptr;
    std::vector<epaper::document::ErasePt> m_world;
    std::vector<QPointF> m_panel;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
