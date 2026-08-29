#pragma once

/**
 * Eraser Mode — BrushErase / AreaErase / ObjectErase; secondary Navigation only.
 * @implements [SRS-EP-54] erase mode
 */

#include "../mode.hpp"
#include "../operation.hpp"

namespace epaper {
namespace tools {

class EraserMode final : public InteractionMode {
public:
    ModeId id() const override { return ModeId::Eraser; }

    std::vector<OperationKind> primaryOps() const override
    {
        return {OperationKind::BrushErase, OperationKind::AreaErase, OperationKind::ObjectErase};
    }

    std::vector<OperationKind> secondaryOps() const override
    {
        return {OperationKind::Navigation};
    }
};

} // namespace tools
} // namespace epaper
