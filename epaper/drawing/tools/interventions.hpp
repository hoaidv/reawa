#pragma once

/**
 * Cross-cutting input policies (register at startup; Router executes).
 * @implements [SRS-EP-21] @implements [SRS-EP-24]
 */

namespace epaper {
namespace tools {

enum class Intervention {
    None,
    PenNearCancel,       // cancel finger + active Operation
    SecondContactAbort,  // abort one-finger manip; lock until lift
    PinchSuppressOneFinger,
};

} // namespace tools
} // namespace epaper
