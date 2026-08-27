#pragma once

/**
 * Copy — visible on non-empty selection; clipboard not wired yet.
 * @implements [SRS-EP-12]
 */

#include "action.hpp"
#include "../host_caps.hpp"
#include "../selection_context.hpp"

namespace epaper {
namespace tools {

class CopyAction final : public ToolAction {
public:
    QString id() const override { return QStringLiteral("copy"); }
    QString label(const HostCaps &) const override { return QStringLiteral("Copy"); }
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
