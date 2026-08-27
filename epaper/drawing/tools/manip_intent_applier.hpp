#pragma once

/**
 * ManipIntentApplier — ManipResult → DocContext + ToolContext (Phase 5).
 * @implements [SRS-EP-11]
 */

#include "../manip_session.hpp"
#include "doc_context.hpp"
#include "tool_context.hpp"

namespace epaper {
namespace tools {

class ManipIntentApplier {
public:
    void setDoc(DocContext *doc) { m_doc = doc; }
    void setTool(ToolContext *tool) { m_tool = tool; }

    void apply(const epaper::manip::ManipResult &r, epaper::manip::ManipSession &manip,
               bool restoreOrigin, int *toolIntentSeq);

private:
    DocContext *m_doc = nullptr;
    ToolContext *m_tool = nullptr;
};

} // namespace tools
} // namespace epaper
