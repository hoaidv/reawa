#pragma once

/**
 * Selection Mode — exclusive Mode for sel_rect / sel_freeform.
 * Owns overlay policy (phase + armed chip); forwards in-flight paint to Operations.
 * @implements [SRS-EP-04] @implements [SRS-EP-11] @implements [SRS-EP-12]
 */

#include "../input_hub.hpp"
#include "../mode.hpp"
#include "../operation.hpp"
#include "../contexts/doc_context.hpp"
#include "../contexts/selection_context.hpp"
#include "../contexts/session_doc_context.hpp"
#include "../contexts/tool_context.hpp"
#include "../ui/selection_context_bar.hpp"
#include "../ui/selection_overlay.hpp"

#include <QLatin1String>
#include <QPainter>
#include <QString>

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
            OperationKind::Move,
            OperationKind::Resize,
            OperationKind::Rotate,
        };
    }

    void activate(HostCaps &caps, InputHub &hub) override { syncOverlay(caps, hub); }

    void paintOverlay(QPainter *painter, HostCaps &caps, InputHub &hub) override
    {
        if (!caps.selection || !caps.overlay)
            return;
        const SelectionPhase phase = caps.selection->phase();
        if (phase == SelectionPhase::Idle)
            return;
        if (phase == SelectionPhase::Selecting) {
            if (Operation *op = hub.lockedOperation())
                op->paintOverlay(painter);
            return;
        }
        if (phase == SelectionPhase::Transforming) {
            if (Operation *op = hub.lockedOperation())
                op->paintOverlay(painter);
            caps.overlay->paintLiveManip(painter, caps);
            return;
        }
        caps.overlay->paintSettled(painter, caps);
    }

    void syncOverlay(HostCaps &caps, InputHub &hub) override
    {
        (void)hub;
        if (!caps.toolUi || !caps.selection || !caps.doc)
            return;
        caps.toolUi->setOverlayVisible(true);
        const SelectionPhase phase = caps.selection->phase();
        const QString ex = caps.doc->exclusiveTool();
        bool pen = false;
        if (phase == SelectionPhase::Selecting)
            pen = true;
        else if (phase != SelectionPhase::Selected && phase != SelectionPhase::Transforming)
            pen = ex == QLatin1String("sel_freeform");
        caps.toolUi->setStrokeWaveform(pen);
    }

    void refreshChrome(HostCaps &caps, InputHub &hub) override
    {
        if (!caps.overlay || !caps.selection || !caps.toolUi)
            return;
        auto *sess = dynamic_cast<SessionDocContext *>(caps.doc);
        if (!sess)
            return;
        const bool knobs = caps.selection->phase() == SelectionPhase::Selected;
        caps.overlay->refresh(*caps.selection, *sess, knobs);
        if (caps.bar)
            caps.bar->refresh(caps, caps.overlay->state());
        if (caps.emitChromeChanged)
            caps.emitChromeChanged();
        caps.overlay->publishOverlayHits(hub);
        syncOverlay(caps, hub);
        caps.toolUi->damageChrome(caps.overlay->state().selectionChromeDirty);
    }
};

} // namespace tools
} // namespace epaper
