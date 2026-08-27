#pragma once

/**
 * Host deps for Move/Resize Operations (ManipSession lifecycle).
 * @implements [SRS-EP-11]
 */

#include "../manip_session.hpp"
#include "../selection_session.hpp"

#include <QPointF>
#include <functional>

namespace epaper {
namespace tools {

struct ManipHost {
    epaper::manip::ManipSession *manip = nullptr;
    epaper::selection::SelectionSession *selection = nullptr;
    double penHandleHitDu = 56.0;
    double fingerHandleHitDu = 64.0;
    std::function<int(QPointF, double)> handleIndexAtPanel;
    std::function<void(int, QPointF)> beginHandleDrag;
    /** Pick + startLiveManip move; arm sel_freeform when @p armSelFreeform. Returns false if no move. */
    std::function<bool(QPointF, bool armSelFreeform)> beginMoveFromPanel;
    std::function<void(QPointF)> applyDragFromPanel;
    std::function<void()> commitTransform;
    std::function<void()> abortTransform;
};

} // namespace tools
} // namespace epaper
