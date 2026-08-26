#pragma once

/**
 * HandTouchProfile — Mode.id → allowed Operation kinds + dynamic postHandling.
 * postHandling is a callback (not an enum): Operations/Modes supply logic
 * (e.g. Pen switches to Selection only when a select/move actually selected something).
 * @implements [SRS-EP-21] @implements [SRS-EP-23]
 */

#include "host_caps.hpp"
#include "mode_id.hpp"
#include "operation.hpp"

#include <functional>
#include <vector>

namespace epaper {
namespace tools {

/** Filled by HandTouch / Operation on gesture commit before postHandling runs. */
struct HandTouchCommitInfo {
    OperationKind kind = OperationKind::None;
    bool selectionNonEmpty = false;
    bool didMutateSelection = false;
};

/**
 * Dynamic post-commit hook. Empty function = no post handling.
 * Defined by the Mode when registering (often closing over Operation outcomes).
 */
using HandTouchPostHandling =
    std::function<void(HostCaps &caps, const HandTouchCommitInfo &info)>;

struct HandTouchProfile {
    ModeId modeId = ModeId::Pen;
    std::vector<OperationKind> allowedOperations;
    HandTouchPostHandling postHandling;
};

} // namespace tools
} // namespace epaper
