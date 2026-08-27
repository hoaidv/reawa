#pragma once
/**
 * @implements [SRS-EP-07] remove_node
 */

#include "doc_edit.hpp"

namespace epaper {
namespace document {

class RemoveNodeEdit final : public DocEdit {
public:
    RemoveNodeEdit() = default;
    explicit RemoveNodeEdit(std::string nodeId) : m_nodeId(std::move(nodeId)) {}

    const char *kind() const override { return edit_kind::kRemoveNode; }

    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    const std::string &nodeId() const { return m_nodeId; }
    void setNodeId(std::string id) { m_nodeId = std::move(id); }

    static std::unique_ptr<RemoveNodeEdit> fromPayload(const JsonValue &envelope,
                                                       const JsonValue &payload);

private:
    std::string m_nodeId;
};

} // namespace document
} // namespace epaper
