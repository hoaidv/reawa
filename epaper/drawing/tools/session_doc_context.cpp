#include "session_doc_context.hpp"

#include "../canvas_session.h"
#include "../tabletcanvasitem.h"
#include "document/connector_warp.hpp"

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

} // namespace tools
} // namespace epaper
