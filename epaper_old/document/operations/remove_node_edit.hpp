#pragma once
/**
 * @implements [SRS-EP-07] remove_node
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"
#include "reparent_edit.hpp"

#include <stdexcept>

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

inline std::unique_ptr<RemoveNodeEdit> makeRemoveEdit(const std::string &opId,
                                                      const std::string &nodeId)
{
    auto e = std::make_unique<RemoveNodeEdit>(nodeId);
    e->setId(opId);
    e->setUndo(true);
    return e;
}

inline ApplyResult RemoveNodeEdit::doApply(DeviceDocument &doc)
{
    if (!doc.removeNodeId(m_nodeId))
        throw std::runtime_error(std::string("missing:") + m_nodeId);
    return {true, {}};
}

inline std::unique_ptr<DocEdit> RemoveNodeEdit::generateUndo(const DeviceDocument &doc) const
{
    auto e = makeRestoreEdit(doc, m_nodeId, m_id);
    if (!e)
        return std::make_unique<RemoveNodeEdit>(m_nodeId);
    return e;
}

inline JsonValue RemoveNodeEdit::serialize() const
{
    return envelope(JsonValue::object({{"id", JsonValue::string(m_nodeId)}}));
}

inline std::unique_ptr<DocEdit> RemoveNodeEdit::clone() const
{
    auto e = std::make_unique<RemoveNodeEdit>(*this);
    return e;
}

inline std::unique_ptr<RemoveNodeEdit> RemoveNodeEdit::fromPayload(const JsonValue &envelope,
                                                                  const JsonValue &payload)
{
    auto e = std::make_unique<RemoveNodeEdit>(payload.getString("id"));
    fillMeta(*e, envelope);
    return e;
}

inline std::unique_ptr<DocEdit> ReparentEdit::generateUndo(const DeviceDocument &doc) const
{
    auto e = makeRestoreEdit(doc, m_nodeId, m_id);
    if (e)
        return e;
    /** Insert-via-restore (clipboard paste): inverse is remove. @implements [SRS-EP-31] */
    if (m_hasBody)
        return makeRemoveEdit(m_id, m_nodeId);
    return clone();
}

} // namespace document
} // namespace epaper
