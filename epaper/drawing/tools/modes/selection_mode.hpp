#pragma once

/**
 * Selection Mode — exclusive Mode for sel_rect / sel_freeform.
 * Registers HandTouch allow-list; empty postHandling.
 * @implements [SRS-EP-04] @implements [SRS-EP-11]
 */

#include "../modifiers/hand_touch_modifier.hpp"
#include "../modifiers/hand_touch_profile.hpp"
#include "../input_hub.hpp"
#include "../mode.hpp"
#include "../operation.hpp"

namespace epaper {
namespace tools {

class SelectionMode final : public InteractionMode {
public:
    ModeId id() const override { return ModeId::Selection; }

    std::vector<OperationKind> penOperations() const override
    {
        return {OperationKind::Resize, OperationKind::Move, OperationKind::Lasso,
                OperationKind::Marquee};
    }

    void activate(HostCaps &caps, InputHub &hub, HandTouchModifier &hand) override
    {
        (void)caps;
        (void)hub;
        HandTouchProfile profile;
        profile.modeId = ModeId::Selection;
        profile.allowedOperations = {
            OperationKind::Navigation,
            OperationKind::Select,
            OperationKind::Lasso,
            OperationKind::Marquee,
            OperationKind::Move,
            OperationKind::Resize,
            OperationKind::Rotate,
        };
        profile.postHandling = {}; // none
        hand.registerProfile(profile);
    }

    void deactivate(InputHub &hub, HandTouchModifier &hand) override
    {
        (void)hub;
        hand.clearProfile(ModeId::Selection);
    }
};

} // namespace tools
} // namespace epaper
