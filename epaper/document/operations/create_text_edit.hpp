#pragma once
/**
 * @implements [SRS-EP-07] create_text
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"
#include "remove_node_edit.hpp"

#include <stdexcept>

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

inline ApplyResult CreateTextEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    doc.requireUnique(m_nodeId);
    DocNode n;
    n.id = m_nodeId;
    n.kind = NodeKind::Text;
    n.box = m_box;
    n.style = m_style;
    n.runs = m_runs;
    if (n.runs.empty())
        n.runs.push_back(TextRun{});
    doc.insertUnder(m_parentId, std::move(n));
    return {true, {}};
}

inline std::unique_ptr<DocEdit> CreateTextEdit::generateUndo(const DeviceDocument &) const
{
    return makeRemoveEdit(m_id, m_nodeId);
}

inline JsonValue CreateTextEdit::serialize() const
{
    JsonValue::Object p;
    p.emplace_back("id", JsonValue::string(m_nodeId));
    if (m_parentId)
        p.emplace_back("parentId", JsonValue::string(*m_parentId));
    p.emplace_back("box", aabbToJson(m_box));
    p.emplace_back("style", styleToJson(m_style));
    JsonValue::Array runs;
    for (const auto &r : m_runs) {
        JsonValue::Object o;
        o.emplace_back("text", JsonValue::string(r.text));
        o.emplace_back("bold", JsonValue::boolean(r.bold));
        runs.push_back(JsonValue::object(std::move(o)));
    }
    p.emplace_back("runs", JsonValue::array(std::move(runs)));
    return envelope(JsonValue::object(std::move(p)));
}

inline std::unique_ptr<DocEdit> CreateTextEdit::clone() const
{
    return std::make_unique<CreateTextEdit>(*this);
}

inline std::unique_ptr<CreateTextEdit> CreateTextEdit::fromPayload(const JsonValue &envelope,
                                                                   const JsonValue &payload)
{
    auto e = std::make_unique<CreateTextEdit>();
    fillMeta(*e, envelope);
    e->setNodeId(payload.getString("id"));
    e->setParentId(parentIdFromJson(payload));
    e->setBox(aabbFromJson(payload.get("box")));
    e->setStyle(styleFromJson(payload.get("style")));
    std::vector<TextRun> runs;
    if (const JsonValue *rs = payload.get("runs"); rs && rs->isArray()) {
        for (const auto &r : rs->asArray()) {
            TextRun tr;
            tr.text = r.getString("text");
            const JsonValue *b = r.get("bold");
            tr.bold = b && b->isBool() && b->asBool();
            runs.push_back(tr);
        }
    }
    e->setRuns(std::move(runs));
    return e;
}

} // namespace document
} // namespace epaper
