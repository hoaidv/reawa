#pragma once

/**
 * Capability ports injected into Modes/Operations (ADR-0033).
 * ToolContext = SelectionOverlay Tool UI (not primary ToolChip in this slice).
 * @implements [SRS-EP-04] @implements [SRS-EP-12]
 */

#include <QString>

#include <functional>

namespace epaper {
namespace tools {

class InkSink;
class DocContext;
class ToolContext;
class SelectionContext;

struct HostCaps {
    InkSink *ink = nullptr;
    DocContext *doc = nullptr;
    ToolContext *toolUi = nullptr;
    SelectionContext *selection = nullptr;
    /** Switch primary exclusive tool id (e.g. sel_freeform). Empty = unsupported. */
    std::function<void(const QString &exclusiveToolId)> setExclusiveTool;
};

} // namespace tools
} // namespace epaper
