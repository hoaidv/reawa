#pragma once
/**
 * @implements [SRS-EP-07] create_frame
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"
#include "remove_node_edit.hpp"

#include <stdexcept>

namespace epaper {
namespace document {

class CreateFrameEdit final : public DocEdit {
public:
    CreateFrameEdit() = default;
    CreateFrameEdit(std::string nodeId, Aabb bounds)
        : m_nodeId(std::move(nodeId))
        , m_bounds(bounds)
    {
    }

    const char *kind() const override { return edit_kind::kCreateFrame; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setBounds(Aabb b) { m_bounds = b; }
    const std::string &nodeId() const { return m_nodeId; }

    static std::unique_ptr<CreateFrameEdit> fromPayload(const JsonValue &envelope,
                                                        const JsonValue &payload);

private:
    std::string m_nodeId;
    Aabb m_bounds;
};

inline ApplyResult CreateFrameEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    doc.requireUnique(m_nodeId);
    DocNode n;
    n.id = m_nodeId;
    n.kind = NodeKind::Frame;
    n.bounds = m_bounds;
    doc.rootChildren.push_back(std::move(n));
    return {true, {}};
}

inline std::unique_ptr<DocEdit> CreateFrameEdit::generateUndo(const DeviceDocument &) const
{
    return makeRemoveEdit(m_id, m_nodeId);
}

inline JsonValue CreateFrameEdit::serialize() const
{
    return envelope(JsonValue::object({
        {"id", JsonValue::string(m_nodeId)},
        {"bounds", aabbToJson(m_bounds)},
    }));
}

inline std::unique_ptr<DocEdit> CreateFrameEdit::clone() const
{
    return std::make_unique<CreateFrameEdit>(*this);
}

inline std::unique_ptr<CreateFrameEdit> CreateFrameEdit::fromPayload(const JsonValue &envelope,
                                                                    const JsonValue &payload)
{
    auto e = std::make_unique<CreateFrameEdit>();
    fillMeta(*e, envelope);
    e->setNodeId(payload.getString("id"));
    e->setBounds(aabbFromJson(payload.get("bounds")));
    return e;
}

} // namespace document
} // namespace epaper
