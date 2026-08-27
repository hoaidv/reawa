#pragma once
/**
 * @implements [SRS-EP-07] create_group
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"
#include "remove_node_edit.hpp"

#include <stdexcept>

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

inline ApplyResult CreateGroupEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    doc.requireUnique(m_nodeId);
    DocNode n;
    n.id = m_nodeId;
    n.kind = NodeKind::Group;
    doc.insertUnder(m_parentId, std::move(n));
    return {true, {}};
}

inline std::unique_ptr<DocEdit> CreateGroupEdit::generateUndo(const DeviceDocument &) const
{
    return makeRemoveEdit(m_id, m_nodeId);
}

inline JsonValue CreateGroupEdit::serialize() const
{
    JsonValue::Object p;
    p.emplace_back("id", JsonValue::string(m_nodeId));
    if (m_parentId)
        p.emplace_back("parentId", JsonValue::string(*m_parentId));
    return envelope(JsonValue::object(std::move(p)));
}

inline std::unique_ptr<DocEdit> CreateGroupEdit::clone() const
{
    return std::make_unique<CreateGroupEdit>(*this);
}

inline std::unique_ptr<CreateGroupEdit> CreateGroupEdit::fromPayload(const JsonValue &envelope,
                                                                    const JsonValue &payload)
{
    auto e = std::make_unique<CreateGroupEdit>(payload.getString("id"));
    fillMeta(*e, envelope);
    e->setParentId(parentIdFromJson(payload));
    return e;
}

} // namespace document
} // namespace epaper
