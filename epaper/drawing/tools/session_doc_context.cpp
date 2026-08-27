#include "session_doc_context.hpp"

#include "../canvas_session.h"
#include "../tabletcanvasitem.h"
#include "debug/debug_log_format.hpp"
#include "document/capability.hpp"
#include "document/connector_warp.hpp"
#include "document/hand_touch.hpp"
#include "document/manipulate.hpp"
#include "document/surround_create.hpp"
#include "rendering/rendering.hpp"

#include <QDebug>
#include <unordered_set>

namespace epaper {
namespace tools {

SessionDocContext::SessionDocContext(CanvasSession *session, TabletCanvasItem *surface)
    : m_session(session)
    , m_surface(surface)
{
}

epaper::document::DeviceDocument &SessionDocContext::document()
{
    return m_session->document;
}

const epaper::document::DeviceDocument &SessionDocContext::document() const
{
    return m_session->document;
}

void SessionDocContext::beginGesture()
{
    document().beginGesture();
}

void SessionDocContext::abortGesture()
{
    document().abortGesture();
}

void SessionDocContext::applyLiveSmartGeometry(const std::string &nodeId,
                                               const epaper::document::SmartTransform &t,
                                               const epaper::document::SmartBounds &b)
{
    document().applyLiveSmartGeometry(nodeId, t, b);
}

void SessionDocContext::previewManipulationFrame()
{
    document().previewManipulationFrame();
}

void SessionDocContext::commitSetSmartTransform(const std::string &opId, const std::string &nodeId,
                                                const epaper::document::SmartTransform &t,
                                                const epaper::document::SmartBounds *bounds)
{
    document().commitOp(epaper::document::makeSetSmartTransformOp(opId, nodeId, t, bounds));
}

void SessionDocContext::refreshConnectorsBoundTo(const std::string &nodeId)
{
    epaper::document::refreshConnectorsBoundTo(document(), nodeId);
}

void SessionDocContext::refreshAllConnectorWarps()
{
    epaper::document::refreshAllConnectorWarps(document());
}

void SessionDocContext::notifyHistory()
{
    if (m_surface)
        m_surface->notifyHistory();
}

void SessionDocContext::noteDocumentMutated()
{
    if (m_session)
        m_session->noteDocumentMutated();
}

void SessionDocContext::flushWire()
{
    if (m_surface)
        m_surface->flushWire();
}

void SessionDocContext::clearLiveManipSuppressIds()
{
    if (m_session)
        m_session->clearLiveManipSuppressIds();
}

QString SessionDocContext::exclusiveTool() const
{
    return m_session ? m_session->exclusiveTool() : QStringLiteral("pen");
}

void SessionDocContext::setExclusiveTool(const QString &id)
{
    if (m_session)
        m_session->setExclusiveTool(id);
}

epaper::handtouch::FollowDirection SessionDocContext::followDirection() const
{
    return m_session ? epaper::handtouch::parseFollow(m_session->followDirection().toStdString())
                     : epaper::handtouch::FollowDirection::None;
}

epaper::canvasframe::CanvasFrame &SessionDocContext::frame()
{
    return m_session->frame;
}

const epaper::canvasframe::CanvasFrame &SessionDocContext::frame() const
{
    return m_session->frame;
}

epaper::canvasframe::WorldPt SessionDocContext::panelToWorld(double px, double py) const
{
    const auto w = frame().panelToWorld({px, py});
    return {w.x, w.y};
}

QPointF SessionDocContext::worldToPanel(double wx, double wy) const
{
    const auto p = frame().worldToPanel(wx, wy);
    return QPointF(p.x, p.y);
}

epaper::canvasframe::FrameUv SessionDocContext::panelToFrameUv(double px, double py) const
{
    return frame().panelToFrameUv({px, py});
}

QRectF SessionDocContext::worldBoundsToPanel(const epaper::document::SmartBounds &wb) const
{
    epaper::canvasframe::PanelPt tl, br;
    frame().worldBoundsToPanel(wb.x, wb.y, wb.width, wb.height, &tl, &br);
    return QRectF(QPointF(tl.x, tl.y), QPointF(br.x, br.y)).normalized();
}

bool SessionDocContext::lodOkPanel(const epaper::document::SmartBounds &wb) const
{
    return frame().lodOkPanel(wb.x, wb.y, wb.width, wb.height);
}

void SessionDocContext::applyCamera(const epaper::handtouch::WorldAabb &region, bool markValid)
{
    if (m_session)
        m_session->applyCamera(region, markValid);
}

epaper::handtouch::TwoFingerContacts SessionDocContext::uvPair(double ax, double ay, double bx,
                                                               double by) const
{
    const auto ua = panelToFrameUv(ax, ay);
    const auto ub = panelToFrameUv(bx, by);
    return epaper::handtouch::TwoFingerContacts{ua.u, ua.v, ub.u, ub.v};
}

void SessionDocContext::worldThroughPanOrigin(const epaper::canvasframe::WorldAabb &panOrigin,
                                              double px, double py, double *wx, double *wy) const
{
    const auto uv = panelToFrameUv(px, py);
    epaper::handtouch::mapUvToWorld(panOrigin.box(), uv.u, uv.v, wx, wy);
}

const epaper::document::DocNode *SessionDocContext::pickTopMoveTarget(double wx, double wy) const
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

bool SessionDocContext::fingerHitsBox(double wx, double wy) const
{
    const epaper::document::DocNode *hit = pickTopMoveTarget(wx, wy);
    if (!hit)
        return false;
    epaper::document::SmartBounds b;
    if (!epaper::document::boundsOf(*hit, b))
        return false;
    return lodOkPanel(b);
}

QString SessionDocContext::hitLocalSmartGroup(double wx, double wy) const
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

bool SessionDocContext::encloseSelection(const std::vector<std::string> &ids, QString *refuseReason)
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
    noteDocumentMutated();
    notifyHistory();
    flushWire();
    return true;
}

void SessionDocContext::toggleInkScaleMode(const std::string &nodeId)
{
    using namespace epaper::document;
    const DocNode *selected = document().find(nodeId);
    if (!selected || !descriptorFor(selected->kind).has(Verb::SetInkScaleMode))
        return;
    const std::string next = selected->inkScaleMode == "fixedInk" ? "withBounds" : "fixedInk";
    static int seq = 0;
    document().commitOp(
        makeSetInkScaleModeOp(std::string("ism-") + std::to_string(++seq), selected->id, next));
    noteDocumentMutated();
    notifyHistory();
    flushWire();
}

void SessionDocContext::setLiveManipSuppressIds(const std::string &nodeId)
{
    if (!m_session)
        return;
    std::unordered_set<std::string> suppress;
    epaper::render::collectManipSuppressIds(document(), nodeId, &suppress);
    m_session->setLiveManipSuppressIds(std::move(suppress));
}

QRectF SessionDocContext::boundConnectorsPanelUnion(const std::string &nodeId) const
{
    return m_surface ? m_surface->boundConnectorsPanelUnion(nodeId) : QRectF();
}

} // namespace tools
} // namespace epaper
