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
#include "../contexts/tool_context.hpp"
#include "../ui/selection_overlay.hpp"
#include "document/surround_create.hpp"

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
        return !clipboard().empty() && caps.pasteOriginValid;
    }
    bool enabled(const HostCaps &caps) const override { return visible(caps); }
    void trigger(HostCaps &caps) override
    {
        if (!caps.doc || !visible(caps))
            return;
        const std::string hit = caps.doc->hitSelectTarget(caps.pastePressWorld.x(),
                                                          caps.pastePressWorld.y());
        const auto out = clipops::commitPaste(caps.doc->document(), clipboard(),
                                              caps.pastePressWorld.x(), caps.pastePressWorld.y(),
                                              hit, /*enqueue=*/false);
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
            world.x = caps.pastePressWorld.x();
            world.y = caps.pastePressWorld.y();
            const QPointF tl = caps.toolUi->worldToPanel(world.x, world.y);
            const QPointF br =
                caps.toolUi->worldToPanel(world.x + world.width, world.y + world.height);
            caps.doc->noteDocumentDirty(QRectF(tl, br).normalized().adjusted(-8, -8, 8, 8));
        } else {
            caps.doc->noteDocumentMutated();
        }
        caps.doc->notifyHistory();
        caps.clearPasteOrigin();
        if (caps.toolUi)
            caps.toolUi->refreshChrome();
    }
};

} // namespace tools
} // namespace epaper
