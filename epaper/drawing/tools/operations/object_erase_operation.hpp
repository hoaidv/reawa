#pragma once

/**
 * ObjectErase — dotted freeform + AABB highlight for 80% candidates.
 * @implements [SRS-EP-54] erase_object exclusive
 * @implements [SRS-EP-58] object erase
 */

#include "../host_caps.hpp"
#include "../operation.hpp"
#include "../contexts/doc_context.hpp"
#include "../contexts/tool_context.hpp"
#include "document/erase_object.hpp"
#include "debug/ui_stall.hpp"

#include <QElapsedTimer>
#include <QLatin1String>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>
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
        if (!m_caps || !m_caps->doc)
            return false;
        return m_caps->doc->exclusiveTool() == QLatin1String("erase_object");
    }

    void onDown(const PointerSample &s) override
    {
        m_pts.clear();
        m_hitPanelRects.clear();
        m_candidateClock.invalidate();
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
            QRectF(prev, s.panel).normalized().adjusted(-16, -16, 16, 16));
        maybeRefreshCandidates();
    }
    void onUp(const PointerSample &) override { commit(); }
    void onCancel() override { cancel(); }
    void cancel() override
    {
        m_pts.clear();
        m_hitPanelRects.clear();
        m_candidateClock.invalidate();
        if (m_caps && m_caps->toolUi) {
            m_caps->toolUi->setStrokeWaveform(false);
            m_caps->toolUi->refreshChrome();
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
            const std::vector<QPointF> draw = downsamplePanel(m_pts, kOverlayPaintMax);
            QPainterPath path;
            path.moveTo(draw.front());
            for (size_t i = 1; i < draw.size(); ++i)
                path.lineTo(draw[i]);
            painter->drawPath(path);
        }
        QPen thick(Qt::black);
        thick.setWidthF(5.0);
        thick.setStyle(Qt::DotLine);
        painter->setPen(thick);
        for (const QRectF &r : m_hitPanelRects)
            painter->drawRect(r);
        painter->restore();
    }

private:
    static constexpr size_t kOverlayPaintMax = 96;
    static constexpr size_t kOverlayHitMax = 48;

    static std::vector<QPointF> downsamplePanel(const std::vector<QPointF> &pts, size_t maxPts)
    {
        if (maxPts < 2 || pts.size() <= maxPts)
            return pts;
        std::vector<QPointF> o;
        o.reserve(maxPts);
        const size_t last = pts.size() - 1;
        for (size_t i = 0; i < maxPts; ++i) {
            const size_t j = i * last / (maxPts - 1);
            o.push_back(pts[j]);
        }
        return o;
    }

    std::vector<epaper::document::ErasePt> worldPoly() const
    {
        return worldPolyFrom(m_pts);
    }

    std::vector<epaper::document::ErasePt> worldPolyFrom(const std::vector<QPointF> &pts) const
    {
        std::vector<epaper::document::ErasePt> world;
        if (!m_caps || !m_caps->toolUi)
            return world;
        world.reserve(pts.size());
        for (const auto &p : pts) {
            const auto w = m_caps->toolUi->panelToWorld(p);
            world.push_back({w.x, w.y});
        }
        return world;
    }

    static QRectF paddedPanelAabb(QRectF r)
    {
        if (r.width() < 8.0) {
            const qreal cx = r.center().x();
            r.setLeft(cx - 4.0);
            r.setRight(cx + 4.0);
        }
        if (r.height() < 8.0) {
            const qreal cy = r.center().y();
            r.setTop(cy - 4.0);
            r.setBottom(cy + 4.0);
        }
        return r.adjusted(-3.0, -3.0, 3.0, 3.0);
    }

    QRectF nodePanelAabb(const epaper::document::DocNode &n) const
    {
        if (!m_caps || !m_caps->toolUi)
            return {};
        epaper::document::SmartBounds wb;
        if (n.kind == epaper::document::NodeKind::Connector) {
            const auto path = epaper::document::connectorWorldPath(n);
            if (path.empty())
                return {};
            wb = epaper::document::samplesAabb(path);
        } else if (!epaper::document::boundsOf(n, wb)) {
            return {};
        }
        return paddedPanelAabb(m_caps->toolUi->worldBoundsToPanel(wb));
    }

    QRectF unionHitRects(const std::vector<QRectF> &rects) const
    {
        QRectF u;
        for (const QRectF &r : rects)
            u = u.united(r);
        return u;
    }

    void maybeRefreshCandidates()
    {
        if (!m_caps || !m_caps->doc || !m_caps->toolUi)
            return;
        if (m_pts.size() < 3)
            return;
        int period = kCandidatePeriodMs;
        if (m_pts.size() > 400)
            period = 100;
        if (m_pts.size() > 1200)
            period = 160;
        if (m_candidateClock.isValid() && m_candidateClock.elapsed() < period)
            return;
        m_candidateClock.start();
        const auto ids = epaper::document::objectEraseCandidateIds(
            m_caps->doc->document(), worldPolyFrom(downsamplePanel(m_pts, kOverlayHitMax)),
            epaper::document::ObjectErasePass::Overlay);
        std::vector<QRectF> next;
        next.reserve(ids.size());
        for (const auto &id : ids) {
            const epaper::document::DocNode *n = m_caps->doc->document().find(id);
            if (!n)
                continue;
            const QRectF r = nodePanelAabb(*n);
            if (!r.isNull())
                next.push_back(r);
        }
        if (next.size() == m_hitPanelRects.size()) {
            bool same = true;
            for (size_t i = 0; i < next.size(); ++i) {
                if (next[i] != m_hitPanelRects[i]) {
                    same = false;
                    break;
                }
            }
            if (same)
                return;
        }
        const QRectF dirty = unionHitRects(m_hitPanelRects).united(unionHitRects(next));
        m_hitPanelRects = std::move(next);
        if (!dirty.isEmpty())
            m_caps->toolUi->damageChrome(dirty);
    }

    void commit()
    {
        using namespace epaper::document;
        std::vector<ErasePt> world = worldPoly();
        m_pts.clear();
        m_hitPanelRects.clear();
        m_candidateClock.invalidate();
        if (!m_caps || !m_caps->doc || !m_caps->toolUi)
            return;
        m_caps->toolUi->setStrokeWaveform(false);
        m_caps->toolUi->refreshChrome();
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
        m_caps->toolUi->refreshChrome();
    }

    static constexpr int kCandidatePeriodMs = 50;

    HostCaps *m_caps = nullptr;
    std::vector<QPointF> m_pts;
    std::vector<QRectF> m_hitPanelRects;
    QElapsedTimer m_candidateClock;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
