#pragma once

/**
 * Interaction Mode — object with enum id + state (ADR-0033).
 * @implements [SRS-EP-04]
 */

#include "host_caps.hpp"
#include "mode_id.hpp"
#include "operation.hpp"

#include <vector>

namespace epaper {
namespace tools {

class InputHub;
class HandTouchModifier;

class InteractionMode {
public:
    virtual ~InteractionMode() = default;
    virtual ModeId id() const = 0;
    virtual void activate(HostCaps &caps, InputHub &hub, HandTouchModifier &hand) = 0;
    virtual void deactivate(InputHub &hub, HandTouchModifier &hand) = 0;
    /** Pen-down candidates (not HandTouch allow-list). Empty = none. */
    virtual std::vector<OperationKind> penOperations() const { return {}; }
};

} // namespace tools
} // namespace epaper
