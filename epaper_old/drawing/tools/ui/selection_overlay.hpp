#pragma once

/**
 * SelectionOverlay — ADR-0019 ToolCanvasLayer selection state (AABB, knobs, live fill, hits).
 * Host-owned; not a member of ToolContextImpl; not inlined into ToolCanvasItem.
 * @implements [SRS-EP-12] @implements [ADR-0019] @implements [ADR-0035]
 */

#include "document/manipulate.hpp"
#include "../contexts/selection_context.hpp"
#include "../contexts/session_doc_context.hpp"
#include "../host_caps.hpp"

#include <QPointF>
#include <QRectF>
#include <QString>
#include <functional>

class QPainter;

namespace epaper {
namespace tools {

class InputHub;

struct SelectionOverlayState {
    QString encloseRefuseReason;
    QRectF selectionBoundsRect;
    int handleCount = 0;
    qreal handleSize = 16.0;
    QString manipUnavailable;
    QRectF manipUnavailableRect;
    QRectF selectionChromeDirty;
    QRectF originPanelRect;
    QRectF liveDirtyPrev;
};

class SelectionOverlay {
public:
    SelectionOverlayState &state() { return m_state; }
    const SelectionOverlayState &state() const { return m_state; }

    void refresh(SelectionContext &selection, SessionDocContext &doc, bool showKnobs);
    void paintLiveManip(QPainter *painter, SelectionContext &selection, SessionDocContext &doc);
    void paintSettled(QPainter *painter, SelectionContext &selection, SessionDocContext &doc);
    void paintLiveManip(QPainter *painter, HostCaps &caps);
    void paintSettled(QPainter *painter, HostCaps &caps);

    void redrawLiveManip(SelectionContext &selection, SessionDocContext &doc, bool resizing,
                         const std::function<void(const QRectF &)> &repaint,
                         const std::function<void()> &emitChanged);
    void redrawLiveManip(HostCaps &caps, bool resizing);
    void publishOverlayHits(InputHub &hub) const;
    void showManipUnavailable(const epaper::document::SmartBounds &wb, SessionDocContext &doc,
                              qreal hostWidth, qreal hostHeight,
                              const std::function<void(const QRectF &)> &repaint,
                              const std::function<void()> &emitChanged);
    void showManipUnavailable(HostCaps &caps, const epaper::document::SmartBounds &wb);
    void setRefuseReason(HostCaps &caps, const QString &reason);
    void resetTransientFlags();
    void clearManipUnavailable();
    void clearOriginPanelRect() { m_state.originPanelRect = QRectF(); }
    void setOriginPanelRect(const QRectF &r) { m_state.originPanelRect = r; }

private:
    SelectionOverlayState m_state;
};

} // namespace tools
} // namespace epaper
