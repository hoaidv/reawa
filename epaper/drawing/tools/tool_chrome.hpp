#pragma once

/**
 * ToolChrome — SelectionOverlay state, refresh, paint, damage (ADR-0033).
 * @implements [SRS-EP-12] @implements [ADR-0019]
 */

#include "document/manipulate.hpp"
#include "contexts/selection_context.hpp"
#include "contexts/session_doc_context.hpp"

#include <QPointF>
#include <QRectF>
#include <QString>
#include <functional>

class QPainter;

namespace epaper {
namespace tools {

class InputHub;

struct ToolChromeState {
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

class ToolChrome {
public:
    ToolChromeState &state() { return m_state; }
    const ToolChromeState &state() const { return m_state; }

    void refresh(SelectionContext &selection, SessionDocContext &doc, bool isSelectionTool);
    void damage(const QRectF &next, const std::function<void(const QRectF &)> &repaint);
    void damageSegment(const QRectF &seg, const std::function<void(const QRectF &)> &repaint);
    void paint(QPainter *painter, SelectionContext &selection, SessionDocContext &doc,
               bool isSelectionTool);
    void redrawLiveManip(SelectionContext &selection, SessionDocContext &doc, bool resizing,
                         const std::function<void(const QRectF &)> &repaint,
                         const std::function<void()> &emitChanged);
    void publishOverlayHits(InputHub &hub) const;
    void showManipUnavailable(const epaper::document::SmartBounds &wb, SessionDocContext &doc,
                              qreal hostWidth, qreal hostHeight,
                              const std::function<void(const QRectF &)> &repaint,
                              const std::function<void()> &emitChanged);
    void resetTransientFlags();
    void clearOriginPanelRect() { m_state.originPanelRect = QRectF(); }

private:
    ToolChromeState m_state;
    QRectF m_toolChromePrev;
};

} // namespace tools
} // namespace epaper
