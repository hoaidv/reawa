#pragma once

/**
 * SelectionIntentApplier — SelectionResult → ToolContext (+ manip reset).
 * @implements [SRS-EP-11] @implements [SRS-EP-12]
 */

#include "../selection_session.hpp"
#include "tool_context.hpp"

#include <functional>

namespace epaper {
namespace tools {

class SelectionIntentApplier {
public:
    void setTool(ToolContext *tool) { m_tool = tool; }
    void setResetManip(std::function<void()> fn) { m_resetManip = std::move(fn); }
    void setInteractionDebug(std::function<void(const std::string &)> fn)
    {
        m_setDebug = std::move(fn);
    }

    void apply(const epaper::selection::SelectionResult &r);

private:
    ToolContext *m_tool = nullptr;
    std::function<void()> m_resetManip;
    std::function<void(const std::string &)> m_setDebug;
};

} // namespace tools
} // namespace epaper
