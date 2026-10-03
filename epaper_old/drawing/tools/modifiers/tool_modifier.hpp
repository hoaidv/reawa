#pragma once

/**
 * ToolModifier — orthogonal Mode behavior (not an exclusive Mode, not an Operation).
 * SecondaryDeviceModifier gates Secondary pointer ops; recognizers change the pen-up pipeline.
 * @implements [SRS-EP-04] @implements [ADR-0033]
 */

namespace epaper {
namespace tools {

class ToolModifier {
public:
    virtual ~ToolModifier() = default;
    virtual bool armed() const = 0;
};

} // namespace tools
} // namespace epaper
