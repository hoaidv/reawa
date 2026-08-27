#pragma once
/**
 * @implements [SRS-EP-07] reparent restore body + index
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"

#include <optional>
#include <stdexcept>

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

inline ReparentEdit ReparentEdit::restore(const std::string &id, const std::string &parentId,
                                         int index, DocNode nodeBody)
{
    ReparentEdit e;
    e.setNodeId(id);
    e.setNewParentId(parentId);
    e.setIndex(index);
    e.setBody(std::move(nodeBody));
    return e;
}

inline std::unique_ptr<ReparentEdit>
makeRestoreEdit(const DeviceDocument &doc, const std::string &id, const std::string &forwardOpId)
{
    DeviceDocument::NodePlace pl;
    const DocNode *n = doc.find(id);
    if (!n || !doc.findPlace(id, &pl))
        return nullptr;
    auto e = std::make_unique<ReparentEdit>(ReparentEdit::restore(id, pl.parentId, pl.index, *n));
    e->setId(forwardOpId);
    e->setUndo(true);
    return e;
}

inline ApplyResult ReparentEdit::doApply(DeviceDocument &doc)
{
    DocNode moved;
    if (m_hasBody) {
        moved = m_body;
        moved.id = m_nodeId;
        doc.detachAny(m_nodeId, nullptr);
    } else {
        DocNode *node = doc.mutableFind(m_nodeId);
        if (!node)
            throw std::runtime_error(std::string("missing:") + m_nodeId);
        moved = *node;
        if (!doc.detachAny(m_nodeId, nullptr))
            throw std::runtime_error(std::string("missing:") + m_nodeId);
    }
    if (m_index >= 0)
        doc.insertAt(m_newParentId, m_index, std::move(moved));
    else
        doc.insertUnder(m_newParentId.empty() ? std::nullopt
                                              : std::optional<std::string>(m_newParentId),
                        std::move(moved));
    return {true, {}};
}

inline std::unique_ptr<DocEdit> ReparentEdit::generateUndo(const DeviceDocument &doc) const
{
    auto e = makeRestoreEdit(doc, m_nodeId, m_id);
    if (e)
        return e;
    return clone();
}

inline JsonValue ReparentEdit::serialize() const
{
    JsonValue::Object p;
    p.emplace_back("id", JsonValue::string(m_nodeId));
    p.emplace_back("newParentId", JsonValue::string(m_newParentId));
    p.emplace_back("index", JsonValue::number(m_index));
    if (m_hasBody)
        p.emplace_back("node", DeviceDocument::nodeToJson(m_body));
    return envelope(JsonValue::object(std::move(p)));
}

inline std::unique_ptr<DocEdit> ReparentEdit::clone() const
{
    return std::make_unique<ReparentEdit>(*this);
}

inline std::unique_ptr<ReparentEdit> ReparentEdit::fromPayload(const JsonValue &envelope,
                                                              const JsonValue &payload)
{
    auto e = std::make_unique<ReparentEdit>();
    fillMeta(*e, envelope);
    e->setNodeId(payload.getString("id"));
    e->setNewParentId(payload.getString("newParentId"));
    int index = -1;
    if (const JsonValue *ix = payload.get("index"); ix && ix->isNumber())
        index = static_cast<int>(ix->asNumber());
    e->setIndex(index);
    if (const JsonValue *body = payload.get("node"); body && body->isObject())
        e->setBody(DeviceDocument::nodeFromJson(*body));
    return e;
}

} // namespace document
} // namespace epaper
