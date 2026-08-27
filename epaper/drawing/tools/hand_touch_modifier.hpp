#pragma once

/**
 * HandTouch modifier — armed + lock-until-lift; profile dispatch by Mode id.
 * @implements [SRS-EP-21]
 */

#include "hand_touch_profile.hpp"

#include <unordered_map>

namespace epaper {
namespace tools {

class HandTouchModifier {
public:
    bool armed() const { return m_armed; }
    void setArmed(bool on)
    {
        m_armed = on;
        if (!on)
            m_lockedUntilLift = false;
    }

    bool lockedUntilLift() const { return m_lockedUntilLift; }
    void setLockedUntilLift(bool on) { m_lockedUntilLift = on; }

    void registerProfile(const HandTouchProfile &profile)
    {
        m_profiles[static_cast<int>(profile.modeId)] = profile;
    }

    void clearProfile(ModeId id) { m_profiles.erase(static_cast<int>(id)); }

    const HandTouchProfile *profileFor(ModeId id) const
    {
        const auto it = m_profiles.find(static_cast<int>(id));
        return it == m_profiles.end() ? nullptr : &it->second;
    }

    /** Run profile postHandling if set (empty callback = no-op). */
    void runPostHandling(ModeId id, HostCaps &caps, const HandTouchCommitInfo &info) const
    {
        const HandTouchProfile *p = profileFor(id);
        if (p && p->postHandling)
            p->postHandling(caps, info);
    }

private:
    bool m_armed = true;
    bool m_lockedUntilLift = false;
    std::unordered_map<int, HandTouchProfile> m_profiles;
};

} // namespace tools
} // namespace epaper
