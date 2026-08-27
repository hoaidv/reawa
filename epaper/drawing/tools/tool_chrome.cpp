#include "tool_chrome.hpp"

#include "document/capability.hpp"
#include "document/hand_touch.hpp"
#include "document/manipulate.hpp"
#include "input_hub.hpp"
#include "rendering/rendering.hpp"
#include "rendering/rendering_qt.hpp"

#include <QPainter>
#include <QtMath>

namespace epaper {
namespace tools {

void ToolChrome::refresh(SelectionContext &selection, SessionDocContext &doc, bool isSelectionTool)
{
    using namespace epaper::document;
    m_state.encloseRefuseReason.clear();
    SmartBounds unionB;
    const std::vector<std::string> &ids = selection.ids();
    QRectF bounds;
    if (!ids.empty() && unionAabbOfIds(doc.document(), ids, unionB)) {
        const QPointF tl = doc.worldToPanel(unionB.x, unionB.y);
        const QPointF br =
            doc.worldToPanel(unionB.x + unionB.width, unionB.y + unionB.height);
        bounds = QRectF(tl, br).normalized();
    }
    const bool stroke = selection.phase() == SelectionPhase::Selecting;
    const bool transforming = selection.phase() == SelectionPhase::Transforming;
    QRectF dirty = bounds;
    if (!bounds.isEmpty() && !stroke)
        dirty = bounds.adjusted(-12, -12, 12, 120);
    else
        dirty.adjust(-12, -12, 12, 12);
    m_state.selectionChromeDirty = dirty;
    m_state.selectionBoundsRect = bounds;
    m_state.liveDirtyPrev = QRectF();
    m_state.handleCount = 0;
    m_state.handleSize = 16.0;
    if (isSelectionTool && !ids.empty() && !bounds.isEmpty() && !stroke && !transforming) {
        const DocNode *one = ids.size() == 1 ? doc.document().find(ids[0]) : nullptr;
        const bool manipChrome = one && descriptorFor(one->kind).has(Verb::Resize);
        m_state.handleCount = manipChrome ? 8 : 6;
        m_state.handleSize = manipChrome ? kHandleVisualDu : 16.0;
    }
}

void ToolChrome::damage(const QRectF &next, const std::function<void(const QRectF &)> &repaint)
{
    const QRectF u = m_toolChromePrev.isNull() ? next : m_toolChromePrev.united(next);
    m_toolChromePrev = next;
    if (u.isEmpty() || !repaint)
        return;
    repaint(u.toAlignedRect().adjusted(-8, -8, 8, 8));
}

void ToolChrome::damageSegment(const QRectF &seg,
                               const std::function<void(const QRectF &)> &repaint)
{
    m_toolChromePrev = m_toolChromePrev.united(seg);
    if (seg.isEmpty() || !repaint)
        return;
    repaint(seg.toAlignedRect());
}

void ToolChrome::syncPresence(SelectionContext &selection, bool isSelectionTool, bool penWaveform,
                              const std::function<void(bool visible)> &setVisible,
                              const std::function<void(bool penWaveform)> &setStrokeWaveform)
{
    // Keep the overlay attached for the whole Selection mode so the first lasso
    // pen-down does not pay Mono-attach / waveform switch (ADR-0019).
    if (setVisible)
        setVisible(isSelectionTool);
    if (isSelectionTool && setStrokeWaveform)
        setStrokeWaveform(penWaveform);
    (void)selection;
}

void ToolChrome::paint(QPainter *painter, SelectionContext &selection, SessionDocContext &doc,
                       bool isSelectionTool)
{
    if (!isSelectionTool)
        return;

    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (selection.phase() == SelectionPhase::Transforming) {
        using namespace epaper::document;
        const std::string &id = selection.pickableId();
        const DocNode *n = doc.document().find(id);
        if (n) {
            epaper::render::FrameProjector proj;
            proj.frame = &doc.frame();
            epaper::render::RenderRequest req;
            req.sharp = true;
            epaper::render::DocumentRenderer renderer;
            epaper::render::QPainterPixelSink sink(painter);
            renderer.renderSubtree(doc.document(), proj, req, id, sink);

            SmartBounds wb;
            if (boundsOf(*n, wb)) {
                const QRectF r =
                    QRectF(doc.worldToPanel(wb.x, wb.y),
                           doc.worldToPanel(wb.x + wb.width, wb.y + wb.height))
                        .normalized();
                QPen dotted(Qt::black);
                dotted.setWidthF(3.0);
                dotted.setStyle(Qt::DotLine);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(dotted);
                painter->drawRect(r);
            }
        }
        painter->restore();
        return;
    }

    if (selection.phase() == SelectionPhase::Selecting) {
        painter->restore();
        return;
    }

    QPen dotted(Qt::black);
    dotted.setWidthF(3.0);
    dotted.setStyle(Qt::DotLine);
    painter->setBrush(Qt::NoBrush);

    if (selection.ids().empty() && selection.pickableId().empty()) {
        painter->restore();
        return;
    }

    using namespace epaper::document;
    std::vector<std::string> ids = selection.ids();
    if (ids.empty() && !selection.pickableId().empty())
        ids.push_back(selection.pickableId());

    SmartBounds unionB;
    QRectF r;
    if (unionAabbOfIds(doc.document(), ids, unionB)) {
        const QPointF tl = doc.worldToPanel(unionB.x, unionB.y);
        const QPointF br =
            doc.worldToPanel(unionB.x + unionB.width, unionB.y + unionB.height);
        r = QRectF(tl, br).normalized();
    }
    if (r.isEmpty()) {
        painter->restore();
        return;
    }

    painter->setPen(dotted);
    painter->drawRect(r);
    painter->restore();
}

void ToolChrome::redrawLiveManip(SelectionContext &selection, SessionDocContext &doc, bool resizing,
                                 const std::function<void(const QRectF &)> &repaint,
                                 const std::function<void()> &emitChanged)
{
    using namespace epaper::document;
    (void)resizing;
    SmartBounds wb;
    const DocNode *n = doc.document().find(selection.pickableId());
    QRectF liveBounds;
    QRectF next;
    if (n && boundsOf(*n, wb)) {
        liveBounds = doc.worldBoundsToPanel(wb);
        next = liveBounds.adjusted(-12, -12, 12, 48);
    }
    const QRectF connLive = doc.boundConnectorsPanelUnion(selection.pickableId());
    if (!connLive.isEmpty())
        next = next.isEmpty() ? connLive : next.united(connLive);
    const QRectF toolDirty = m_state.liveDirtyPrev.isNull()
        ? next.united(m_state.originPanelRect)
        : m_state.liveDirtyPrev.united(next);
    m_state.liveDirtyPrev = next;
    m_state.selectionChromeDirty = m_state.originPanelRect;
    if (!liveBounds.isEmpty())
        m_state.selectionBoundsRect = liveBounds;
    // QML knobs stay hidden during the live gesture — driving 8 items every
    // ghost frame is the resize stall. Overlay paints the live node + AABB.
    m_state.handleCount = 0;
    if (emitChanged)
        emitChanged();
    damage(toolDirty, repaint);
}

void ToolChrome::publishOverlayHits(InputHub &hub) const
{
    hub.clearHitRegions();
    if (m_state.handleCount != 8)
        return;
    const QRectF r = m_state.selectionBoundsRect;
    if (r.width() <= 0.0 || r.height() <= 0.0)
        return;
    const QPointF pts[8] = {
        {r.left(), r.top()},
        {r.center().x(), r.top()},
        {r.right(), r.top()},
        {r.right(), r.center().y()},
        {r.right(), r.bottom()},
        {r.center().x(), r.bottom()},
        {r.left(), r.bottom()},
        {r.left(), r.center().y()},
    };
    const double hitDu = epaper::handtouch::kFingerHandleHitDu;
    const qreal half = hitDu * 0.5;
    for (int i = 0; i < 8; ++i) {
        HitRegion hr;
        hr.panelRect = QRectF(pts[i].x() - half, pts[i].y() - half, hitDu, hitDu);
        hr.priority = 60;
        hr.ownerToken = reinterpret_cast<void *>(static_cast<intptr_t>(i));
        hub.registerHitRegion(hr);
    }
}

void ToolChrome::showManipUnavailable(const epaper::document::SmartBounds &wb,
                                      SessionDocContext &doc, qreal hostWidth, qreal hostHeight,
                                      const std::function<void(const QRectF &)> &repaint,
                                      const std::function<void()> &emitChanged)
{
    m_state.manipUnavailable = QStringLiteral("Too far out to move");
    const QRectF box = doc.worldBoundsToPanel(wb);
    constexpr qreal kW = 220.0;
    constexpr qreal kH = 36.0;
    qreal x = box.center().x() - kW * 0.5;
    qreal y = box.bottom() + 8.0;
    if (y + kH > hostHeight)
        y = std::max(8.0, box.top() - kH - 8.0);
    x = qBound(8.0, x, qMax(8.0, hostWidth - kW - 8.0));
    m_state.manipUnavailableRect = QRectF(x, y, kW, kH);
    if (emitChanged)
        emitChanged();
    damage(m_state.manipUnavailableRect, repaint);
}

void ToolChrome::resetTransientFlags()
{
    m_state.handleCount = 0;
}

} // namespace tools
} // namespace epaper
