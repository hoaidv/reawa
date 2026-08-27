#pragma once

/**
 * SelectionManipController — selection gesture + live manip lifecycle.
 * @implements [SRS-EP-11] @implements [SRS-EP-12]
 */

#include "../finger_gesture_machine.hpp"
#include "../manip_session.hpp"
#include "../selection_session.hpp"
#include "manip_intent_applier.hpp"
#include "selection_intent_applier.hpp"
#include "session_doc_context.hpp"
#include "tool_context.hpp"

#include <QElapsedTimer>
#include <QEvent>
#include <QSizeF>
#include <QPointF>
#include <functional>
#include <memory>

namespace epaper {
namespace tools {

class InputHub;
class Operation;
class SelectionStrokeHost;
class ToolChrome;

class SelectionManipController {
public:
    void setDoc(SessionDocContext *doc) { m_doc = doc; }
    void setTool(ToolContext *tool) { m_tool = tool; }
    void setChrome(ToolChrome *chrome) { m_chrome = chrome; }
    void setHub(InputHub *hub) { m_hub = hub; }
    void setSelection(epaper::selection::SelectionSession *sel) { m_selection = sel; }
    void setManip(epaper::manip::ManipSession *manip) { m_manip = manip; }
    void setFinger(epaper::fingergesture::FingerGestureMachine *finger) { m_finger = finger; }
    void setSelApplier(SelectionIntentApplier *a) { m_selApplier = a; }
    void setManipApplier(ManipIntentApplier *a) { m_manipApplier = a; }
    void setGhostClock(QElapsedTimer *clock) { m_ghostClock = clock; }
    void setToolIntentSeq(int *seq) { m_toolIntentSeq = seq; }

    void setIsSelectionTool(std::function<bool()> fn) { m_isSelectionTool = std::move(fn); }
    void setMakeStrokeHost(std::function<SelectionStrokeHost()> fn)
    {
        m_makeStrokeHost = std::move(fn);
    }
    void setSelectStroke(std::function<Operation *()> get,
                          std::function<void(std::unique_ptr<Operation>)> set)
    {
        m_getSelectStroke = std::move(get);
        m_setSelectStroke = std::move(set);
    }
    void setFeedSelectStroke(
        std::function<void(QEvent::Type, const QPointF &)> fn)
    {
        m_feedSelectStroke = std::move(fn);
    }
    void setRefreshHitTargets(std::function<void()> fn) { m_refreshHitTargets = std::move(fn); }
    void setOriginPanelRect(std::function<QRectF *()> fn) { m_originPanelRect = std::move(fn); }
    void setLiveDirtyPrev(std::function<QRectF *()> fn) { m_liveDirtyPrev = std::move(fn); }
    void setHostSize(std::function<QSizeF()> fn) { m_hostSize = std::move(fn); }

    void clearSelection();
    void onDocumentOrCameraChanged();
    bool beginMoveFromPanel(const QPointF &panel, bool armSelFreeform);
    bool tryBeginHandleAtPanel(const QPointF &panel, double hitDu);
    void beginSelectionGesture(const QPointF &canvasPos);
    void updateSelectionGesture(const QPointF &canvasPos);
    void endSelectionGesture();
    void beginHandleDrag(int handleIndex, double wx, double wy);
    void applyDragWorld(double wx, double wy);
    void commitLiveManip();
    void abortFingerManip();
    void encloseSelection();
    void tapModeChip();

    static constexpr double kMinMarqueeGesture = 8.0;
    static constexpr qint64 kGhostMinIntervalMs = 200;

private:
    void beginMarqueeOrLasso(const QPointF &canvasPos);
    void finishMarqueeOrLasso();
    void startLiveManip(const epaper::document::DocNode *subject,
                        epaper::document::ResizeHandle handle, double wx, double wy);
    bool selectionGestureActive() const;
    void applySelectionIntent(const epaper::selection::SelectionResult &r);
    void applyManipIntent(const epaper::manip::ManipResult &r, bool restoreOrigin = false);

    SessionDocContext *m_doc = nullptr;
    ToolContext *m_tool = nullptr;
    ToolChrome *m_chrome = nullptr;
    InputHub *m_hub = nullptr;
    epaper::selection::SelectionSession *m_selection = nullptr;
    epaper::manip::ManipSession *m_manip = nullptr;
    epaper::fingergesture::FingerGestureMachine *m_finger = nullptr;
    SelectionIntentApplier *m_selApplier = nullptr;
    ManipIntentApplier *m_manipApplier = nullptr;
    QElapsedTimer *m_ghostClock = nullptr;
    int *m_toolIntentSeq = nullptr;

    std::function<bool()> m_isSelectionTool;
    std::function<SelectionStrokeHost()> m_makeStrokeHost;
    std::function<Operation *()> m_getSelectStroke;
    std::function<void(std::unique_ptr<Operation>)> m_setSelectStroke;
    std::function<void(QEvent::Type, const QPointF &)> m_feedSelectStroke;
    std::function<void()> m_refreshHitTargets;
    std::function<QRectF *()> m_originPanelRect;
    std::function<QRectF *()> m_liveDirtyPrev;
    std::function<QSizeF()> m_hostSize;
};

} // namespace tools
} // namespace epaper
