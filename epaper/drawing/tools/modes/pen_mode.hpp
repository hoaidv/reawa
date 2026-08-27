#pragma once

/**
 * Pen Mode — exclusive Mode id Pen; registers HandTouch profile with dynamic postHandling.
 * @implements [SRS-EP-04]
 */

#include "../modifiers/hand_touch_modifier.hpp"
#include "../modifiers/hand_touch_profile.hpp"
#include "../input_hub.hpp"
#include "../mode.hpp"
#include "../operation.hpp"

#include <QString>

namespace epaper {
namespace tools {

class PenMode final : public InteractionMode {
public:
    ModeId id() const override { return ModeId::Pen; }

    std::vector<OperationKind> penOperations() const override
    {
        return {OperationKind::InkStroke};
    }

    void activate(HostCaps &caps, InputHub &hub, HandTouchModifier &hand) override
    {
        (void)hub;
        m_caps = &caps;
        HandTouchProfile profile;
        profile.modeId = ModeId::Pen;
        profile.allowedOperations = {
            OperationKind::Navigation,
            OperationKind::Select,
            OperationKind::Move,
        };
        // Dynamic: only switch when a finger select/move actually mutated selection.
        profile.postHandling = [](HostCaps &c, const HandTouchCommitInfo &info) {
            if (!info.didMutateSelection || !info.selectionNonEmpty)
                return;
            if (c.setExclusiveTool)
                c.setExclusiveTool(QStringLiteral("sel_freeform"));
        };
        hand.registerProfile(profile);
    }

    void deactivate(InputHub &hub, HandTouchModifier &hand) override
    {
        (void)hub;
        hand.clearProfile(ModeId::Pen);
        m_caps = nullptr;
    }

    HostCaps *caps() const { return m_caps; }

private:
    HostCaps *m_caps = nullptr;
};

} // namespace tools
} // namespace epaper
