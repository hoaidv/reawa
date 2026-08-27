#pragma once

/**
 * Interventions — registered input policies (gate + optional matcher + apply).
 * Router executes only rows whose gate matches the event. Not pointer-move.
 * @implements [SRS-EP-21] @implements [SRS-EP-24]
 */

#include <functional>

namespace epaper {
namespace tools {

enum class InterventionGate {
    PenProximity,
    PenDown,
    SecondContact,
};

struct Intervention {
    InterventionGate gate = InterventionGate::PenProximity;
    std::function<bool()> match; // empty = true
    std::function<void()> apply;
};

} // namespace tools
} // namespace epaper
