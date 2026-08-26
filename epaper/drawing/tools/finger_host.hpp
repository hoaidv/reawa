#pragma once

/**
 * Host deps for finger Operations (Navigation, Move, Select, Resize).
 * Operations call back into ToolCanvasItem; FingerGestureMachine stays on host.
 * @implements [SRS-EP-21] @implements [SRS-EP-24]
 */

#include "../finger_gesture_machine.hpp"
#include "document/hand_touch.hpp"

#include <QPointF>
#include <functional>

namespace epaper {
namespace tools {

struct FingerHost {
    epaper::fingergesture::FingerGestureMachine *machine = nullptr;
    std::function<void(const epaper::fingergesture::FingerResult &, const QPointF &panel)>
        applyIntent;
    std::function<void(const epaper::fingergesture::FingerResult &)> applyIntentBare;
    std::function<void()> ensureLocalDrawingRegion;
    std::function<epaper::canvasframe::WorldAabb()> drawingRegion;
    std::function<epaper::canvasframe::WorldPt(QPointF)> panelToWorld;
    std::function<epaper::handtouch::TwoFingerContacts(QPointF, QPointF)> uvPair;
    std::function<epaper::handtouch::FollowDirection()> follow;
    std::function<void(QPointF, double *, double *)> worldThroughPanOrigin;
    std::function<bool()> previewDue;
    std::function<void()> markPreviewPublished;
    std::function<void()> invalidatePanClock;
    std::function<void()> restartPanClock;
    std::function<void(QPointF)> beginSelectionGesture;
    std::function<void(QPointF)> updateSelectionGesture;
    std::function<void()> endSelectionGesture;
    std::function<bool(QPointF, double)> tryBeginHandleAtPanel;
    std::function<bool()> manipActive;
};

} // namespace tools
} // namespace epaper
