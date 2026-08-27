#pragma once
/**
 * @implements [SRS-EP-07] create_text
 */

#include "doc_edit.hpp"

namespace epaper {
namespace document {

class CreateTextEdit final : public DocEdit {
public:
    CreateTextEdit() = default;

    const char *kind() const override { return edit_kind::kCreateText; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setParentId(std::optional<std::string> p) { m_parentId = std::move(p); }
    void setBox(Aabb b) { m_box = b; }
    void setStyle(Style s) { m_style = std::move(s); }
    void setRuns(std::vector<TextRun> r) { m_runs = std::move(r); }
    const std::string &nodeId() const { return m_nodeId; }

    static std::unique_ptr<CreateTextEdit> fromPayload(const JsonValue &envelope,
                                                       const JsonValue &payload);

private:
    std::string m_nodeId;
    std::optional<std::string> m_parentId;
    Aabb m_box;
    Style m_style;
    std::vector<TextRun> m_runs;
};

} // namespace document
} // namespace epaper
