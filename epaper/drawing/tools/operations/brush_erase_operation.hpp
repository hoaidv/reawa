#pragma once

/**
 * BrushErase — capsule clip of Ink along the primary stroke.
 * Ghost is stamped into a persistent overlay image (same pattern as live ink)
 * so paint is O(dirty) rather than O(samples).
 * @implements [SRS-EP-56] brush erase
 * @implements [SRS-EP-54] erase_brush exclusive
 * @implements [SRS-EP-59] erase quality
 */

#include "../host_caps.hpp"
#include "../operation.hpp"
#include "../contexts/session_doc_context.hpp"
#include "document/erase_clip.hpp"
#include "document/erase_commit.hpp"
#include "debug/ui_stall.hpp"

#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QString>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace epaper {
namespace tools {

class BrushEraseOperation final : public Operation, public RawPointerSink, public StylusHoverSink {
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
        if (channel != StrategyKind::RawPointer && channel != StrategyKind::StylusHover)
            return false;
        if (!m_caps || !m_caps->toolUi)
            return false;
        return m_caps->toolUi->exclusiveTool() == QLatin1String("erase_brush");
    }

    void onHoverEnter(const PointerSample &s) override { applyHover(s.panel); }
    void onHoverMove(const PointerSample &s) override { applyHover(s.panel); }
    void onHoverLeave() override { dropHover(); }

    void onDown(const PointerSample &s) override
    {
        m_world.clear();
        m_ghost = QImage();
        m_ghostDirty = QRectF();
        m_lastPanel = s.panel;
        m_hasPanel = true;
        appendWorld(s);
        stampGhost(s.panel, s.panel);
        m_ghostDirty = ghostDirty(s.panel);
        if (m_caps && m_caps->toolUi) {
            m_caps->toolUi->syncOverlayPresence();
            m_caps->toolUi->setStrokeWaveform(true);
            m_caps->toolUi->damageChrome(m_ghostDirty);
        }
    }

    void onMove(const PointerSample &s) override
    {
        const QPointF prev = m_hasPanel ? m_lastPanel : s.panel;
        appendWorld(s);
        stampGhost(prev, s.panel);
        m_lastPanel = s.panel;
        m_hasPanel = true;
        if (m_caps && m_caps->toolUi) {
            const double pad = ghostPad();
            const QRectF seg =
                QRectF(prev, s.panel).normalized().adjusted(-pad, -pad, pad, pad);
            m_ghostDirty = m_ghostDirty.united(seg);
            m_caps->toolUi->damageChromeSegment(seg);
        }
    }

    void onUp(const PointerSample &s) override
    {
        appendWorld(s);
        commit();
    }

    void onCancel() override { cancel(); }

    void cancel() override
    {
        m_world.clear();
        dropGhost();
        if (m_caps && m_caps->toolUi)
            m_caps->toolUi->requestChromeRefresh();
    }

    void paintOverlay(QPainter *painter) override
    {
        if (!painter)
            return;
        if (!m_ghost.isNull()) {
            const QRectF clip = painter->clipBoundingRect();
            if (clip.isEmpty())
                painter->drawImage(0, 0, m_ghost);
            else
                painter->drawImage(clip.topLeft(), m_ghost, clip);
            return;
        }
        paintHover(painter);
    }

private:
    bool hoverEnabled() const
    {
        auto *sess = dynamic_cast<SessionDocContext *>(m_caps ? m_caps->doc : nullptr);
        return sess && sess->session() && sess->session()->chip.eraseBrushHover;
    }

    void applyHover(const QPointF &panel)
    {
        if (!hoverEnabled()) {
            dropHover();
            return;
        }
        const double scale = m_caps && m_caps->toolUi ? m_caps->toolUi->panelScale() : 1.0;
        const double d =
            epaper::document::eraseMmToWorld(epaper::document::kEraseBrushDiameterMm) * scale;
        const double stroke =
            epaper::document::eraseMmToWorld(epaper::document::kEraseHoverStrokeMm) * scale;
        const double pad = d * 0.5 + stroke + 2.0;
        const QRectF next = QRectF(panel, panel).adjusted(-pad, -pad, pad, pad);
        QRectF dirty = next;
        const bool wasValid = m_hoverValid;
        if (wasValid)
            dirty = dirty.united(m_hoverDirty);
        m_hoverPanel = panel;
        m_hoverValid = true;
        m_hoverDirty = next;
        if (!wasValid && m_caps && m_caps->toolUi)
            m_caps->toolUi->syncOverlayPresence();
        if (m_caps && m_caps->toolUi)
            m_caps->toolUi->damageChrome(dirty);
    }

    void dropHover()
    {
        if (!m_hoverValid)
            return;
        const QRectF dirty = m_hoverDirty;
        m_hoverValid = false;
        m_hoverDirty = QRectF();
        if (m_caps && m_caps->toolUi)
            m_caps->toolUi->damageChrome(dirty);
    }

    void paintHover(QPainter *painter)
    {
        if (!m_hoverValid || !hoverEnabled())
            return;
        if (!m_caps || !m_caps->toolUi)
            return;
        const double scale = m_caps->toolUi->panelScale();
        const double d =
            epaper::document::eraseMmToWorld(epaper::document::kEraseBrushDiameterMm) * scale;
        const double stroke = std::max(
            1.0, epaper::document::eraseMmToWorld(epaper::document::kEraseHoverStrokeMm) * scale);
        painter->save();
        QPen pen(Qt::black);
        pen.setWidthF(stroke);
        painter->setPen(pen);
        painter->setBrush(Qt::white);
        painter->drawEllipse(m_hoverPanel, d * 0.5, d * 0.5);
        painter->restore();
    }

    void appendWorld(const PointerSample &s)
    {
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

    QPen ghostPen() const
    {
        QPen pen(Qt::white);
        const double w = (!m_caps || !m_caps->toolUi)
            ? 1.0
            : epaper::document::eraseMmToWorld(epaper::document::kEraseBrushDiameterMm) *
                  m_caps->toolUi->panelScale();
        pen.setWidthF(std::max(1.0, w));
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        return pen;
    }

    void ensureGhost()
    {
        if (!m_caps || !m_caps->toolUi)
            return;
        const QSizeF hs = m_caps->toolUi->hostSize();
        const QSize sz(std::max(1, int(hs.width())), std::max(1, int(hs.height())));
        if (m_ghost.size() == sz)
            return;
        QImage next(sz, QImage::Format_ARGB32_Premultiplied);
        next.fill(Qt::transparent);
        if (!m_ghost.isNull()) {
            QPainter p(&next);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.drawImage(0, 0, m_ghost);
        }
        m_ghost = next;
    }

    void stampGhost(const QPointF &from, const QPointF &to)
    {
        ensureGhost();
        if (m_ghost.isNull())
            return;
        QPainter p(&m_ghost);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setPen(ghostPen());
        p.setBrush(Qt::NoBrush);
        if (from == to)
            p.drawPoint(from);
        else
            p.drawLine(from, to);
    }

    void dropGhost()
    {
        const QRectF dirty = m_ghostDirty;
        m_ghost = QImage();
        m_ghostDirty = QRectF();
        m_hasPanel = false;
        if (!dirty.isEmpty() && m_caps && m_caps->toolUi)
            m_caps->toolUi->damageChrome(dirty);
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
        dropGhost();
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
    QImage m_ghost;
    QRectF m_ghostDirty;
    QPointF m_lastPanel;
    bool m_hasPanel = false;
    QPointF m_hoverPanel;
    bool m_hoverValid = false;
    QRectF m_hoverDirty;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
