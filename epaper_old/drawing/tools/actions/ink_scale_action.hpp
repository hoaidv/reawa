#pragma once

/**
 * InkScale — toggle Keep size / Scale ink on a selected ink-box.
 * @implements [SRS-EP-12]
 */

#include "action.hpp"
#include "document/capability.hpp"
#include "document/manipulate.hpp"
#include "document/nested_inkbox.hpp"
#include "../contexts/doc_context.hpp"
#include "../host_caps.hpp"
#include "../contexts/selection_context.hpp"
#include "../contexts/tool_context.hpp"
#include "../ui/selection_overlay.hpp"

namespace epaper {
namespace tools {

class InkScaleAction final : public ToolAction {
public:
    QString id() const override { return QStringLiteral("ink_scale"); }
    QString label(const HostCaps &caps) const override
    {
        const epaper::document::DocNode *n = selected(caps);
        if (n && n->inkScaleMode == "fixedInk")
            return QStringLiteral("Keep size");
        return QStringLiteral("Scale ink");
    }

    bool visible(const HostCaps &caps) const override
    {
        if (!caps.toolUi || !caps.selection || !caps.doc)
            return false;
        if (caps.selection->phase() != SelectionPhase::Selected)
            return false;
        const epaper::document::DocNode *n = selected(caps);
        return n && epaper::document::descriptorFor(n->kind).has(epaper::document::Verb::SetInkScaleMode);
    }

    bool enabled(const HostCaps &caps) const override { return visible(caps); }

    void trigger(HostCaps &caps) override
    {
        if (!visible(caps))
            return;
        const epaper::document::DocNode *n = selected(caps);
        if (!n)
            return;
        epaper::document::SmartBounds wb;
        if (epaper::document::composedBoundsOf(caps.doc->document(), *n, wb) && !caps.toolUi->lodOkPanel(wb)) {
            if (caps.overlay)
                caps.overlay->showManipUnavailable(caps, wb);
            return;
        }
        caps.doc->toggleInkScaleMode(n->id);
        caps.toolUi->refreshChrome();
    }

private:
    static const epaper::document::DocNode *selected(const HostCaps &caps)
    {
        if (!caps.doc || !caps.selection || caps.selection->ids().size() != 1)
            return nullptr;
        return caps.doc->document().find(caps.selection->pickableId());
    }
};

} // namespace tools
} // namespace epaper
