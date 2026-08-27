#pragma once

/**
 * Selection Mode — exclusive Mode for sel_rect / sel_freeform.
 * @implements [SRS-EP-04] @implements [SRS-EP-11]
 */

#include "../mode.hpp"
#include "../operation.hpp"

namespace epaper {
namespace tools {

class SelectionMode final : public InteractionMode {
public:
    ModeId id() const override { return ModeId::Selection; }

    std::vector<OperationKind> primaryOps() const override
    {
        return {OperationKind::Resize, OperationKind::Move, OperationKind::Lasso,
                OperationKind::Marquee};
    }

    std::vector<OperationKind> secondaryOps() const override
    {
        return {
            OperationKind::Navigation,
            OperationKind::Select,
            // OperationKind::Lasso,
            // OperationKind::Marquee,
            OperationKind::Move,
            OperationKind::Resize,
            OperationKind::Rotate,
        };
    }
};

} // namespace tools
} // namespace epaper
