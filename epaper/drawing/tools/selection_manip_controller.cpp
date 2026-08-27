#include "selection_manip_controller.hpp"

#include "../tabletcanvasitem.h"
#include "debug/ui_stall.hpp"
#include "document/capability.hpp"
#include "document/manipulate.hpp"
#include "input_hub.hpp"
#include "operations/lasso_operation.hpp"
#include "operations/marquee_operation.hpp"
#include "operation.hpp"
#include "tool_chrome.hpp"

#include <QEvent>

namespace epaper {
namespace tools {

void SelectionManipController::clearSelection()
{
    if (!m_selection || !m_manip)
        return;
    m_selection->clear();
    m_manip->clearNodeId();
    m_selection->gesture = epaper::selection::Gesture::None;
}

void SelectionManipController::onDocumentOrCameraChanged()
{
    if (!m_doc || !m_selection || !m_tool)
        return;
    std::vector<std::string> keep;
    keep.reserve(m_selection->ids.size());
    for (const std::string &id : m_selection->ids) {
        if (m_doc->document().find(id))
            keep.push_back(id);
    }
    m_selection->setIds(keep);
    if (!m_selection->pickableId.empty() && !m_doc->document().find(m_selection->pickableId)) {
        m_selection->pickableId.clear();
        m_manip->clearNodeId();
    }
    m_tool->requestChromeRefresh();
}

bool SelectionManipController::selectionGestureActive() const
{
    return m_selection && m_selection->active();
}

void SelectionManipController::applySelectionIntent(const epaper::selection::SelectionResult &r)
{
    if (m_selApplier)
        m_selApplier->apply(r);
}

void SelectionManipController::applyManipIntent(const epaper::manip::ManipResult &r,
                                                bool restoreOrigin)
{
    if (m_manipApplier && m_manip)
        m_manipApplier->apply(r, *m_manip, restoreOrigin, m_toolIntentSeq);
}

bool SelectionManipController::beginMoveFromPanel(const QPointF &panel, bool armSelFreeform)
{
    using namespace epaper::document;
    if (!m_doc || !m_manip || !m_selection || !m_tool || !m_chrome)
        return false;
    if (armSelFreeform)
        m_doc->setExclusiveTool(QStringLiteral("sel_freeform"));
    m_chrome->state().encloseRefuseReason.clear();
    m_chrome->state().manipUnavailable.clear();
    m_chrome->state().manipUnavailableRect = QRectF();
    const auto w = m_doc->panelToWorld(panel.x(), panel.y());
    const DocNode *hit = m_doc->pickTopMoveTarget(w.x, w.y);

    CapabilityDescriptor cap;
    bool lodOk = true;
    if (hit) {
        cap = descriptorFor(hit->kind);
        SmartBounds wb;
        if (boundsOf(*hit, wb))
            lodOk = m_doc->lodOkPanel(wb);
    }

    const GestureKind kind = resolvePress(cap, lodOk, false, false, hit != nullptr);
    if (kind == GestureKind::Unavailable) {
        SmartBounds wb;
        const QSizeF host = m_hostSize ? m_hostSize() : QSizeF();
        if (hit && boundsOf(*hit, wb)) {
            m_chrome->showManipUnavailable(
                wb, *m_doc, host.width(), host.height(),
                [this](const QRectF &r) { m_tool->damageChrome(r); }, [this]() { m_tool->emitChromeChanged(); });
        } else {
            m_chrome->state().manipUnavailable = QStringLiteral("Too far out to move");
            m_tool->emitChromeChanged();
            m_tool->damageChrome(m_chrome->state().manipUnavailableRect);
        }
        return false;
    }
    if (kind == GestureKind::SelectMove && hit) {
        if (armSelFreeform && m_finger)
            m_finger->gesture = epaper::fingergesture::Kind::Move;
        startLiveManip(hit, ResizeHandle::None, w.x, w.y);
        return true;
    }
    return false;
}

bool SelectionManipController::tryBeginHandleAtPanel(const QPointF &panel, double hitDu)
{
    if (!m_doc || !m_chrome)
        return false;
    const int idx = m_chrome->handleIndexAtPanel(panel, hitDu);
    if (idx < 0)
        return false;
    const auto w = m_doc->panelToWorld(panel.x(), panel.y());
    beginHandleDrag(idx, w.x, w.y);
    return true;
}

void SelectionManipController::beginSelectionGesture(const QPointF &canvasPos)
{
    using namespace epaper::document;
    if (!m_doc || !m_manip || !m_selection || !m_tool || !m_chrome)
        return;
    m_chrome->state().encloseRefuseReason.clear();
    m_chrome->state().manipUnavailable.clear();
    m_chrome->state().manipUnavailableRect = QRectF();
    const auto w = m_doc->panelToWorld(canvasPos.x(), canvasPos.y());
    const DocNode *hit = m_doc->pickTopMoveTarget(w.x, w.y);

    CapabilityDescriptor cap;
    bool lodOk = true;
    if (hit) {
        cap = descriptorFor(hit->kind);
        SmartBounds wb;
        if (boundsOf(*hit, wb))
            lodOk = m_doc->lodOkPanel(wb);
    }

    const GestureKind kind = resolvePress(cap, lodOk, false, false, hit != nullptr);
    if (kind == GestureKind::Unavailable) {
        SmartBounds wb;
        const QSizeF host = m_hostSize ? m_hostSize() : QSizeF();
        if (hit && boundsOf(*hit, wb)) {
            m_chrome->showManipUnavailable(
                wb, *m_doc, host.width(), host.height(),
                [this](const QRectF &r) { m_tool->damageChrome(r); }, [this]() { m_tool->emitChromeChanged(); });
        } else {
            m_chrome->state().manipUnavailable = QStringLiteral("Too far out to move");
            m_tool->emitChromeChanged();
            m_tool->damageChrome(m_chrome->state().manipUnavailableRect);
        }
        return;
    }
    if (kind == GestureKind::SelectMove && hit) {
        startLiveManip(hit, ResizeHandle::None, w.x, w.y);
        return;
    }
    beginMarqueeOrLasso(canvasPos);
}

void SelectionManipController::updateSelectionGesture(const QPointF &canvasPos)
{
    if (!m_selection || !m_hub || !m_doc)
        return;
    if (m_hub->lockedOperation()) {
        PointerSample s;
        s.panel = canvasPos;
        s.device = PointerDevice::Pen;
        m_hub->dispatchPointerMove(s);
        return;
    }
    if (!selectionGestureActive())
        return;
    if (m_getSelectStroke && m_getSelectStroke()) {
        if (m_feedSelectStroke)
            m_feedSelectStroke(QEvent::TabletMove, canvasPos);
        return;
    }
    if (m_selection->gesture == epaper::selection::Gesture::Marquee) {
        applySelectionIntent(m_selection->updateMarquee(canvasPos.x(), canvasPos.y()));
        return;
    }
    if (m_selection->gesture == epaper::selection::Gesture::Lasso) {
        applySelectionIntent(m_selection->updateLasso(canvasPos.x(), canvasPos.y()));
        return;
    }
    const auto w = m_doc->panelToWorld(canvasPos.x(), canvasPos.y());
    applyDragWorld(w.x, w.y);
}

void SelectionManipController::endSelectionGesture()
{
    if (!m_selection)
        return;
    const bool strokeActive = m_getSelectStroke && m_getSelectStroke();
    if (!selectionGestureActive() && !strokeActive)
        return;
    if (strokeActive || m_selection->gesture == epaper::selection::Gesture::Marquee
        || m_selection->gesture == epaper::selection::Gesture::Lasso) {
        if (strokeActive && m_feedSelectStroke)
            m_feedSelectStroke(QEvent::TabletRelease, QPointF());
        else
            finishMarqueeOrLasso();
        return;
    }
    if (m_selection->gesture == epaper::selection::Gesture::Move
        || m_selection->gesture == epaper::selection::Gesture::Resize) {
        commitLiveManip();
        return;
    }
    m_selection->gesture = epaper::selection::Gesture::None;
}

void SelectionManipController::beginHandleDrag(int handleIndex, double wx, double wy)
{
    using namespace epaper::document;
    if (!m_doc || !m_selection || !m_tool || !m_chrome)
        return;
    const ResizeHandle handle = epaper::manip::handleFromIndex(handleIndex);
    if (handle == ResizeHandle::None)
        return;
    const DocNode *selected = m_doc->document().find(m_selection->pickableId);
    if (!selected || !descriptorFor(selected->kind).has(Verb::Resize))
        return;
    SmartBounds wb;
    if (!boundsOf(*selected, wb))
        return;
    if (!m_doc->lodOkPanel(wb)) {
        const QSizeF host = m_hostSize ? m_hostSize() : QSizeF();
        m_chrome->showManipUnavailable(
            wb, *m_doc, host.width(), host.height(),
            [this](const QRectF &r) { m_tool->damageChrome(r); }, [this]() { m_tool->emitChromeChanged(); });
        return;
    }
    if (m_finger)
        m_finger->gesture = epaper::fingergesture::Kind::Resize;
    startLiveManip(selected, handle, wx, wy);
}

void SelectionManipController::applyDragWorld(double wx, double wy)
{
    if (!m_manip || !m_manip->active || !m_doc)
        return;
    epaper::UiStallSection stall("applyDragWorld");
    const bool previewDue =
        !m_ghostClock || !m_ghostClock->isValid()
        || m_ghostClock->elapsed() >= kGhostMinIntervalMs;
    const epaper::document::DocNode *n = m_doc->document().find(m_manip->nodeId);
    const std::string mode = n ? n->inkScaleMode : std::string("withBounds");
    applyManipIntent(m_manip->apply({wx, wy}, mode, previewDue));
    if (previewDue && m_ghostClock)
        m_ghostClock->restart();
}

void SelectionManipController::commitLiveManip()
{
    if (!m_manip || !m_manip->active || !m_selection)
        return;
    const std::string id = m_manip->nodeId;
    const epaper::manip::ManipResult r = m_manip->commit();
    m_selection->gesture = epaper::selection::Gesture::None;
    m_selection->pickableId = id;
    if (m_selection->ids.empty())
        m_selection->ids.push_back(id);
    applyManipIntent(r, /*restoreOrigin=*/!r.moved);
    m_manip->reset();
    if (m_liveDirtyPrev && m_liveDirtyPrev())
        *m_liveDirtyPrev() = QRectF();
}

void SelectionManipController::abortFingerManip()
{
    if (!m_selection || !m_manip)
        return;
    if (m_manip->active) {
        const std::string id = m_manip->nodeId;
        const epaper::manip::ManipResult r = m_manip->abort();
        m_selection->gesture = epaper::selection::Gesture::None;
        m_selection->pickableId = id;
        applyManipIntent(r, /*restoreOrigin=*/true);
        m_manip->reset();
    } else {
        m_selection->gesture = epaper::selection::Gesture::None;
    }
    if (m_liveDirtyPrev && m_liveDirtyPrev())
        *m_liveDirtyPrev() = QRectF();
    if (m_finger)
        m_finger->gesture = epaper::fingergesture::Kind::None;
}

void SelectionManipController::encloseSelection()
{
    if (!m_doc || !m_selection || !m_tool || !m_chrome)
        return;
    if (!m_chrome->state().encloseVisible)
        return;
    QString refuse;
    if (!m_doc->encloseSelection(m_selection->ids, &refuse)) {
        m_chrome->state().encloseRefuseReason = refuse;
        m_tool->emitChromeChanged();
        m_tool->damageChrome(m_chrome->state().selectionChromeDirty);
        return;
    }
    clearSelection();
    m_chrome->state().encloseVisible = false;
    m_chrome->state().encloseCtaRect = QRectF();
    m_chrome->state().encloseRefuseReason.clear();
    m_selection->gesture = epaper::selection::Gesture::None;
    m_tool->requestChromeRefresh();
}

void SelectionManipController::tapModeChip()
{
    if (!m_doc || !m_selection || !m_tool || !m_chrome)
        return;
    const epaper::document::DocNode *selected =
        m_doc->document().find(m_selection->pickableId);
    if (!selected)
        return;
    epaper::document::SmartBounds wb;
    if (epaper::document::boundsOf(*selected, wb) && !m_doc->lodOkPanel(wb)) {
        const QSizeF host = m_hostSize ? m_hostSize() : QSizeF();
        m_chrome->showManipUnavailable(
            wb, *m_doc, host.width(), host.height(),
            [this](const QRectF &r) { m_tool->damageChrome(r); }, [this]() { m_tool->emitChromeChanged(); });
        return;
    }
    m_doc->toggleInkScaleMode(m_selection->pickableId);
    m_tool->requestChromeRefresh();
}

void SelectionManipController::beginMarqueeOrLasso(const QPointF &canvasPos)
{
    if (!m_makeStrokeHost || !m_setSelectStroke || !m_feedSelectStroke || !m_doc)
        return;
    m_setSelectStroke(nullptr);
    if (m_doc->exclusiveTool() == QLatin1String("sel_freeform"))
        m_setSelectStroke(std::make_unique<LassoOperation>(m_makeStrokeHost()));
    else
        m_setSelectStroke(std::make_unique<MarqueeOperation>(m_makeStrokeHost()));
    m_feedSelectStroke(QEvent::TabletPress, canvasPos);
}

void SelectionManipController::finishMarqueeOrLasso()
{
    if (!m_selection || !m_doc)
        return;
    if (m_getSelectStroke && m_getSelectStroke()) {
        if (m_feedSelectStroke)
            m_feedSelectStroke(QEvent::TabletRelease, QPointF());
        return;
    }
    applySelectionIntent(m_selection->finish(
        kMinMarqueeGesture, m_doc->document(),
        [this](double px, double py, double *wx, double *wy) {
            const auto w = m_doc->panelToWorld(px, py);
            *wx = w.x;
            *wy = w.y;
        }));
}

void SelectionManipController::startLiveManip(const epaper::document::DocNode *subject,
                                              epaper::document::ResizeHandle handle, double wx,
                                              double wy)
{
    using namespace epaper::document;
    if (!m_selection || !m_manip || !m_doc)
        return;
    m_selection->setIds({subject->id});
    m_selection->gesture = handle == ResizeHandle::None ? epaper::selection::Gesture::Move
                                                        : epaper::selection::Gesture::Resize;
    const epaper::manip::ManipResult r =
        m_manip->begin(subject->id, handle, {wx, wy}, subject->transform, subject->smartBounds);
    if (m_ghostClock)
        m_ghostClock->invalidate();
    if (m_liveDirtyPrev && m_liveDirtyPrev())
        *m_liveDirtyPrev() = QRectF();
    if (m_originPanelRect && m_originPanelRect()) {
        SmartBounds originWorld;
        if (boundsOf(*subject, originWorld))
            *m_originPanelRect() = m_doc->worldBoundsToPanel(originWorld).adjusted(-8, -8, 8, 8);
        else
            *m_originPanelRect() = QRectF();
    }
    m_doc->setLiveManipSuppressIds(subject->id);
    applyManipIntent(r);
}

} // namespace tools
} // namespace epaper
