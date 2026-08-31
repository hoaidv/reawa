#pragma once

/**
 * Shared live move/resize gesture — used by MoveOperation and ResizeOperation.
 * Not a Host bag: Ops own an instance; it dies on unlock.
 * @implements [SRS-EP-11]
 */

#include "transform_session.hpp"
#include "debug/ui_stall.hpp"
#include "document/manipulate.hpp"
#include "document/operations/set_smart_transform_edit.hpp"
#include "../host_caps.hpp"
#include "../contexts/doc_context.hpp"
#include "../contexts/selection_context.hpp"
#include "../contexts/tool_context.hpp"
#include "../ui/selection_overlay.hpp"

#include <QElapsedTimer>
#include <QRectF>
#include <string>

namespace epaper {
namespace tools {

class TransformGesture {
public:
    static constexpr qint64 kGhostMinIntervalMs = 200;

    bool active() const { return m_live.active; }
    bool resizing() const { return m_live.resizing(); }
    const std::string &nodeId() const { return m_live.nodeId; }

    void begin(HostCaps *caps, const epaper::document::DocNode *subject,
               epaper::document::ResizeHandle handle, epaper::canvasframe::WorldPt world)
    {
        if (!caps || !caps->doc || !caps->toolUi || !caps->selection || !subject)
            return;
        caps->selection->setIds({subject->id});
        caps->selection->setPhase(SelectionPhase::Transforming);
        m_live.begin(subject->id, handle, world, subject->transform, subject->smartBounds);
        m_ghost.invalidate();
        if (caps->overlay) {
            epaper::document::SmartBounds originWorld;
            if (epaper::document::boundsOf(*subject, originWorld))
                caps->overlay->setOriginPanelRect(
                    caps->toolUi->worldBoundsToPanel(originWorld).adjusted(-8, -8, 8, 8));
            else
                caps->overlay->clearOriginPanelRect();
        }
        caps->doc->setLiveManipSuppressIds(subject->id);
        caps->doc->beginGesture();
        caps->doc->refreshAllConnectorWarps();
        caps->doc->noteDocumentMutated();
        caps->toolUi->refreshChrome();
        if (caps->overlay)
            caps->overlay->redrawLiveManip(*caps, m_live.resizing());
        m_didMutate = true;
    }

    void apply(HostCaps *caps, epaper::canvasframe::WorldPt world)
    {
        if (!caps || !caps->doc || !caps->toolUi || !m_live.active)
            return;
        epaper::UiStallSection stall("applyDragWorld");
        const bool previewDue = !m_ghost.isValid() || m_ghost.elapsed() >= kGhostMinIntervalMs;
        const epaper::document::DocNode *n = caps->doc->document().find(m_live.nodeId);
        const std::string mode = n ? n->inkScaleMode : std::string("withBounds");
        m_live.apply(world, mode);
        caps->doc->applyLiveSmartGeometry(m_live.nodeId, m_live.liveT, m_live.liveB);
        caps->doc->refreshConnectorsBoundTo(m_live.nodeId);
        caps->doc->previewManipulationFrame();
        if (caps->overlay)
            caps->overlay->redrawLiveManip(*caps, m_live.resizing());
        if (previewDue && caps->selection) {
            const auto &id = caps->selection->pickableId();
            const epaper::document::DocNode *cur = caps->doc->document().find(id);
            if (cur) {
                const epaper::document::SmartBounds *bptr =
                    m_live.resizing() ? &cur->smartBounds : nullptr;
                caps->doc->publishManipPreview(cur->id, cur->transform, bptr);
            }
            m_ghost.restart();
        }
    }

    void commit(HostCaps *caps)
    {
        if (!caps || !caps->doc || !caps->toolUi || !caps->selection || !m_live.active)
            return;
        const std::string id = m_live.nodeId;
        const epaper::document::SmartTransform originT = m_live.originT;
        const epaper::document::SmartBounds originB = m_live.originB;
        const TransformResult r = m_live.commit();
        caps->selection->setPhase(SelectionPhase::Selected);
        caps->selection->setIds({id});
        if (!r.moved) {
            caps->doc->applyLiveSmartGeometry(id, originT, originB);
            caps->doc->abortGesture();
            caps->doc->clearLiveManipSuppressIds();
            caps->doc->noteDocumentMutated();
            caps->toolUi->refreshChrome();
            caps->doc->notifyHistory();
        } else {
            // MoveOperation and ResizeOperation each own a TransformGesture.
            // A per-instance seq reused sst-1 across tools, so resize after
            // move was dropped as duplicate_opId while live geometry stayed.
            static int seq = 0;
            const std::string opId = std::string("sst-") + std::to_string(++seq);
            const epaper::document::SmartBounds liveB = m_live.liveB;
            epaper::document::SetSmartTransformEdit edit(
                opId, id, originT, originB, m_live.liveT, liveB, true);
            caps->doc->applyEdit(edit);
            if (caps->overlay)
                caps->overlay->clearOriginPanelRect();
            caps->doc->clearLiveManipSuppressIds();
            caps->doc->refreshAllConnectorWarps();
            caps->doc->noteDocumentMutated();
            caps->toolUi->refreshChrome();
            caps->doc->notifyHistory();
            caps->doc->flushWire();
        }
        m_live.reset();
    }

    void abort(HostCaps *caps)
    {
        if (!caps || !caps->doc || !caps->toolUi || !caps->selection)
            return;
        if (m_live.active) {
            const std::string id = m_live.nodeId;
            const epaper::document::SmartTransform originT = m_live.originT;
            const epaper::document::SmartBounds originB = m_live.originB;
            caps->doc->applyLiveSmartGeometry(id, originT, originB);
            caps->doc->refreshConnectorsBoundTo(id);
            caps->doc->abortGesture();
            caps->doc->refreshAllConnectorWarps();
            caps->doc->clearLiveManipSuppressIds();
            caps->doc->noteDocumentMutated();
            caps->selection->setPhase(caps->selection->ids().empty() ? SelectionPhase::Idle
                                                                     : SelectionPhase::Selected);
            caps->toolUi->refreshChrome();
            m_live.reset();
        } else if (caps->selection->phase() == SelectionPhase::Transforming) {
            caps->selection->setPhase(caps->selection->ids().empty() ? SelectionPhase::Idle
                                                                     : SelectionPhase::Selected);
        }
        if (caps->overlay)
            caps->overlay->clearOriginPanelRect();
    }

    bool didMutateSelection() const { return m_didMutate; }
    void resetMutate() { m_didMutate = false; }

private:
    TransformSession m_live;
    QElapsedTimer m_ghost;
    bool m_didMutate = false;
};

} // namespace tools
} // namespace epaper
