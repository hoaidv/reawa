#pragma once

/**
 * SessionDocContext — DocContext over CanvasSession + Tablet Surface.
 * @implements [SRS-EP-07]
 */

#include "doc_context.hpp"

class CanvasSession;
class TabletCanvasItem;

namespace epaper {
namespace tools {

class SessionDocContext final : public DocContext {
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
    void refreshConnectorsBoundTo(const std::string &nodeId) override;
    void refreshAllConnectorWarps() override;

    void notifyHistory() override;
    void noteDocumentMutated() override;
    void flushWire() override;
    void clearLiveManipSuppressIds() override;

    void setSession(CanvasSession *session) { m_session = session; }
    void setSurface(TabletCanvasItem *surface) { m_surface = surface; }

private:
    CanvasSession *m_session = nullptr;
    TabletCanvasItem *m_surface = nullptr;
};

} // namespace tools
} // namespace epaper
