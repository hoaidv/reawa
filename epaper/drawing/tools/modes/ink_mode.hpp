#pragma once

/**
 * Ink Mode — exclusive Mode id Ink; primary ink, secondary nav/pick/move.
 * Overlay only while Transforming (live node after tablet suppress).
 * @implements [SRS-EP-04] @implements [SRS-EP-12]
 */

#include "../input_hub.hpp"
#include "../mode.hpp"
#include "../operation.hpp"
#include "../contexts/selection_context.hpp"
#include "../contexts/tool_context.hpp"

#include <QPainter>
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

    void activate(HostCaps &caps, InputHub &hub) override { syncOverlay(caps, hub); }

    void onSecondaryCommit(HostCaps &caps, const SecondaryCommitInfo &info) override
    {
        if (!info.didMutateSelection || !info.selectionNonEmpty)
            return;
        if (caps.setExclusiveTool)
            caps.setExclusiveTool(QStringLiteral("sel_freeform"));
    }

    void paintOverlay(QPainter *painter, HostCaps &caps, InputHub &hub) override
    {
        if (!caps.selection || !caps.toolUi)
            return;
        if (caps.selection->phase() != SelectionPhase::Transforming)
            return;
        if (Operation *op = hub.lockedOperation())
            op->paintOverlay(painter);
        caps.toolUi->paintSelectionChrome(painter);
    }

    void syncOverlay(HostCaps &caps, InputHub &hub) override
    {
        (void)hub;
        if (!caps.toolUi)
            return;
        const bool transforming =
            caps.selection && caps.selection->phase() == SelectionPhase::Transforming;
        caps.toolUi->setOverlayVisible(transforming);
        if (transforming)
            caps.toolUi->setStrokeWaveform(false);
    }
};

} // namespace tools
} // namespace epaper
