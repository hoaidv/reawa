#pragma once

/**
 * FingerIntentApplier — FingerResult → DocContext + ToolContext + selection/manip.
 * @implements [SRS-EP-21] @implements [SRS-EP-24]
 */

#include "../finger_gesture_machine.hpp"
#include "doc_context.hpp"
#include "session_doc_context.hpp"
#include "tool_context.hpp"

#include <QPointF>
#include <functional>

namespace epaper {
namespace tools {

class SelectionManipController;

class FingerIntentApplier {
public:
    void setDoc(SessionDocContext *doc) { m_doc = doc; }
    void setTool(ToolContext *tool) { m_tool = tool; }
    void setSelectionManip(SelectionManipController *ctrl) { m_selManip = ctrl; }
    void setAbortManip(std::function<void()> fn) { m_abortManip = std::move(fn); }

    void apply(const epaper::fingergesture::FingerResult &r, const QPointF &panel = QPointF());

private:
    SessionDocContext *m_doc = nullptr;
    ToolContext *m_tool = nullptr;
    SelectionManipController *m_selManip = nullptr;
    std::function<void()> m_abortManip;
};

} // namespace tools
} // namespace epaper
