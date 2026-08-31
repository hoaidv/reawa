#pragma once

/**
 * SessionDocContext — DocContext over CanvasSession + Tablet Surface.
 * @implements [SRS-EP-07]
 */

#include "../../canvas_frame.hpp"
#include "../viewport.hpp"
#include "doc_context.hpp"
#include "document/hand_touch.hpp"

#include <QPointF>
#include <QRectF>
#include <QString>
#include <vector>

#include "../../canvas_session.h"
#include "../../tabletcanvasitem.h"
#include "debug/debug_log_format.hpp"
#include "document/capability.hpp"
#include "document/connector_warp.hpp"
#include "document/manipulate.hpp"
#include "document/surround_create.hpp"
#include "rendering/rendering.hpp"

#include <QDebug>
#include <unordered_set>


class CanvasSession;
class TabletCanvasItem;

namespace epaper {
namespace document {
struct DocNode;
struct SmartBounds;
} // namespace document
namespace tools {

class SessionDocContext final : public DocContext, public Viewport {
public:
    SessionDocContext(CanvasSession *session, TabletCanvasItem *surface);

    epaper::document::DeviceDocument &document() override;
    const epaper::document::DeviceDocument &document() const override;

    void beginGesture() override;
    void abortGesture() override;
    void applyLiveSmartGeometry(const std::string &nodeId,
                                const epaper::document::SmartTransform &t,
                                const epaper::document::SmartBounds &b) override;
    void previewManipulationFrame() override;
    void commitSetSmartTransform(const std::string &opId, const std::string &nodeId,
                                 const epaper::document::SmartTransform &t,
                                 const epaper::document::SmartBounds *bounds) override;
    epaper::document::ApplyResult applyEdit(epaper::document::DocEdit &edit) override;
    void refreshConnectorsBoundTo(const std::string &nodeId) override;
    void refreshAllConnectorWarps() override;

    void notifyHistory() override;
    void noteDocumentMutated() override;
    void noteDocumentDirty(const QRectF &panelDirty) override;
    void flushWire() override;
    void clearLiveManipSuppressIds() override;
    void setLiveManipSuppressIds(const std::string &nodeId) override;

    const epaper::document::DocNode *hitMoveTarget(double wx, double wy) const override;
    bool fingerHitsBox(double wx, double wy) const override;
    std::string hitSelectTarget(double wx, double wy) const override;

    bool encloseSelection(const std::vector<std::string> &ids, QString *refuseReason) override;
    void toggleInkScaleMode(const std::string &nodeId) override;

    QString exclusiveTool() const override;
    void publishManipPreview(const std::string &nodeId,
                             const epaper::document::SmartTransform &liveT,
                             const epaper::document::SmartBounds *liveB) override;
    void setInteractionDebug(const std::string &line) override;

    void applyCamera(const epaper::handtouch::WorldAabb &region, bool markValid) override;
    void publishViewport(bool settle) override;
    void scheduleRasterize(bool sharp) override;
    void ensureDrawingRegion() override;
    epaper::canvasframe::WorldAabb drawingRegion() const override;
    epaper::handtouch::FollowDirection follow() const override;
    epaper::handtouch::TwoFingerContacts uvPair(double ax, double ay, double bx,
                                                double by) const override;
    void worldThroughPanOrigin(const epaper::canvasframe::WorldAabb &origin, double px, double py,
                               double *wx, double *wy) const override;

    void setSession(CanvasSession *session) { m_session = session; }
    void setSurface(TabletCanvasItem *surface) { m_surface = surface; }

    CanvasSession *session() const { return m_session; }
    TabletCanvasItem *surface() const { return m_surface; }

    void setExclusiveTool(const QString &id);

    epaper::canvasframe::CanvasFrame &frame();
    const epaper::canvasframe::CanvasFrame &frame() const;
    epaper::canvasframe::WorldPt panelToWorld(double px, double py) const;
    QPointF worldToPanel(double wx, double wy) const;
    epaper::canvasframe::FrameUv panelToFrameUv(double px, double py) const;
    QRectF worldBoundsToPanel(const epaper::document::SmartBounds &wb) const;
    bool lodOkPanel(const epaper::document::SmartBounds &wb) const;

    QString hitLocalSmartGroup(double wx, double wy) const;
    QRectF boundConnectorsPanelUnion(const std::string &nodeId) const;

private:
    CanvasSession *m_session = nullptr;
    TabletCanvasItem *m_surface = nullptr;
};

} // namespace tools
} // namespace epaper

namespace epaper {
namespace tools {

inline SessionDocContext::SessionDocContext(CanvasSession *session, TabletCanvasItem *surface)
    : m_session(session)
    , m_surface(surface)
{
}

inline epaper::document::DeviceDocument &SessionDocContext::document()
{
    return m_session->document;
}

inline const epaper::document::DeviceDocument &SessionDocContext::document() const
{
    return m_session->document;
}

inline void SessionDocContext::beginGesture()
{
    document().beginGesture();
}

inline void SessionDocContext::abortGesture()
{
    document().abortGesture();
}

inline void SessionDocContext::applyLiveSmartGeometry(const std::string &nodeId,
                                               const epaper::document::SmartTransform &t,
                                               const epaper::document::SmartBounds &b)
{
    document().applyLiveSmartGeometry(nodeId, t, b);
}

inline void SessionDocContext::previewManipulationFrame()
{
    document().previewManipulationFrame();
}

inline void SessionDocContext::commitSetSmartTransform(const std::string &opId, const std::string &nodeId,
                                                const epaper::document::SmartTransform &t,
                                                const epaper::document::SmartBounds *bounds)
{
    epaper::document::SetSmartTransformEdit edit;
    edit.setId(opId);
    edit.setNodeId(nodeId);
    edit.setTo(t, bounds);
    if (const auto *n = document().find(nodeId))
        edit.setFrom(n->transform, n->smartBounds);
    document().commitEdit(edit);
}

inline epaper::document::ApplyResult SessionDocContext::applyEdit(epaper::document::DocEdit &edit)
{
    return document().commitEdit(edit);
}

inline void SessionDocContext::refreshConnectorsBoundTo(const std::string &nodeId)
{
    epaper::document::refreshConnectorsBoundTo(document(), nodeId);
}

inline void SessionDocContext::refreshAllConnectorWarps()
{
    epaper::document::refreshAllConnectorWarps(document());
}

inline void SessionDocContext::notifyHistory()
{
    if (m_surface)
        m_surface->notifyHistory();
}

inline void SessionDocContext::noteDocumentMutated()
{
    if (m_session)
        m_session->noteDocumentMutated();
}

inline void SessionDocContext::noteDocumentDirty(const QRectF &panelDirty)
{
    if (m_surface)
        m_surface->scheduleDirtyRasterize(panelDirty, true);
    if (m_session)
        m_session->noteDocumentMutated();
}

inline void SessionDocContext::flushWire()
{
    if (m_surface)
        m_surface->flushWire();
}

inline void SessionDocContext::clearLiveManipSuppressIds()
{
    if (m_session)
        m_session->clearLiveManipSuppressIds();
}

inline QString SessionDocContext::exclusiveTool() const
{
    return m_session ? m_session->exclusiveTool() : QStringLiteral("pen");
}

inline void SessionDocContext::publishManipPreview(const std::string &nodeId,
                                                   const epaper::document::SmartTransform &liveT,
                                                   const epaper::document::SmartBounds *liveB)
{
    if (m_surface)
        m_surface->publishManipPreview(nodeId, liveT, liveB);
}

inline void SessionDocContext::setInteractionDebug(const std::string &line)
{
    if (m_surface)
        m_surface->setInteractionDebug(QString::fromStdString(line));
}

inline void SessionDocContext::setExclusiveTool(const QString &id)
{
    if (m_session)
        m_session->setExclusiveTool(id);
}

inline epaper::handtouch::FollowDirection SessionDocContext::follow() const
{
    return m_session ? epaper::handtouch::parseFollow(m_session->followDirection().toStdString())
                     : epaper::handtouch::FollowDirection::None;
}

inline epaper::canvasframe::CanvasFrame &SessionDocContext::frame()
{
    return m_session->frame;
}

inline const epaper::canvasframe::CanvasFrame &SessionDocContext::frame() const
{
    return m_session->frame;
}

inline epaper::canvasframe::WorldPt SessionDocContext::panelToWorld(double px, double py) const
{
    const auto w = frame().panelToWorld({px, py});
    return {w.x, w.y};
}

inline QPointF SessionDocContext::worldToPanel(double wx, double wy) const
{
    const auto p = frame().worldToPanel(wx, wy);
    return QPointF(p.x, p.y);
}

inline epaper::canvasframe::FrameUv SessionDocContext::panelToFrameUv(double px, double py) const
{
    return frame().panelToFrameUv({px, py});
}

inline QRectF SessionDocContext::worldBoundsToPanel(const epaper::document::SmartBounds &wb) const
{
    epaper::canvasframe::PanelPt tl, br;
    frame().worldBoundsToPanel(wb.x, wb.y, wb.width, wb.height, &tl, &br);
    return QRectF(QPointF(tl.x, tl.y), QPointF(br.x, br.y)).normalized();
}

inline bool SessionDocContext::lodOkPanel(const epaper::document::SmartBounds &wb) const
{
    return frame().lodOkPanel(wb.x, wb.y, wb.width, wb.height);
}

inline void SessionDocContext::applyCamera(const epaper::handtouch::WorldAabb &region, bool markValid)
{
    if (m_session)
        m_session->applyCamera(region, markValid);
}

inline void SessionDocContext::publishViewport(bool settle)
{
    if (m_surface)
        m_surface->maybePublishLocalViewport(settle);
}

inline void SessionDocContext::scheduleRasterize(bool sharp)
{
    if (m_surface)
        m_surface->scheduleDocumentRasterize(sharp);
}

inline void SessionDocContext::ensureDrawingRegion()
{
    if (m_surface)
        m_surface->ensureLocalDrawingRegion();
}

inline epaper::canvasframe::WorldAabb SessionDocContext::drawingRegion() const
{
    if (!m_session)
        return {};
    return frame().drawingRegion;
}

inline epaper::handtouch::TwoFingerContacts SessionDocContext::uvPair(double ax, double ay, double bx,
                                                               double by) const
{
    const auto ua = panelToFrameUv(ax, ay);
    const auto ub = panelToFrameUv(bx, by);
    return epaper::handtouch::TwoFingerContacts{ua.u, ua.v, ub.u, ub.v};
}

inline void SessionDocContext::worldThroughPanOrigin(const epaper::canvasframe::WorldAabb &panOrigin,
                                              double px, double py, double *wx, double *wy) const
{
    const auto uv = panelToFrameUv(px, py);
    epaper::handtouch::mapUvToWorld(panOrigin.box(), uv.u, uv.v, wx, wy);
}

inline const epaper::document::DocNode *SessionDocContext::hitMoveTarget(double wx, double wy) const
{
    using namespace epaper::document;
    std::vector<const DocNode *> pick;
    collectPickable(document().rootChildren, pick);
    for (int i = int(pick.size()) - 1; i >= 0; --i) {
        const DocNode *n = pick[size_t(i)];
        if (!n || !descriptorFor(n->kind).has(Verb::Move))
            continue;
        SmartBounds b;
        if (!boundsOf(*n, b))
            continue;
        if (wx >= b.x && wx <= b.x + b.width && wy >= b.y && wy <= b.y + b.height)
            return n;
    }
    return nullptr;
}

inline bool SessionDocContext::fingerHitsBox(double wx, double wy) const
{
    const epaper::document::DocNode *hit = hitMoveTarget(wx, wy);
    if (!hit)
        return false;
    epaper::document::SmartBounds b;
    if (!epaper::document::boundsOf(*hit, b))
        return false;
    return lodOkPanel(b);
}

inline std::string SessionDocContext::hitSelectTarget(double wx, double wy) const
{
    using namespace epaper::document;
    std::vector<const DocNode *> pick;
    collectPickable(document().rootChildren, pick);
    for (int i = int(pick.size()) - 1; i >= 0; --i) {
        const DocNode *n = pick[size_t(i)];
        if (!n || !descriptorFor(n->kind).has(Verb::Select))
            continue;
        SmartBounds b;
        if (!boundsOf(*n, b))
            continue;
        if (wx >= b.x && wx <= b.x + b.width && wy >= b.y && wy <= b.y + b.height)
            return n->id;
    }
    return {};
}

inline QString SessionDocContext::hitLocalSmartGroup(double wx, double wy) const
{
    using namespace epaper::document;
    std::vector<const DocNode *> pick;
    collectPickable(document().rootChildren, pick);
    for (int i = int(pick.size()) - 1; i >= 0; --i) {
        const DocNode *n = pick[size_t(i)];
        if (!n || n->kind != NodeKind::SmartGroup)
            continue;
        SmartBounds b;
        if (!nodeWorldAabb(*n, b))
            continue;
        if (wx >= b.x && wx <= b.x + b.width && wy >= b.y && wy <= b.y + b.height)
            return QString::fromStdString(n->id);
    }
    return {};
}

inline bool SessionDocContext::encloseSelection(const std::vector<std::string> &ids, QString *refuseReason)
{
    using namespace epaper::document;
    const SelectionCreateResult r = createSmartGroupFromSelection(document(), ids);
    if (!r.created) {
        if (refuseReason) {
            *refuseReason = r.reason == "smartgroup_in_selection"
                ? QStringLiteral("Cannot enclose a Smart Group")
                : QStringLiteral("No surrounding stroke");
        }
        const std::string line =
            epaper::debuglog::formatEncloseLog("OrdinaryInk", r.reason, "", {});
        qInfo().noquote() << QString::fromStdString(line);
        if (m_surface)
            m_surface->setInteractionDebug(QString::fromStdString(line));
        return false;
    }
    const std::string line =
        epaper::debuglog::formatEncloseLog("Created", "", r.smartGroupId, r.childIds);
    qInfo().noquote() << QString::fromStdString(line);
    if (m_surface)
        m_surface->setInteractionDebug(QString::fromStdString(line));
    QRectF dirty;
    if (const DocNode *sg = document().find(r.smartGroupId)) {
        SmartBounds b;
        if (boundsOf(*sg, b) && m_surface)
            dirty = worldBoundsToPanel(b);
    }
    if (dirty.isEmpty())
        noteDocumentMutated();
    else
        noteDocumentDirty(dirty);
    notifyHistory();
    flushWire();
    return true;
}

inline void SessionDocContext::toggleInkScaleMode(const std::string &nodeId)
{
    using namespace epaper::document;
    const DocNode *selected = document().find(nodeId);
    if (!selected || !descriptorFor(selected->kind).has(Verb::SetInkScaleMode))
        return;
    const std::string next = selected->inkScaleMode == "fixedInk" ? "withBounds" : "fixedInk";
    static int seq = 0;
    SetInkScaleModeEdit edit(selected->id, next);
    edit.setId(std::string("ism-") + std::to_string(++seq));
    edit.setOldMode(selected->inkScaleMode);
    document().commitEdit(edit);
    QRectF dirty;
    if (const DocNode *n = document().find(nodeId)) {
        SmartBounds b;
        if (boundsOf(*n, b) && m_surface)
            dirty = worldBoundsToPanel(b);
    }
    if (dirty.isEmpty())
        noteDocumentMutated();
    else
        noteDocumentDirty(dirty);
    notifyHistory();
    flushWire();
}

inline void SessionDocContext::setLiveManipSuppressIds(const std::string &nodeId)
{
    if (!m_session)
        return;
    std::unordered_set<std::string> suppress;
    epaper::render::collectManipSuppressIds(document(), nodeId, &suppress);
    m_session->setLiveManipSuppressIds(std::move(suppress));
}

inline QRectF SessionDocContext::boundConnectorsPanelUnion(const std::string &nodeId) const
{
    return m_surface ? m_surface->boundConnectorsPanelUnion(nodeId) : QRectF();
}

} // namespace tools
} // namespace epaper
