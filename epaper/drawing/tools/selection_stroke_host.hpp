#pragma once

/**
 * Shared host deps for Marquee/Lasso Operations (Phase 2).
 * @implements [SRS-EP-11]
 */

#include "../selection_session.hpp"

#include <functional>

namespace epaper {
namespace document {
class DeviceDocument;
}

namespace tools {

struct SelectionStrokeHost {
    epaper::selection::SelectionSession *session = nullptr;
    std::function<void(const epaper::selection::SelectionResult &)> applyIntent;
    std::function<const epaper::document::DeviceDocument &()> document;
    epaper::selection::PanelToWorldFn panelToWorld;
    double minGesture = 8.0;
};

} // namespace tools
} // namespace epaper
