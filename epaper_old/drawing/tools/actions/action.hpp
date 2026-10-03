#pragma once

/**
 * ToolAction — click → document command (not a pointer-locked Operation).
 * @implements [SRS-EP-12] @implements [ADR-0033]
 */

#include <QString>

namespace epaper {
namespace tools {

struct HostCaps;

class ToolAction {
public:
    virtual ~ToolAction() = default;
    virtual QString id() const = 0;
    virtual QString icon() const { return {}; }
    virtual QString label(const HostCaps &) const { return {}; }
    virtual bool visible(const HostCaps &) const = 0;
    virtual bool enabled(const HostCaps &) const = 0;
    virtual void trigger(HostCaps &) = 0;
};

} // namespace tools
} // namespace epaper
