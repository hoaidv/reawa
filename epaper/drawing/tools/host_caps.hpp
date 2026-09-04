#pragma once

/**
 * Capability ports injected into Modes/Operations (ADR-0033 / ADR-0035).
 * toolUi is ToolContext host ports, not selection overlay.
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
class SelectionOverlay;
class SelectionContextBar;
class NodeEmphasis;

struct HostCaps {
    InkSink *ink = nullptr;
    DocContext *doc = nullptr;
    ToolContext *toolUi = nullptr;
    SelectionContext *selection = nullptr;
    SelectionOverlay *overlay = nullptr;
    SelectionContextBar *bar = nullptr;
    NodeEmphasis *emphasis = nullptr;
    std::function<void()> emitChromeChanged;
    /** Switch primary exclusive tool id (e.g. sel_freeform). Empty = unsupported. */
    std::function<void(const QString &exclusiveToolId)> setExclusiveTool;
};

} // namespace tools
} // namespace epaper
