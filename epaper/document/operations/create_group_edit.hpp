#pragma once
/**
 * @implements [SRS-EP-07] create_group
 */

#include "doc_edit.hpp"

namespace epaper {
namespace document {

class CreateGroupEdit final : public DocEdit {
public:
    CreateGroupEdit() = default;
    explicit CreateGroupEdit(std::string nodeId) : m_nodeId(std::move(nodeId)) {}

    const char *kind() const override { return edit_kind::kCreateGroup; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setParentId(std::optional<std::string> p) { m_parentId = std::move(p); }
    const std::string &nodeId() const { return m_nodeId; }

    static std::unique_ptr<CreateGroupEdit> fromPayload(const JsonValue &envelope,
                                                        const JsonValue &payload);

private:
    std::string m_nodeId;
    std::optional<std::string> m_parentId;
};

} // namespace document
} // namespace epaper
