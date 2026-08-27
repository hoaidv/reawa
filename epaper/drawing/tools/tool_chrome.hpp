#pragma once

/**
 * ToolChrome — SelectionOverlay state, refresh, paint, damage (ADR-0033).
 * @implements [SRS-EP-12] @implements [ADR-0019]
 */

#include "../manip_session.hpp"
#include "../selection_session.hpp"
#include "document/manipulate.hpp"
#include "session_doc_context.hpp"

#include <QRectF>
#include <QString>

class QPainter;

namespace epaper {
namespace tools {

class InputHub;

struct ToolChromeState {
    QRectF encloseCtaRect;
    bool encloseVisible = false;
    QString encloseRefuseReason;
    QRectF selectionBoundsRect;
    int handleCount = 0;
    qreal handleSize = 16.0;
    bool modeChipVisible = false;
    QString modeChipLabel;
    QRectF modeChipRect;
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

    void refresh(epaper::selection::SelectionSession &selection, epaper::manip::ManipSession &manip,
                 SessionDocContext &doc, bool isSelectionTool);
    void damage(const QRectF &next, const std::function<void(const QRectF &)> &repaint);
    void damageSegment(const QRectF &seg, const std::function<void(const QRectF &)> &repaint);
    void syncPresence(epaper::selection::SelectionSession &selection, bool isSelectionTool,
                      const std::function<void(bool visible)> &setVisible,
                      const std::function<void(bool penWaveform)> &setStrokeWaveform);
    void paint(QPainter *painter, epaper::selection::SelectionSession &selection,
               epaper::manip::ManipSession &manip, SessionDocContext &doc, bool isSelectionTool);
    void redrawLiveManip(epaper::selection::SelectionSession &selection,
                         epaper::manip::ManipSession &manip, SessionDocContext &doc,
                         const std::function<void(const QRectF &)> &repaint,
                         const std::function<void()> &emitChanged);
    int handleIndexAtPanel(const QPointF &panel, double hitDu) const;
    void syncHitTargets(InputHub &hub) const;
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
