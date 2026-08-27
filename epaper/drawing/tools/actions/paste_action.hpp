#pragma once

/**
 * Paste — visible on non-empty selection; clipboard not wired yet.
 * @implements [SRS-EP-12]
 */

#include "action.hpp"
#include "../host_caps.hpp"
#include "../contexts/selection_context.hpp"

namespace epaper {
namespace tools {

class PasteAction final : public ToolAction {
public:
    QString id() const override { return QStringLiteral("paste"); }
    QString icon() const override { return QStringLiteral("qrc:/icons/icons/icon-epaper-paste.png"); }
    QString label(const HostCaps &) const override { return QStringLiteral("Paste"); }
    bool visible(const HostCaps &caps) const override
    {
        return caps.selection && caps.selection->phase() == SelectionPhase::Selected
            && !caps.selection->ids().empty();
    }
    bool enabled(const HostCaps &) const override { return false; }
    void trigger(HostCaps &) override {}
};

} // namespace tools
} // namespace epaper
