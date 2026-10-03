#pragma once
/**
 * @implements [SRS-EP-07] create_connector
 * @implements [SRS-EP-17] commit envelope
 */

#include "compound_edit.hpp"
#include "doc_edit.hpp"
#include "edit_helpers.hpp"
#include "remove_node_edit.hpp"

#include <stdexcept>

namespace epaper {
namespace document {

class CreateConnectorEdit final : public DocEdit {
public:
    CreateConnectorEdit() = default;

    const char *kind() const override { return edit_kind::kCreateConnector; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setParentId(std::optional<std::string> p) { m_parentId = std::move(p); }
    void setFrom(ConnectorAnchor a) { m_from = std::move(a); }
    void setTo(ConnectorAnchor a) { m_to = std::move(a); }
    void setWarpStyle(std::string s) { m_warpStyle = std::move(s); }
    void setRestSpine(std::vector<ConnectorRestPt> s) { m_restSpine = std::move(s); }
    void setRestOffsets(std::vector<ConnectorRestOff> o) { m_restOffsets = std::move(o); }
    void setCaptureIds(std::vector<std::string> ids) { m_captureIds = std::move(ids); }
    void setFromPose(ConnectorEndPose p) { m_fromPose = p; }
    void setToPose(ConnectorEndPose p) { m_toPose = p; }
    const std::string &nodeId() const { return m_nodeId; }
    const std::vector<std::string> &captureIds() const { return m_captureIds; }

    static std::unique_ptr<CreateConnectorEdit> fromPayload(const JsonValue &envelope,
                                                            const JsonValue &payload);

private:
    std::string m_nodeId;
    std::optional<std::string> m_parentId;
    ConnectorAnchor m_from;
    ConnectorAnchor m_to;
    std::string m_warpStyle = "morph";
    std::vector<ConnectorRestPt> m_restSpine;
    std::vector<ConnectorRestOff> m_restOffsets;
    std::vector<std::string> m_captureIds;
    ConnectorEndPose m_fromPose;
    ConnectorEndPose m_toPose;
};

inline ApplyResult CreateConnectorEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    doc.requireUnique(m_nodeId);
    DocNode n;
    n.id = m_nodeId;
    n.kind = NodeKind::Connector;
    n.fromAnchor = m_from;
    n.toAnchor = m_to;
    n.fromNodeId = n.fromAnchor.nodeId;
    n.toNodeId = n.toAnchor.nodeId;
    n.warpStyle = m_warpStyle.empty() ? "morph" : m_warpStyle;
    n.restSpine = m_restSpine;
    n.restOffsets = m_restOffsets;
    for (const auto &cid : m_captureIds) {
        DocNode ink;
        if (!doc.detachInk(cid, &ink))
            throw std::runtime_error(std::string("capture_missing:") + cid);
        n.children.push_back(std::move(ink));
    }
    n.connectorInvalid = false;
    n.fromPose = m_fromPose;
    n.toPose = m_toPose;
    doc.insertUnder(m_parentId, std::move(n));
    return {true, {}};
}

inline std::unique_ptr<DocEdit> CreateConnectorEdit::generateUndo(const DeviceDocument &doc) const
{
    auto compound = std::make_unique<CompoundEdit>();
    compound->setId(m_id);
    collectCapturedRestores(doc, m_captureIds, *compound, m_id);
    compound->addPart(makeRemoveEdit(m_id, m_nodeId));
    if (compound->parts().size() == 1)
        return makeRemoveEdit(m_id, m_nodeId);
    return compound;
}

inline JsonValue CreateConnectorEdit::serialize() const
{
    JsonValue::Object rest;
    JsonValue::Array spine;
    for (const auto &pt : m_restSpine) {
        JsonValue::Object o;
        o.emplace_back("x", JsonValue::number(pt.x));
        o.emplace_back("y", JsonValue::number(pt.y));
        spine.push_back(JsonValue::object(std::move(o)));
    }
    JsonValue::Array offs;
    for (const auto &pt : m_restOffsets) {
        JsonValue::Object o;
        o.emplace_back("s", JsonValue::number(pt.s));
        o.emplace_back("d", JsonValue::number(pt.d));
        offs.push_back(JsonValue::object(std::move(o)));
    }
    rest.emplace_back("spine", JsonValue::array(std::move(spine)));
    rest.emplace_back("offsets", JsonValue::array(std::move(offs)));
    JsonValue::Array cap;
    for (const auto &id : m_captureIds)
        cap.push_back(JsonValue::string(id));
    JsonValue::Object p;
    p.emplace_back("id", JsonValue::string(m_nodeId));
    if (m_parentId)
        p.emplace_back("parentId", JsonValue::string(*m_parentId));
    p.emplace_back("from", DeviceDocument::anchorToJson(m_from));
    p.emplace_back("to", DeviceDocument::anchorToJson(m_to));
    p.emplace_back("warpStyle", JsonValue::string(m_warpStyle));
    p.emplace_back("restShape", JsonValue::object(std::move(rest)));
    p.emplace_back("captureIds", JsonValue::array(std::move(cap)));
    if (m_fromPose.valid)
        p.emplace_back("fromPose", DeviceDocument::poseToJson(m_fromPose));
    if (m_toPose.valid)
        p.emplace_back("toPose", DeviceDocument::poseToJson(m_toPose));
    return envelope(JsonValue::object(std::move(p)));
}

inline std::unique_ptr<DocEdit> CreateConnectorEdit::clone() const
{
    return std::make_unique<CreateConnectorEdit>(*this);
}

inline std::unique_ptr<CreateConnectorEdit>
CreateConnectorEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<CreateConnectorEdit>();
    fillMeta(*e, envelope);
    e->setNodeId(payload.getString("id"));
    e->setParentId(parentIdFromJson(payload));
    e->setFrom(DeviceDocument::anchorFromJson(payload.get("from")));
    e->setTo(DeviceDocument::anchorFromJson(payload.get("to")));
    e->setWarpStyle(payload.getString("warpStyle", "morph"));
    DocNode tmp;
    DeviceDocument::fillConnectorRest(tmp, payload);
    e->setRestSpine(tmp.restSpine);
    e->setRestOffsets(tmp.restOffsets);
    std::vector<std::string> caps;
    if (const JsonValue *ids = payload.get("captureIds"); ids && ids->isArray()) {
        for (const auto &idv : ids->asArray()) {
            if (idv.isString())
                caps.push_back(idv.asString());
        }
    }
    e->setCaptureIds(std::move(caps));
    e->setFromPose(DeviceDocument::poseFromJson(payload.get("fromPose")));
    e->setToPose(DeviceDocument::poseFromJson(payload.get("toPose")));
    return e;
}

} // namespace document
} // namespace epaper
