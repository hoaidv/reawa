#pragma once

/**
 * Shared live move/resize gesture — used by MoveOperation and ResizeOperation.
 * Not a Host bag: Ops own an instance; it dies on unlock.
 * @implements [SRS-EP-11]
 */

#include "manip_session.hpp"
#include "debug/ui_stall.hpp"
#include "document/capability.hpp"
#include "document/manipulate.hpp"
#include "host_caps.hpp"
#include "selection_context.hpp"

#include <QElapsedTimer>
#include <QRectF>
#include <string>

namespace epaper {
namespace tools {

class TransformGesture {
public:
    static constexpr qint64 kGhostMinIntervalMs = 200;

    bool active() const { return m_manip.active; }
    bool resizing() const { return m_manip.resizing(); }
    const std::string &nodeId() const { return m_manip.nodeId; }

    void begin(HostCaps *caps, const epaper::document::DocNode *subject,
               epaper::document::ResizeHandle handle, epaper::canvasframe::WorldPt world)
    {
        if (!caps || !caps->doc || !caps->toolUi || !caps->selection || !subject)
            return;
        caps->selection->setIds({subject->id});
        caps->selection->setPhase(SelectionPhase::Transforming);
        m_manip.begin(subject->id, handle, world, subject->transform, subject->smartBounds);
        m_ghost.invalidate();
        epaper::document::SmartBounds originWorld;
        if (epaper::document::boundsOf(*subject, originWorld))
            caps->toolUi->setOriginPanelRect(
                caps->toolUi->worldBoundsToPanel(originWorld).adjusted(-8, -8, 8, 8));
        else
            caps->toolUi->clearOriginPanelRect();
        caps->doc->setLiveManipSuppressIds(subject->id);
        caps->doc->beginGesture();
        caps->doc->refreshAllConnectorWarps();
        caps->doc->noteDocumentMutated();
        caps->toolUi->requestChromeRefresh();
        caps->toolUi->redrawLiveManip(m_manip.resizing());
        m_didMutate = true;
    }

    void apply(HostCaps *caps, epaper::canvasframe::WorldPt world)
    {
        if (!caps || !caps->doc || !caps->toolUi || !m_manip.active)
            return;
        epaper::UiStallSection stall("applyDragWorld");
        const bool previewDue = !m_ghost.isValid() || m_ghost.elapsed() >= kGhostMinIntervalMs;
        const epaper::document::DocNode *n = caps->doc->document().find(m_manip.nodeId);
        const std::string mode = n ? n->inkScaleMode : std::string("withBounds");
        m_manip.apply(world, mode, previewDue);
        caps->doc->applyLiveSmartGeometry(m_manip.nodeId, m_manip.liveT, m_manip.liveB);
        caps->doc->refreshConnectorsBoundTo(m_manip.nodeId);
        caps->doc->previewManipulationFrame();
        if (previewDue) {
            caps->toolUi->sendManipPreview(m_manip.resizing());
            caps->toolUi->redrawLiveManip(m_manip.resizing());
            m_ghost.restart();
        }
    }

    void commit(HostCaps *caps)
    {
        if (!caps || !caps->doc || !caps->toolUi || !caps->selection || !m_manip.active)
            return;
        const std::string id = m_manip.nodeId;
        const epaper::document::SmartTransform originT = m_manip.originT;
        const epaper::document::SmartBounds originB = m_manip.originB;
        const epaper::manip::ManipResult r = m_manip.commit();
        caps->selection->setPhase(SelectionPhase::Selected);
        caps->selection->setIds({id});
        if (!r.moved) {
            caps->doc->applyLiveSmartGeometry(id, originT, originB);
            caps->doc->abortGesture();
            caps->doc->clearLiveManipSuppressIds();
            caps->doc->noteDocumentMutated();
            caps->toolUi->requestChromeRefresh();
            caps->doc->notifyHistory();
        } else {
            ++m_seq;
            const std::string opId = std::string("sst-") + std::to_string(m_seq);
            const epaper::document::SmartBounds liveB = m_manip.liveB;
            const epaper::document::SmartBounds *bptr = r.resized ? &liveB : nullptr;
            caps->doc->commitSetSmartTransform(opId, id, m_manip.liveT, bptr);
            caps->toolUi->clearOriginPanelRect();
            caps->doc->clearLiveManipSuppressIds();
            caps->doc->refreshAllConnectorWarps();
            caps->doc->noteDocumentMutated();
            caps->toolUi->requestChromeRefresh();
            caps->doc->notifyHistory();
            caps->doc->flushWire();
        }
        m_manip.reset();
    }

    void abort(HostCaps *caps)
    {
        if (!caps || !caps->doc || !caps->toolUi || !caps->selection)
            return;
        if (m_manip.active) {
            const std::string id = m_manip.nodeId;
            const epaper::document::SmartTransform originT = m_manip.originT;
            const epaper::document::SmartBounds originB = m_manip.originB;
            m_manip.abort();
            caps->doc->applyLiveSmartGeometry(id, originT, originB);
            caps->doc->refreshConnectorsBoundTo(id);
            caps->doc->abortGesture();
            caps->doc->refreshAllConnectorWarps();
            caps->doc->clearLiveManipSuppressIds();
            caps->doc->noteDocumentMutated();
            caps->selection->setPhase(caps->selection->ids().empty() ? SelectionPhase::Idle
                                                                     : SelectionPhase::Selected);
            caps->toolUi->requestChromeRefresh();
            m_manip.reset();
        } else if (caps->selection->phase() == SelectionPhase::Transforming) {
            caps->selection->setPhase(caps->selection->ids().empty() ? SelectionPhase::Idle
                                                                     : SelectionPhase::Selected);
        }
        caps->toolUi->clearOriginPanelRect();
    }

    bool didMutateSelection() const { return m_didMutate; }
    void resetMutate() { m_didMutate = false; }

private:
    epaper::manip::ManipSession m_manip;
    QElapsedTimer m_ghost;
    int m_seq = 0;
    bool m_didMutate = false;
};

} // namespace tools
} // namespace epaper
