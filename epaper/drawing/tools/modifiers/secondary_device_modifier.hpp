#pragma once

/**
 * SecondaryDeviceModifier — armed + lock-until-lift for Secondary pointer ops.
 * Allow-lists live on InteractionMode (secondaryOps); this is the on/off gate.
 * @implements [SRS-EP-21]
 */

#include "tool_modifier.hpp"

namespace epaper {
namespace tools {

class SecondaryDeviceModifier : public ToolModifier {
public:
    bool armed() const override { return m_armed; }
    void setArmed(bool on)
    {
        m_armed = on;
        if (!on)
            m_lockedUntilLift = false;
    }

    bool lockedUntilLift() const { return m_lockedUntilLift; }
    void setLockedUntilLift(bool on) { m_lockedUntilLift = on; }

private:
    bool m_armed = true;
    bool m_lockedUntilLift = false;
};

} // namespace tools
} // namespace epaper
