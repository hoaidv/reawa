#pragma once

/**
 * SelectionMode tap vs travel classifier (Qt-free).
 * @implements [SRS-EP-11] tap vs travel
 * @implements [ADR-0037] tap-origin paste routing
 */

#include "document/hand_touch.hpp"

namespace epaper {
namespace tools {

constexpr double kHoldStillMm = 1.0;

inline bool holdTravelExceeded(double panelDx, double panelDy)
{
    return epaper::handtouch::travelMm(panelDx, panelDy) > kHoldStillMm;
}

} // namespace tools
} // namespace epaper
