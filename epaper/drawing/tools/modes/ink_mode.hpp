#pragma once

/**
 * Ink Mode — exclusive Mode id Ink; primary ink, secondary nav/pick/move.
 * @implements [SRS-EP-04]
 */

#include "../mode.hpp"
#include "../operation.hpp"

#include <QString>

namespace epaper {
namespace tools {

class InkMode final : public InteractionMode {
public:
    ModeId id() const override { return ModeId::Ink; }

    std::vector<OperationKind> primaryOps() const override { return {OperationKind::InkStroke}; }

    std::vector<OperationKind> secondaryOps() const override
    {
        return {OperationKind::Navigation, OperationKind::Select, OperationKind::Move};
    }

    void onSecondaryCommit(HostCaps &caps, const SecondaryCommitInfo &info) override
    {
        if (!info.didMutateSelection || !info.selectionNonEmpty)
            return;
        if (caps.setExclusiveTool)
            caps.setExclusiveTool(QStringLiteral("sel_freeform"));
    }
};

} // namespace tools
} // namespace epaper
