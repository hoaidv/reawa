#pragma once

/**
 * Cut — clone then remove selection roots; empty groups left in the tree.
 * @implements [SRS-EP-31] clipboard cut
 * @implements [SRS-EP-32] cta.cut
 */

#include "action.hpp"
#include "../clipboard.hpp"
#include "../host_caps.hpp"
#include "../contexts/selection_context.hpp"
#include "../contexts/doc_context.hpp"
#include "../contexts/tool_context.hpp"
#include "document/surround_create.hpp"

#include <QRectF>

namespace epaper {
namespace tools {

class CutAction final : public ToolAction {
public:
    QString id() const override { return QStringLiteral("cut"); }
    QString icon() const override { return QStringLiteral("qrc:/icons/icons/icon-epaper-cut.png"); }
    QString label(const HostCaps &) const override { return QStringLiteral("Cut"); }
    bool visible(const HostCaps &caps) const override
    {
        return caps.selection && caps.selection->phase() == SelectionPhase::Selected
            && !caps.selection->ids().empty();
    }
    bool enabled(const HostCaps &caps) const override { return visible(caps); }
    void trigger(HostCaps &caps) override
    {
        if (!visible(caps) || !caps.doc || !caps.selection)
            return;
        epaper::document::SmartBounds world;
        const bool hadBounds =
            epaper::document::unionAabbOfIds(caps.doc->document(), caps.selection->ids(), world);
        const auto r = clipops::commitCut(caps.doc->document(), clipboard(),
                                          caps.selection->ids(), /*enqueue=*/false);
        if (!r.applied)
            return;
        caps.selection->clear();
        if (hadBounds && caps.toolUi) {
            const QPointF tl = caps.toolUi->worldToPanel(world.x, world.y);
            const QPointF br =
                caps.toolUi->worldToPanel(world.x + world.width, world.y + world.height);
            caps.doc->noteDocumentDirty(QRectF(tl, br).normalized().adjusted(-8, -8, 8, 8));
        } else {
            caps.doc->noteDocumentMutated();
        }
        caps.doc->notifyHistory();
        if (caps.toolUi)
            caps.toolUi->refreshChrome();
    }
};

} // namespace tools
} // namespace epaper
