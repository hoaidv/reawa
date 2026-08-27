#include "tool_chrome.hpp"

#include "document/capability.hpp"
#include "document/manipulate.hpp"
#include "input_hub.hpp"
#include "rendering/rendering.hpp"
#include "rendering/rendering_qt.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {
QRectF modeChipRect(const QRectF &box)
{
    constexpr qreal w = 120.0;
    constexpr qreal h = 36.0;
    constexpr qreal gap = 32.0;
    return QRectF(box.center().x() - w * 0.5, box.bottom() + gap, w, h);
}
} // namespace

namespace epaper {
namespace tools {

void ToolChrome::refresh(epaper::selection::SelectionSession &selection,
                         epaper::manip::ManipSession &manip, SessionDocContext &doc,
                         bool isSelectionTool)
{
    using namespace epaper::document;
    m_state.encloseRefuseReason.clear();
    SmartBounds unionB;
    const std::vector<std::string> &ids = selection.ids;
    QRectF bounds;
    if (!ids.empty() && unionAabbOfIds(doc.document(), ids, unionB)) {
        const QPointF tl = doc.worldToPanel(unionB.x, unionB.y);
        const QPointF br =
            doc.worldToPanel(unionB.x + unionB.width, unionB.y + unionB.height);
        bounds = QRectF(tl, br).normalized();
    }
    m_state.encloseVisible =
        isSelectionTool && ids.size() >= 2 && !selection.isMarqueeOrLasso();
    if (m_state.encloseVisible && !bounds.isEmpty())
        m_state.encloseCtaRect =
            QRectF(bounds.center().x() - 32.0, bounds.bottom() + 36.0, 64.0, 64.0);
    else
        m_state.encloseCtaRect = QRectF();
    m_state.selectionChromeDirty = bounds.united(m_state.encloseCtaRect);
    if (ids.size() == 1 && !bounds.isEmpty())
        m_state.selectionChromeDirty = m_state.selectionChromeDirty.united(modeChipRect(bounds));
    m_state.selectionChromeDirty.adjust(-12, -12, 12, 12);
    m_state.selectionBoundsRect = bounds;
    m_state.handleCount = 0;
    m_state.handleSize = 16.0;
    m_state.modeChipVisible = false;
    m_state.modeChipLabel.clear();
    m_state.modeChipRect = QRectF();
    if (isSelectionTool && !ids.empty() && !bounds.isEmpty() && !selection.isMarqueeOrLasso()
        && !selection.isLiveManip()) {
        const DocNode *one = ids.size() == 1 ? doc.document().find(ids[0]) : nullptr;
        const bool manipChrome = one && descriptorFor(one->kind).has(Verb::Resize);
        m_state.handleCount = manipChrome ? 8 : 6;
        m_state.handleSize = manipChrome ? kHandleVisualDu : 16.0;
        if (manipChrome && one) {
            m_state.modeChipVisible = true;
            m_state.modeChipLabel = QString::fromStdString(
                one->inkScaleMode == "fixedInk" ? "Keep size" : "Scale ink");
            m_state.modeChipRect = modeChipRect(bounds);
        }
    }
    (void)manip;
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

void ToolChrome::syncPresence(epaper::selection::SelectionSession &selection,
                              bool isSelectionTool,
                              const std::function<void(bool visible)> &setVisible,
                              const std::function<void(bool penWaveform)> &setStrokeWaveform)
{
    const bool liveManip = selection.isLiveManip();
    const bool strokeChrome = selection.isMarqueeOrLasso();
    const bool settled =
        isSelectionTool && !selection.ids.empty() && !liveManip && !strokeChrome;
    const bool on = isSelectionTool && (strokeChrome || liveManip || settled);
    if (setVisible)
        setVisible(on);
    if (on && !strokeChrome && setStrokeWaveform)
        setStrokeWaveform(false);
}

void ToolChrome::paint(QPainter *painter, epaper::selection::SelectionSession &selection,
                       epaper::manip::ManipSession &manip, SessionDocContext &doc,
                       bool isSelectionTool)
{
    if (!isSelectionTool)
        return;

    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    if (selection.gesture == epaper::selection::Gesture::Move
        || selection.gesture == epaper::selection::Gesture::Resize) {
        using namespace epaper::document;
        const DocNode *n = doc.document().find(manip.nodeId);
        if (n) {
            epaper::render::FrameProjector proj;
            proj.frame = &doc.frame();
            epaper::render::RenderRequest req;
            req.sharp = true;
            epaper::render::DocumentRenderer renderer;
            epaper::render::QPainterPixelSink sink(painter);
            renderer.renderSubtree(doc.document(), proj, req, manip.nodeId, sink);

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

                if (manip.resizing()) {
                    const qreal h = kHandleVisualDu;
                    const QPointF pts[8] = {
                        r.topLeft(),
                        QPointF(r.center().x(), r.top()),
                        r.topRight(),
                        QPointF(r.right(), r.center().y()),
                        r.bottomRight(),
                        QPointF(r.center().x(), r.bottom()),
                        r.bottomLeft(),
                        QPointF(r.left(), r.center().y()),
                    };
                    painter->setBrush(Qt::white);
                    QPen solid(Qt::black);
                    solid.setWidthF(4.0);
                    painter->setPen(solid);
                    for (const QPointF &pt : pts)
                        painter->drawRect(QRectF(pt.x() - h * 0.5, pt.y() - h * 0.5, h, h));
                    const QRectF chip = modeChipRect(r);
                    painter->fillRect(chip, Qt::white);
                    painter->drawRect(chip);
                    painter->drawText(chip, Qt::AlignCenter,
                                      QString::fromStdString(n->inkScaleMode == "fixedInk"
                                                                 ? "Keep size"
                                                                 : "Scale ink"));
                }
            }
        }
        painter->restore();
        return;
    }

    QPen dotted(Qt::black);
    dotted.setWidthF(3.0);
    dotted.setStyle(Qt::DotLine);
    painter->setBrush(Qt::NoBrush);

    if (selection.gesture == epaper::selection::Gesture::Marquee) {
        painter->setPen(dotted);
        painter->drawRect(
            QRectF(QPointF(selection.marqueeStart.x, selection.marqueeStart.y),
                   QPointF(selection.marqueeEnd.x, selection.marqueeEnd.y))
                .normalized());
        painter->restore();
        return;
    }
    if (selection.gesture == epaper::selection::Gesture::Lasso && selection.lasso.size() >= 2) {
        painter->setPen(dotted);
        QPainterPath path;
        path.moveTo(QPointF(selection.lasso.front().x, selection.lasso.front().y));
        for (size_t i = 1; i < selection.lasso.size(); ++i)
            path.lineTo(QPointF(selection.lasso[i].x, selection.lasso[i].y));
        painter->drawPath(path);
        painter->restore();
        return;
    }

    if (selection.ids.empty() && selection.pickableId.empty() && !selection.active()) {
        painter->restore();
        return;
    }

    using namespace epaper::document;
    std::vector<std::string> ids = selection.ids;
    if (ids.empty() && !selection.pickableId.empty())
        ids.push_back(selection.pickableId);

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

void ToolChrome::redrawLiveManip(epaper::selection::SelectionSession &selection,
                                 epaper::manip::ManipSession &manip, SessionDocContext &doc,
                                 const std::function<void(const QRectF &)> &repaint,
                                 const std::function<void()> &emitChanged)
{
    using namespace epaper::document;
    SmartBounds wb;
    const DocNode *n = doc.document().find(manip.nodeId);
    QRectF liveBounds;
    QRectF next;
    if (n && boundsOf(*n, wb)) {
        liveBounds = doc.worldBoundsToPanel(wb);
        next = liveBounds.adjusted(-12, -12, 12, 48);
    }
    const QRectF connLive = doc.boundConnectorsPanelUnion(manip.nodeId);
    if (!connLive.isEmpty())
        next = next.isEmpty() ? connLive : next.united(connLive);
    const QRectF toolDirty = m_state.liveDirtyPrev.isNull()
        ? next.united(m_state.originPanelRect)
        : m_state.liveDirtyPrev.united(next);
    m_state.liveDirtyPrev = next;
    m_state.selectionChromeDirty = m_state.originPanelRect;
    if (!liveBounds.isEmpty())
        m_state.selectionBoundsRect = liveBounds;
    const bool holdKnobs = manip.active && manip.resizing();
    if (!holdKnobs)
        m_state.handleCount = 0;
    m_state.modeChipVisible = false;
    if (emitChanged)
        emitChanged();
    (void)selection;
    damage(toolDirty, repaint);
}

int ToolChrome::handleIndexAtPanel(const QPointF &panel, double hitDu) const
{
    if (m_state.handleCount != 8)
        return -1;
    const QRectF r = m_state.selectionBoundsRect;
    if (r.width() <= 0.0 || r.height() <= 0.0)
        return -1;
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
    const qreal half = hitDu * 0.5;
    for (int i = 0; i < 8; ++i) {
        if (qAbs(panel.x() - pts[i].x()) <= half && qAbs(panel.y() - pts[i].y()) <= half)
            return i;
    }
    return -1;
}

void ToolChrome::syncHitTargets(InputHub &hub) const
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
    const double hitDu = epaper::document::kHandleHitDu;
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
    m_state.encloseVisible = false;
    m_state.handleCount = 0;
    m_state.modeChipVisible = false;
}

} // namespace tools
} // namespace epaper
