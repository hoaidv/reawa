#pragma once
/**
 * @implements [SRS-EP-07] append_ink
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"
#include "remove_node_edit.hpp"

#include <stdexcept>
#include <utility>

namespace epaper {
namespace document {

class AppendInkEdit final : public DocEdit {
public:
    AppendInkEdit() = default;
    AppendInkEdit(std::string nodeId, std::vector<InkSample> samples, Style style = {})
        : m_nodeId(std::move(nodeId))
        , m_style(std::move(style))
        , m_samples(std::move(samples))
    {
    }

    const char *kind() const override { return edit_kind::kAppendInk; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setParentId(std::optional<std::string> p) { m_parentId = std::move(p); }
    void setStyle(Style s) { m_style = std::move(s); }
    void setRole(std::optional<std::string> r) { m_role = std::move(r); }
    void setSamples(std::vector<InkSample> s) { m_samples = std::move(s); }
    void setInsertIndex(int i) { m_insertIndex = i; }
    void setInsertAfter(std::string id) { m_insertAfter = std::move(id); }
    void setLayoutOffset(std::optional<std::pair<double, double>> uv)
    {
        m_layoutOffset = std::move(uv);
    }

    const std::string &nodeId() const { return m_nodeId; }
    const std::optional<std::string> &parentId() const { return m_parentId; }

    static std::unique_ptr<AppendInkEdit> fromPayload(const JsonValue &envelope,
                                                      const JsonValue &payload);

private:
    std::string m_nodeId;
    std::optional<std::string> m_parentId;
    Style m_style;
    std::optional<std::string> m_role;
    std::vector<InkSample> m_samples;
    std::optional<int> m_insertIndex;
    std::optional<std::string> m_insertAfter;
    std::optional<std::pair<double, double>> m_layoutOffset;
};

inline ApplyResult AppendInkEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    doc.requireUnique(m_nodeId);
    DocNode n;
    n.id = m_nodeId;
    n.kind = NodeKind::Ink;
    n.style = m_style;
    n.role = m_role;
    n.samples = m_samples;
    n.layoutOffset = m_layoutOffset;
    if (n.samples.empty())
        throw std::runtime_error("missing_samples");
    std::string parent = m_parentId.value_or("");
    int index = m_insertIndex.value_or(-1);
    if (m_insertAfter && !m_insertAfter->empty()) {
        DeviceDocument::NodePlace place;
        if (doc.findPlace(*m_insertAfter, &place)) {
            parent = place.parentId;
            index = place.index + 1;
        }
    }
    if (index >= 0) {
        doc.insertAt(parent, index, std::move(n));
        return {true, {}};
    }
    const DocNode *p = parent.empty() ? nullptr : doc.find(parent);
    if (p && p->kind == NodeKind::SmartGroup)
        doc.insertAt(parent, static_cast<int>(p->children.size()), std::move(n));
    else
        doc.insertUnder(m_parentId, std::move(n));
    return {true, {}};
}

inline std::unique_ptr<DocEdit> AppendInkEdit::generateUndo(const DeviceDocument &) const
{
    return makeRemoveEdit(m_id, m_nodeId);
}

inline JsonValue AppendInkEdit::serialize() const
{
    JsonValue::Object p;
    p.emplace_back("id", JsonValue::string(m_nodeId));
    if (m_parentId)
        p.emplace_back("parentId", JsonValue::string(*m_parentId));
    p.emplace_back("samples", samplesToJsonArray(m_samples));
    p.emplace_back("style", styleToJson(m_style));
    if (m_role)
        p.emplace_back("role", JsonValue::string(*m_role));
    if (m_insertIndex)
        p.emplace_back("index", JsonValue::number(static_cast<double>(*m_insertIndex)));
    if (m_insertAfter)
        p.emplace_back("insertAfter", JsonValue::string(*m_insertAfter));
    if (m_layoutOffset) {
        JsonValue::Object lo;
        lo.emplace_back("u", JsonValue::number(m_layoutOffset->first));
        lo.emplace_back("v", JsonValue::number(m_layoutOffset->second));
        p.emplace_back("layoutOffset", JsonValue::object(std::move(lo)));
    }
    return envelope(JsonValue::object(std::move(p)));
}

inline std::unique_ptr<DocEdit> AppendInkEdit::clone() const
{
    return std::make_unique<AppendInkEdit>(*this);
}

inline std::unique_ptr<AppendInkEdit> AppendInkEdit::fromPayload(const JsonValue &envelope,
                                                                const JsonValue &payload)
{
    auto e = std::make_unique<AppendInkEdit>();
    fillMeta(*e, envelope);
    e->setNodeId(payload.getString("id"));
    e->setParentId(parentIdFromJson(payload));
    e->setStyle(styleFromJson(payload.get("style")));
    if (const JsonValue *role = payload.get("role"); role && role->isString())
        e->setRole(role->asString());
    if (const JsonValue *idx = payload.get("index"); idx && idx->isNumber())
        e->setInsertIndex(static_cast<int>(idx->asNumber()));
    if (const JsonValue *after = payload.get("insertAfter"); after && after->isString())
        e->setInsertAfter(after->asString());
    if (const JsonValue *lo = payload.get("layoutOffset"); lo && lo->isObject())
        e->setLayoutOffset(std::make_pair(lo->getNumber("u"), lo->getNumber("v")));
    e->setSamples(samplesFromJsonArray(payload.get("samples")));
    return e;
}

} // namespace document
} // namespace epaper
