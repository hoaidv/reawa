#pragma once

/**
 * Eraser Mode — BrushErase / AreaErase / ObjectErase; secondary Navigation only.
 * Owns overlay policy; idle paint goes to the exclusive-armed Operation.
 * @implements [SRS-EP-54] erase mode
 */

#include "../input_hub.hpp"
#include "../mode.hpp"
#include "../operation.hpp"
#include "../contexts/tool_context.hpp"

#include <QLatin1String>
#include <QPainter>
#include <QString>
#include <climits>

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

    void activate(HostCaps &caps, InputHub &hub) override { syncOverlay(caps, hub); }

    void paintOverlay(QPainter *painter, HostCaps &caps, InputHub &hub) override
    {
        (void)caps;
        if (Operation *locked = hub.lockedOperation()) {
            locked->paintOverlay(painter);
            return;
        }
        if (Operation *armed = armedExclusive(hub))
            armed->paintOverlay(painter);
    }

    void syncOverlay(HostCaps &caps, InputHub &hub) override
    {
        (void)hub;
        if (!caps.toolUi)
            return;
        caps.toolUi->setOverlayVisible(true);
        const QString ex = caps.toolUi->exclusiveTool();
        const bool pen = ex.startsWith(QLatin1String("erase_"));
        caps.toolUi->setStrokeWaveform(pen);
    }

private:
    Operation *armedExclusive(InputHub &hub) const
    {
        PointerSample dummy;
        dummy.role = PointerRole::Primary;
        dummy.device = PointerDevice::Pen;
        Operation *best = nullptr;
        int bestPriority = INT_MIN;
        for (OperationKind kind : primaryOps()) {
            Operation *op = hub.operation(kind);
            if (!op || !op->match(StrategyKind::RawPointer, dummy))
                continue;
            if (op->descriptor().priority > bestPriority) {
                bestPriority = op->descriptor().priority;
                best = op;
            }
        }
        return best;
    }
};

} // namespace tools
} // namespace epaper
