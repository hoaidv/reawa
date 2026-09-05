#pragma once
/**
 * @implements [SRS-EP-07] create_smart_group
 * @implements [SRS-EP-10] reparent capture into Smart Group
 * @implements [SRS-EP-75] captureIds may name Ink or SmartGroup
 */

#include "compound_edit.hpp"
#include "doc_edit.hpp"
#include "edit_helpers.hpp"
#include "remove_node_edit.hpp"

#include <stdexcept>

namespace epaper {
namespace document {

class CreateSmartGroupEdit final : public DocEdit {
public:
    CreateSmartGroupEdit() = default;

    const char *kind() const override { return edit_kind::kCreateSmartGroup; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override;

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setParentId(std::optional<std::string> p) { m_parentId = std::move(p); }
    void setBounds(SmartBounds b) { m_bounds = b; }
    void setTransform(SmartTransform t) { m_transform = t; }
    void setInkScaleMode(std::string m) { m_inkScaleMode = std::move(m); }
    void setCaptureIds(std::vector<std::string> ids) { m_captureIds = std::move(ids); }
    void setChildren(std::vector<DocNode> ch) { m_children = std::move(ch); }
    void setBoundaryPolyline(std::vector<InkSample> p) { m_boundaryPolyline = std::move(p); }

    const std::string &nodeId() const { return m_nodeId; }
    const std::vector<std::string> &captureIds() const { return m_captureIds; }
    const std::vector<DocNode> &children() const { return m_children; }

    static std::unique_ptr<CreateSmartGroupEdit> fromPayload(const JsonValue &envelope,
                                                             const JsonValue &payload);

private:
    std::string m_nodeId;
    std::optional<std::string> m_parentId;
    SmartBounds m_bounds;
    SmartTransform m_transform;
    std::string m_inkScaleMode = "fixedInk";
    std::vector<std::string> m_captureIds;
    std::vector<DocNode> m_children;
    std::vector<InkSample> m_boundaryPolyline;
};

inline ApplyResult CreateSmartGroupEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    doc.requireUnique(m_nodeId);
    for (const auto &cid : m_captureIds) {
        DocNode discarded;
        if (!doc.detachAny(cid, &discarded))
            throw std::runtime_error(std::string("capture_missing:") + cid);
    }
    DocNode n;
    n.id = m_nodeId;
    n.kind = NodeKind::SmartGroup;
    n.smartBounds = m_bounds;
    n.transform = m_transform;
    n.inkScaleMode = m_inkScaleMode.empty() ? "fixedInk" : m_inkScaleMode;
    n.children = m_children;
    n.boundaryPolyline = m_boundaryPolyline;
    if (n.boundaryPolyline.empty()) {
        for (const auto &c : n.children) {
            const std::string role = c.role ? *c.role : std::string();
            if (role == "boundary" && c.samples.size() >= 2) {
                n.boundaryPolyline = closedPolylineCopy(c.samples);
                break;
            }
        }
    }
    doc.insertUnder(m_parentId, std::move(n));
    return {true, {}};
}

inline std::vector<std::string> CreateSmartGroupEdit::targets() const
{
    std::vector<std::string> ts;
    addTargetId(ts, m_nodeId);
    for (const auto &c : m_children)
        addTargetId(ts, c.id);
    for (const auto &id : m_captureIds)
        addTargetId(ts, id);
    return ts;
}

inline std::unique_ptr<DocEdit> CreateSmartGroupEdit::generateUndo(const DeviceDocument &doc) const
{
    auto compound = std::make_unique<CompoundEdit>();
    compound->setId(m_id);
    std::vector<std::string> ids = m_captureIds;
    for (const auto &c : m_children)
        addTargetId(ids, c.id);
    collectCapturedRestores(doc, ids, *compound, m_id);
    compound->addPart(makeRemoveEdit(m_id, m_nodeId));
    if (compound->parts().size() == 1)
        return makeRemoveEdit(m_id, m_nodeId);
    return compound;
}

inline JsonValue CreateSmartGroupEdit::serialize() const
{
    JsonValue::Array children;
    for (const auto &c : m_children)
        children.push_back(DeviceDocument::nodeToJson(c));
    JsonValue::Array cap;
    for (const auto &id : m_captureIds)
        cap.push_back(JsonValue::string(id));
    JsonValue::Object p;
    p.emplace_back("id", JsonValue::string(m_nodeId));
    if (m_parentId)
        p.emplace_back("parentId", JsonValue::string(*m_parentId));
    p.emplace_back("bounds", boundsToJson(m_bounds));
    p.emplace_back("transform", transformToJson(m_transform));
    p.emplace_back("inkScaleMode", JsonValue::string(m_inkScaleMode));
    p.emplace_back("captureIds", JsonValue::array(std::move(cap)));
    p.emplace_back("children", JsonValue::array(std::move(children)));
    if (!m_boundaryPolyline.empty())
        p.emplace_back("boundaryPolyline", samplesToJsonArray(m_boundaryPolyline));
    return envelope(JsonValue::object(std::move(p)));
}

inline std::unique_ptr<DocEdit> CreateSmartGroupEdit::clone() const
{
    return std::make_unique<CreateSmartGroupEdit>(*this);
}

inline std::unique_ptr<CreateSmartGroupEdit>
CreateSmartGroupEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<CreateSmartGroupEdit>();
    fillMeta(*e, envelope);
    e->setNodeId(payload.getString("id"));
    e->setParentId(parentIdFromJson(payload));
    e->setBounds(boundsFromJson(payload.get("bounds")));
    if (const JsonValue *t = payload.get("transform"); t && t->isObject())
        e->setTransform(transformFromJson(t));
    e->setInkScaleMode(payload.getString("inkScaleMode", "fixedInk"));
    std::vector<std::string> caps;
    if (const JsonValue *ids = payload.get("captureIds"); ids && ids->isArray()) {
        for (const auto &idv : ids->asArray()) {
            if (idv.isString())
                caps.push_back(idv.asString());
        }
    }
    e->setCaptureIds(std::move(caps));
    std::vector<DocNode> children;
    if (const JsonValue *ch = payload.get("children"); ch && ch->isArray()) {
        for (const auto &c : ch->asArray())
            children.push_back(DeviceDocument::nodeFromJson(c));
    }
    e->setChildren(std::move(children));
    if (const JsonValue *bp = payload.get("boundaryPolyline"); bp && bp->isArray())
        e->setBoundaryPolyline(samplesFromJsonArray(bp));
    return e;
}

} // namespace document
} // namespace epaper
