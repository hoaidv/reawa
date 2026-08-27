#pragma once

/**
 * SessionDocContext — DocContext over CanvasSession + Tablet Surface.
 * @implements [SRS-EP-07]
 */

#include "../canvas_frame.hpp"
#include "doc_context.hpp"
#include "document/hand_touch.hpp"

#include <QPointF>
#include <QRectF>
#include <QString>
#include <vector>

class CanvasSession;
class TabletCanvasItem;

namespace epaper {
namespace document {
struct DocNode;
struct SmartBounds;
} // namespace document
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

    CanvasSession *session() const { return m_session; }
    TabletCanvasItem *surface() const { return m_surface; }

    QString exclusiveTool() const;
    void setExclusiveTool(const QString &id);
    epaper::handtouch::FollowDirection followDirection() const;

    epaper::canvasframe::CanvasFrame &frame();
    const epaper::canvasframe::CanvasFrame &frame() const;
    epaper::canvasframe::WorldPt panelToWorld(double px, double py) const;
    QPointF worldToPanel(double wx, double wy) const;
    epaper::canvasframe::FrameUv panelToFrameUv(double px, double py) const;
    QRectF worldBoundsToPanel(const epaper::document::SmartBounds &wb) const;
    bool lodOkPanel(const epaper::document::SmartBounds &wb) const;

    void applyCamera(const epaper::handtouch::WorldAabb &region, bool markValid);
    epaper::handtouch::TwoFingerContacts uvPair(double ax, double ay, double bx, double by) const;
    void worldThroughPanOrigin(const epaper::canvasframe::WorldAabb &panOrigin, double px, double py,
                               double *wx, double *wy) const;

    const epaper::document::DocNode *pickTopMoveTarget(double wx, double wy) const;
    bool fingerHitsBox(double wx, double wy) const;
    QString hitLocalSmartGroup(double wx, double wy) const;

    bool encloseSelection(const std::vector<std::string> &ids, QString *refuseReason);
    void toggleInkScaleMode(const std::string &nodeId);

    void setLiveManipSuppressIds(const std::string &nodeId);
    QRectF boundConnectorsPanelUnion(const std::string &nodeId) const;

private:
    CanvasSession *m_session = nullptr;
    TabletCanvasItem *m_surface = nullptr;
};

} // namespace tools
} // namespace epaper
