#pragma once

/**
 * Enclose — group ≥2 selected inks into a Smart Group.
 * @implements [SRS-EP-12]
 */

#include "action.hpp"
#include "document/device_document.hpp"
#include "../contexts/doc_context.hpp"
#include "../host_caps.hpp"
#include "../contexts/selection_context.hpp"
#include "../contexts/tool_context.hpp"

namespace epaper {
namespace tools {

class EncloseAction final : public ToolAction {
public:
    QString id() const override { return QStringLiteral("enclose"); }
    QString icon() const override { return QStringLiteral("qrc:/icons/icons/icon-epaper-enclose.png"); }
    QString label(const HostCaps &) const override { return QStringLiteral("Enclose"); }

    bool visible(const HostCaps &caps) const override
    {
        if (!caps.toolUi || !caps.selection || !caps.doc)
            return false;
        if (!caps.toolUi->isSelectionTool())
            return false;
        if (caps.selection->phase() != SelectionPhase::Selected)
            return false;
        int inks = 0;
        for (const std::string &id : caps.selection->ids()) {
            const auto *n = caps.doc->document().find(id);
            if (n && n->kind == epaper::document::NodeKind::Ink)
                ++inks;
        }
        return inks >= 2;
    }

    bool enabled(const HostCaps &caps) const override { return visible(caps); }

    void trigger(HostCaps &caps) override
    {
        if (!visible(caps))
            return;
        QString refuse;
        if (!caps.doc->encloseSelection(caps.selection->ids(), &refuse)) {
            caps.toolUi->setRefuseReason(refuse);
            return;
        }
        caps.selection->clear();
        caps.toolUi->requestChromeRefresh();
    }
};

} // namespace tools
} // namespace epaper
