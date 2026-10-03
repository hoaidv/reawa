#pragma once
/**
 * Shared capability descriptor — registration table, not the gesture router.
 * @implements [SRS-EP-11] SmartGroup verb set
 */

#include "device_document.hpp"

#include <cstdint>

namespace epaper {
namespace document {

enum class Verb : std::uint8_t {
    Select = 1,
    Move = 2,
    Resize = 4,
    SetInkScaleMode = 8,
};

struct CapabilityDescriptor {
    std::uint8_t verbs = 0;
    bool has(Verb v) const { return (verbs & static_cast<std::uint8_t>(v)) != 0; }
};

/** Kind → verbs. The only place node kind maps to manipulation. */
inline CapabilityDescriptor descriptorFor(NodeKind kind)
{
    CapabilityDescriptor d;
    if (kind == NodeKind::SmartGroup) {
        d.verbs = static_cast<std::uint8_t>(Verb::Select) | static_cast<std::uint8_t>(Verb::Move)
            | static_cast<std::uint8_t>(Verb::Resize) | static_cast<std::uint8_t>(Verb::SetInkScaleMode);
        return d;
    }
    if (kind == NodeKind::Ink || kind == NodeKind::Text || kind == NodeKind::Primitive
        || kind == NodeKind::Frame || kind == NodeKind::Connector) {
        d.verbs = static_cast<std::uint8_t>(Verb::Select);
        return d;
    }
    return d;
}

inline bool smartGroupVerbsExact(const CapabilityDescriptor &d)
{
    const std::uint8_t want = static_cast<std::uint8_t>(Verb::Select)
        | static_cast<std::uint8_t>(Verb::Move) | static_cast<std::uint8_t>(Verb::Resize)
        | static_cast<std::uint8_t>(Verb::SetInkScaleMode);
    return d.verbs == want;
}

} // namespace document
} // namespace epaper
