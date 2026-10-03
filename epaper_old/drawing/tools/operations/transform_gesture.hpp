#pragma once

/**
 * Shared live move/resize gesture — used by MoveOperation and ResizeOperation.
 * Not a Host bag: Ops own an instance; it dies on unlock.
 * @implements [SRS-EP-11]
 */

#include "transform_session.hpp"
#include "debug/ui_stall.hpp"
#include "document/manipulate.hpp"
#include "document/nested_inkbox.hpp"
#include "document/operations/compound_edit.hpp"
#include "document/operations/reparent_edit.hpp"
#include "document/operations/set_smart_transform_edit.hpp"
#include "../host_caps.hpp"
#include "../contexts/doc_context.hpp"
#include "../contexts/selection_context.hpp"
#include "../contexts/tool_context.hpp"
#include "../ui/selection_overlay.hpp"

#include <QElapsedTimer>
#include <QRectF>
#include <limits>
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
        double lx = world.x;
        double ly = world.y;
        epaper::document::worldToAncestorContent(caps->doc->document(), subject->id, world.x, world.y,
                                                 &lx, &ly);
        m_live.begin(subject->id, handle, {lx, ly}, subject->transform, subject->smartBounds);
        m_ghost.invalidate();
        if (caps->overlay) {
            epaper::document::SmartBounds originWorld;
            if (epaper::document::composedBoundsOf(caps->doc->document(), *subject, originWorld)) {
                m_originPanel =
                    caps->toolUi->worldBoundsToPanel(originWorld).adjusted(-8, -8, 8, 8);
                caps->overlay->setOriginPanelRect(m_originPanel);
            } else {
                caps->overlay->clearOriginPanelRect();
                m_originPanel = QRectF();
            }
        }
        caps->doc->setLiveManipSuppressIds(subject->id);
        caps->doc->beginGesture();
        caps->doc->refreshAllConnectorWarps();
        // Origin punch must include bound connector spines (BR-B19). Box AABB
        // alone leaves the middle of the connector on TabletCanvas.
        const QRectF originConn = caps->doc->boundConnectorsPanelUnion(subject->id);
        if (!originConn.isEmpty())
            m_originPanel = m_originPanel.isEmpty() ? originConn : m_originPanel.united(originConn);
        if (m_originPanel.isEmpty())
            caps->doc->noteDocumentMutated();
        else
            caps->doc->noteDocumentDirty(m_originPanel);
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
        double lx = world.x;
        double ly = world.y;
        epaper::document::worldToAncestorContent(caps->doc->document(), m_live.nodeId, world.x, world.y,
                                                 &lx, &ly);
        m_live.apply({lx, ly}, mode);
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
        const bool wasResize = m_live.resizing();
        const TransformResult r = m_live.commit();
        caps->selection->setPhase(SelectionPhase::Selected);
        caps->selection->setIds({id});
        QRectF live;
        if (const epaper::document::DocNode *n = caps->doc->document().find(id)) {
            epaper::document::SmartBounds b;
            if (epaper::document::composedBoundsOf(caps->doc->document(), *n, b))
                live = caps->toolUi->worldBoundsToPanel(b).adjusted(-8, -8, 8, 8);
        }
        const QRectF liveConn = caps->doc->boundConnectorsPanelUnion(id);
        if (!liveConn.isEmpty())
            live = live.isEmpty() ? liveConn : live.united(liveConn);
        const QRectF dirty =
            m_originPanel.isEmpty() ? live : (live.isEmpty() ? m_originPanel : m_originPanel.united(live));
        auto punch = [&]() {
            if (dirty.isEmpty())
                caps->doc->noteDocumentMutated();
            else
                caps->doc->noteDocumentDirty(dirty);
        };
        if (!r.moved) {
            caps->doc->applyLiveSmartGeometry(id, originT, originB);
            caps->doc->abortGesture();
            caps->doc->clearLiveManipSuppressIds();
            punch();
            caps->toolUi->refreshChrome();
            caps->doc->notifyHistory();
        } else {
            // MoveOperation and ResizeOperation each own a TransformGesture.
            // A per-instance seq reused sst-1 across tools, so resize after
            // move was dropped as duplicate_opId while live geometry stayed.
            static int seq = 0;
            const std::string opId = std::string("sst-") + std::to_string(++seq);
            const epaper::document::SmartBounds liveB = m_live.liveB;
            epaper::document::SmartTransform toT = m_live.liveT;
            std::string newParent;
            bool reparent = false;
            if (!wasResize) {
                newParent = epaper::document::chooseMoveParentId(caps->doc->document(), id);
                epaper::document::DeviceDocument::NodePlace pl;
                if (caps->doc->document().findPlace(id, &pl) && newParent != pl.parentId) {
                    reparent = true;
                    epaper::document::DocNode tmp;
                    if (const epaper::document::DocNode *cur = caps->doc->document().find(id))
                        tmp = *cur;
                    tmp.transform = toT;
                    epaper::document::remapOwnIntoParentCtx(
                        tmp, epaper::document::ancestorContentContext(caps->doc->document(), id),
                        epaper::document::parentContentContext(caps->doc->document(), newParent));
                    toT = tmp.transform;
                }
            }
            epaper::document::SetSmartTransformEdit sst(opId, id, originT, originB, toT, liveB,
                                                        true);
            if (reparent) {
                epaper::document::CompoundEdit compound;
                compound.setId(opId);
                compound.addPart(sst.clone());
                auto rp = std::make_unique<epaper::document::ReparentEdit>();
                rp->setId(opId);
                rp->setNodeId(id);
                rp->setNewParentId(newParent);
                rp->setIndex(std::numeric_limits<int>::max());
                compound.addPart(std::move(rp));
                caps->doc->applyEdit(compound);
            } else {
                caps->doc->applyEdit(sst);
            }
            if (caps->overlay)
                caps->overlay->clearOriginPanelRect();
            caps->doc->clearLiveManipSuppressIds();
            caps->doc->refreshAllConnectorWarps();
            const QRectF settledConn = caps->doc->boundConnectorsPanelUnion(id);
            const QRectF settledDirty = settledConn.isEmpty()
                ? dirty
                : (dirty.isEmpty() ? settledConn : dirty.united(settledConn));
            if (settledDirty.isEmpty())
                caps->doc->noteDocumentMutated();
            else
                caps->doc->noteDocumentDirty(settledDirty);
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
            QRectF live;
            if (const epaper::document::DocNode *n = caps->doc->document().find(id)) {
                epaper::document::SmartBounds b;
                if (epaper::document::composedBoundsOf(caps->doc->document(), *n, b))
                    live = caps->toolUi->worldBoundsToPanel(b).adjusted(-8, -8, 8, 8);
            }
            const QRectF liveConn = caps->doc->boundConnectorsPanelUnion(id);
            if (!liveConn.isEmpty())
                live = live.isEmpty() ? liveConn : live.united(liveConn);
            const QRectF dirty = m_originPanel.isEmpty()
                ? live
                : (live.isEmpty() ? m_originPanel : m_originPanel.united(live));
            if (dirty.isEmpty())
                caps->doc->noteDocumentMutated();
            else
                caps->doc->noteDocumentDirty(dirty);
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
    QRectF m_originPanel;
    bool m_didMutate = false;
};

} // namespace tools
} // namespace epaper
