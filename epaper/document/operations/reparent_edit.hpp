#pragma once
/**
 * @implements [SRS-EP-07] reparent restore body + index
 */

#include "doc_edit.hpp"

#include <optional>

namespace epaper {
namespace document {

class ReparentEdit final : public DocEdit {
public:
    ReparentEdit() = default;

    const char *kind() const override { return edit_kind::kReparent; }

    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setNewParentId(std::string p) { m_newParentId = std::move(p); }
    void setIndex(int i) { m_index = i; }
    void setBody(DocNode n)
    {
        m_body = std::move(n);
        m_hasBody = true;
    }
    bool hasBody() const { return m_hasBody; }
    const std::string &nodeId() const { return m_nodeId; }

    static ReparentEdit restore(const std::string &id, const std::string &parentId, int index,
                                DocNode nodeBody);

    static std::unique_ptr<ReparentEdit> fromPayload(const JsonValue &envelope,
                                                     const JsonValue &payload);

private:
    std::string m_nodeId;
    std::string m_newParentId;
    int m_index = -1;
    DocNode m_body;
    bool m_hasBody = false;
};

} // namespace document
} // namespace epaper
