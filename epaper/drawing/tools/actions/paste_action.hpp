#pragma once

/**
 * Paste — insert slot clones at the unclamped tap world point.
 * @implements [SRS-EP-31] clipboard paste
 * @implements [SRS-EP-32] cta.paste
 * @fix [CHL-0031] tap-origin paste
 */

#include "action.hpp"
#include "../clipboard.hpp"
#include "../host_caps.hpp"
#include "../contexts/doc_context.hpp"
#include "../contexts/selection_context.hpp"
#include "../contexts/tool_context.hpp"
#include "../ui/selection_overlay.hpp"
#include "document/surround_create.hpp"

#include <QPointF>
#include <QRectF>
#include <QString>

namespace epaper {
namespace tools {

class PasteAction final : public ToolAction {
public:
    QString id() const override { return QStringLiteral("paste"); }
    QString icon() const override { return QStringLiteral("qrc:/icons/icons/icon-epaper-paste.png"); }
    QString label(const HostCaps &) const override { return QStringLiteral("Paste"); }
    bool visible(const HostCaps &caps) const override
    {
        return !clipboard().empty() && caps.selection && caps.selection->pasteOriginValid();
    }
    bool enabled(const HostCaps &caps) const override { return visible(caps); }
    void trigger(HostCaps &caps) override
    {
        if (!caps.doc || !caps.selection || !visible(caps))
            return;
        const PasteOrigin &origin = caps.selection->pasteOrigin();
        const std::string hit = caps.doc->hitSelectTarget(origin.worldX, origin.worldY);
        const auto out = clipops::commitPaste(caps.doc->document(), clipboard(), origin.worldX,
                                              origin.worldY, hit, /*enqueue=*/false);
        if (out.refuse == clipops::PasteRefuse::LiveOriginal) {
            if (caps.overlay)
                caps.overlay->setRefuseReason(
                    caps, QString::fromUtf8(clipops::kPasteOntoOriginals));
            return;
        }
        if (!out.result.applied)
            return;
        epaper::document::SmartBounds world;
        if (clipops::unionAabb(clipboard().nodes, world) && caps.toolUi) {
            world.x = origin.worldX;
            world.y = origin.worldY;
            const QPointF tl = caps.toolUi->worldToPanel(world.x, world.y);
            const QPointF br =
                caps.toolUi->worldToPanel(world.x + world.width, world.y + world.height);
            caps.doc->noteDocumentDirty(QRectF(tl, br).normalized().adjusted(-8, -8, 8, 8));
        } else {
            caps.doc->noteDocumentMutated();
        }
        caps.doc->notifyHistory();
        caps.selection->clearPasteOrigin();
        if (caps.toolUi)
            caps.toolUi->refreshChrome();
    }
};

} // namespace tools
} // namespace epaper
