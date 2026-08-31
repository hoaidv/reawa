#pragma once

/**
 * ObjectErase — full dotted freeform paint; 80% overlay off the UI thread.
 * @implements [SRS-EP-54] erase_object exclusive
 * @implements [SRS-EP-58] object erase
 */

#include "../host_caps.hpp"
#include "../operation.hpp"
#include "../contexts/doc_context.hpp"
#include "../contexts/tool_context.hpp"
#include "document/erase_object.hpp"
#include "debug/ui_stall.hpp"
#include "util/latest_job.hpp"

#include <QCoreApplication>
#include <QLatin1String>
#include <QMetaObject>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace epaper {
namespace tools {

class ObjectEraseOperation final : public Operation, public RawPointerSink {
public:
    explicit ObjectEraseOperation(HostCaps *caps)
        : m_caps(caps)
        , m_bridge(std::make_shared<Bridge>())
    {
        m_desc.kind = OperationKind::ObjectErase;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 20;
        m_desc.acceptPrimary = true;
        m_desc.acceptSecondary = false;
        m_bridge->op = this;
        m_job.start(
            [](const epaper::document::ObjectEraseOverlayJob &job,
               const std::atomic<bool> &cancel) {
                return epaper::document::objectEraseRunOverlayJob(job, cancel);
            },
            [bridge = m_bridge](epaper::document::ObjectEraseOverlayResult result) {
                QCoreApplication *app = QCoreApplication::instance();
                if (!app)
                    return;
                QMetaObject::invokeMethod(
                    app,
                    [bridge, result = std::move(result)]() mutable {
                        if (!bridge->alive.load() || !bridge->op)
                            return;
                        bridge->op->onOverlayResult(std::move(result));
                    },
                    Qt::QueuedConnection);
            });
    }

    ~ObjectEraseOperation() override
    {
        if (m_bridge) {
            m_bridge->alive = false;
            m_bridge->op = nullptr;
        }
        m_job.stop();
    }

    ObjectEraseOperation(const ObjectEraseOperation &) = delete;
    ObjectEraseOperation &operator=(const ObjectEraseOperation &) = delete;

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
        bumpEpoch();
        m_pts.clear();
        m_hitPanelRects.clear();
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
        requestOverlayHits();
    }
    void onUp(const PointerSample &) override { commit(); }
    void onCancel() override { cancel(); }
    void cancel() override
    {
        bumpEpoch();
        m_pts.clear();
        m_hitPanelRects.clear();
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
        dotted.setWidthF(kFreeformStrokePanelPx);
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
        thick.setWidthF(kDeletionRectStrokePanelPx);
        thick.setCosmetic(true);
        thick.setStyle(Qt::DotLine);
        painter->setPen(thick);
        for (const QRectF &r : m_hitPanelRects)
            painter->drawRect(r);
        painter->restore();
    }

private:
    struct Bridge {
        std::atomic<bool> alive{true};
        ObjectEraseOperation *op = nullptr;
    };

    /** Dotted freeform — panel pixels (ToolCanvas). */
    static constexpr qreal kFreeformStrokePanelPx = 3.0;
    /** Deletion-rect around 80% candidates. Cosmetic: same px at any world zoom. Half of 5. */
    static constexpr qreal kDeletionRectStrokePanelPx = 2.0;

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

    static std::vector<QPointF> downsamplePanel(const std::vector<QPointF> &pts, size_t maxPts)
    {
        if (maxPts < 2 || pts.size() <= maxPts)
            return pts;
        std::vector<QPointF> o;
        o.reserve(maxPts);
        const size_t last = pts.size() - 1;
        for (size_t i = 0; i < maxPts; ++i)
            o.push_back(pts[i * last / (maxPts - 1)]);
        return o;
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

    void bumpEpoch()
    {
        ++m_submitGen;
        m_appliedGen = m_submitGen;
        m_jobBusy = false;
        m_resnapWaiting = false;
    }

    void requestOverlayHits()
    {
        if (!m_caps || !m_caps->doc || !m_caps->toolUi)
            return;
        if (m_pts.size() < 3)
            return;
        if (m_jobBusy) {
            m_resnapWaiting = true;
            return;
        }
        startOverlayJob();
    }

    void startOverlayJob()
    {
        m_resnapWaiting = false;
        m_jobBusy = true;
        const std::uint64_t gen = ++m_submitGen;
        auto job = epaper::document::objectEraseMakeOverlayJob(
            m_caps->doc->document(),
            worldPolyFrom(downsamplePanel(m_pts, epaper::document::kObjectEraseOverlayLassoMax)),
            gen);
        m_job.submit(std::move(job));
    }

    void onOverlayResult(epaper::document::ObjectEraseOverlayResult result)
    {
        m_jobBusy = false;
        if (result.gen < m_appliedGen) {
            if (m_resnapWaiting && !m_pts.empty())
                startOverlayJob();
            return;
        }
        m_appliedGen = result.gen;
        if (!m_caps || !m_caps->doc || !m_caps->toolUi || m_pts.size() < 3) {
            if (m_resnapWaiting && !m_pts.empty())
                startOverlayJob();
            return;
        }
        std::vector<QRectF> next;
        next.reserve(result.ids.size());
        for (const auto &id : result.ids) {
            const epaper::document::DocNode *n = m_caps->doc->document().find(id);
            if (!n)
                continue;
            const QRectF r = nodePanelAabb(*n);
            if (!r.isNull())
                next.push_back(r);
        }
        const bool same = next.size() == m_hitPanelRects.size()
            && std::equal(next.begin(), next.end(), m_hitPanelRects.begin());
        if (!same) {
            const QRectF dirty = unionHitRects(m_hitPanelRects).united(unionHitRects(next));
            m_hitPanelRects = std::move(next);
            if (!dirty.isEmpty())
                m_caps->toolUi->damageChrome(dirty);
        }
        if (m_resnapWaiting && !m_pts.empty())
            startOverlayJob();
    }

    void commit()
    {
        using namespace epaper::document;
        std::vector<ErasePt> world = worldPoly();
        bumpEpoch();
        m_pts.clear();
        m_hitPanelRects.clear();
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

    HostCaps *m_caps = nullptr;
    std::vector<QPointF> m_pts;
    std::vector<QRectF> m_hitPanelRects;
    OperationDescriptor m_desc;
    epaper::LatestJob<epaper::document::ObjectEraseOverlayJob, epaper::document::ObjectEraseOverlayResult>
        m_job;
    std::shared_ptr<Bridge> m_bridge;
    std::uint64_t m_submitGen = 0;
    std::uint64_t m_appliedGen = 0;
    bool m_jobBusy = false;
    bool m_resnapWaiting = false;
};

} // namespace tools
} // namespace epaper
