#pragma once

/**
 * Copy — clone selection roots into the process-global slot.
 * @implements [SRS-EP-31] clipboard copy
 * @implements [SRS-EP-32] cta.copy
 */

#include "action.hpp"
#include "../clipboard.hpp"
#include "../host_caps.hpp"
#include "../contexts/selection_context.hpp"
#include "../contexts/doc_context.hpp"
#include "../contexts/tool_context.hpp"

namespace epaper {
namespace tools {

class CopyAction final : public ToolAction {
public:
    QString id() const override { return QStringLiteral("copy"); }
    QString icon() const override { return QStringLiteral("qrc:/icons/icons/icon-epaper-copy.png"); }
    QString label(const HostCaps &) const override { return QStringLiteral("Copy"); }
    bool visible(const HostCaps &caps) const override
    {
        return caps.selection && caps.selection->phase() == SelectionPhase::Selected
            && !caps.selection->ids().empty();
    }
    bool enabled(const HostCaps &caps) const override { return visible(caps); }
    void trigger(HostCaps &caps) override
    {
        if (!visible(caps) || !caps.doc)
            return;
        clipops::copyToSlot(caps.doc->document(), caps.selection->ids(), clipboard());
        if (caps.toolUi)
            caps.toolUi->refreshChrome();
    }
};

} // namespace tools
} // namespace epaper
