#pragma once

/**
 * DocContext — document model access + gesture/command helpers (ADR-0033).
 * @implements [SRS-EP-07] @implements [SRS-EP-11]
 */

#include "document/device_document.hpp"
#include "document/manipulate.hpp"

#include <QString>
#include <QRectF>
#include <QStringList>

#include <string>
#include <vector>

namespace epaper {
namespace tools {

class DocContext {
public:
    virtual ~DocContext() = default;
    virtual epaper::document::DeviceDocument &document() = 0;
    virtual const epaper::document::DeviceDocument &document() const = 0;

    virtual void beginGesture() = 0;
    virtual void abortGesture() = 0;
    virtual void applyLiveSmartGeometry(const std::string &nodeId,
                                        const epaper::document::SmartTransform &t,
                                        const epaper::document::SmartBounds &b) = 0;
    virtual void previewManipulationFrame() = 0;
    virtual void commitSetSmartTransform(const std::string &opId, const std::string &nodeId,
                                         const epaper::document::SmartTransform &t,
                                         const epaper::document::SmartBounds *bounds) = 0;
    virtual epaper::document::ApplyResult applyEdit(epaper::document::DocEdit &edit) = 0;
    virtual void refreshConnectorsBoundTo(const std::string &nodeId) = 0;
    virtual void refreshAllConnectorWarps() = 0;

    virtual void notifyHistory() = 0;
    virtual void noteDocumentMutated() = 0;
    virtual void noteDocumentDirty(const QRectF &panelDirty) = 0;
    virtual void flushWire() = 0;
    virtual void clearLiveManipSuppressIds() = 0;
    virtual void setLiveManipSuppressIds(const std::string &nodeId) = 0;
    /** Panel union of connectors bound to @p nodeId (origin punch / settle dirty). */
    virtual QRectF boundConnectorsPanelUnion(const std::string &nodeId) const = 0;

    virtual const epaper::document::DocNode *hitMoveTarget(double wx, double wy) const = 0;
    virtual bool fingerHitsBox(double wx, double wy) const = 0;
    virtual std::string hitSelectTarget(double wx, double wy) const = 0;

    virtual bool encloseSelection(const std::vector<std::string> &ids, QString *refuseReason) = 0;
    virtual void toggleInkScaleMode(const std::string &nodeId) = 0;

    virtual QString exclusiveTool() const = 0;
    virtual void publishManipPreview(const std::string &nodeId,
                                     const epaper::document::SmartTransform &liveT,
                                     const epaper::document::SmartBounds *liveB) = 0;
    virtual void setInteractionDebug(const std::string &line) = 0;
};

} // namespace tools
} // namespace epaper
