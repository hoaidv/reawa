#pragma once
/**
 * DocEdit method bodies. Included after DeviceDocument is complete.
 * @implements [SRS-EP-07] op apply moved out of DeviceDocument
 */

#include "from_json.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace epaper {
namespace document {

inline void addTargetId(std::vector<std::string> &ids, const std::string &nodeId)
{
    if (nodeId.empty())
        return;
    for (const auto &id : ids) {
        if (id == nodeId)
            return;
    }
    ids.push_back(nodeId);
}

inline std::optional<std::string> parentIdFromJson(const JsonValue &p)
{
    const JsonValue *x = p.get("parentId");
    if (!x || !x->isString() || x->asString().empty())
        return std::nullopt;
    return x->asString();
}

inline std::vector<InkSample> samplesFromJsonArray(const JsonValue *a)
{
    std::vector<InkSample> out;
    if (!a || !a->isArray())
        return out;
    for (const auto &s : a->asArray())
        out.push_back(sampleFromJson(s));
    return out;
}

inline JsonValue samplesToJsonArray(const std::vector<InkSample> &ss)
{
    JsonValue::Array a;
    a.reserve(ss.size());
    for (const auto &s : ss)
        a.push_back(sampleToJson(s));
    return JsonValue::array(std::move(a));
}

inline void fillMeta(DocEdit &e, const JsonValue &envelope)
{
    e.setId(envelope.getString("opId"));
    const std::string src = envelope.getString("source");
    e.setSource(src.empty() ? "epaper" : src);
    e.setTimestamp(optNumber(envelope, "ts"));
}

inline std::unique_ptr<RemoveNodeEdit> makeRemoveEdit(const std::string &opId,
                                                      const std::string &nodeId)
{
    auto e = std::make_unique<RemoveNodeEdit>(nodeId);
    e->setId(opId);
    e->setUndo(true);
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
    if (n.samples.empty())
        throw std::runtime_error("missing_samples");
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
    e->setSamples(samplesFromJsonArray(payload.get("samples")));
    return e;
}

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

inline ApplyResult CreatePrimitiveEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    doc.requireUnique(m_nodeId);
    DocNode n;
    n.id = m_nodeId;
    n.kind = NodeKind::Primitive;
    n.style = m_style;
    n.geomKind = m_geomKind;
    n.x1 = m_x1;
    n.y1 = m_y1;
    n.x2 = m_x2;
    n.y2 = m_y2;
    n.gx = m_gx;
    n.gy = m_gy;
    n.gw = m_gw;
    n.gh = m_gh;
    n.cx = m_cx;
    n.cy = m_cy;
    n.rx = m_rx;
    n.ry = m_ry;
    doc.insertUnder(m_parentId, std::move(n));
    return {true, {}};
}

inline std::unique_ptr<DocEdit> CreatePrimitiveEdit::generateUndo(const DeviceDocument &) const
{
    return makeRemoveEdit(m_id, m_nodeId);
}

inline JsonValue CreatePrimitiveEdit::serialize() const
{
    JsonValue::Object geom;
    if (m_geomKind == PrimitiveKind::Line) {
        geom.emplace_back("kind", JsonValue::string("line"));
        geom.emplace_back("x1", JsonValue::number(m_x1));
        geom.emplace_back("y1", JsonValue::number(m_y1));
        geom.emplace_back("x2", JsonValue::number(m_x2));
        geom.emplace_back("y2", JsonValue::number(m_y2));
    } else if (m_geomKind == PrimitiveKind::Ellipse) {
        geom.emplace_back("kind", JsonValue::string("ellipse"));
        geom.emplace_back("cx", JsonValue::number(m_cx));
        geom.emplace_back("cy", JsonValue::number(m_cy));
        geom.emplace_back("rx", JsonValue::number(m_rx));
        geom.emplace_back("ry", JsonValue::number(m_ry));
    } else {
        geom.emplace_back("kind", JsonValue::string("rect"));
        geom.emplace_back("x", JsonValue::number(m_gx));
        geom.emplace_back("y", JsonValue::number(m_gy));
        geom.emplace_back("w", JsonValue::number(m_gw));
        geom.emplace_back("h", JsonValue::number(m_gh));
    }
    JsonValue::Object p;
    p.emplace_back("id", JsonValue::string(m_nodeId));
    if (m_parentId)
        p.emplace_back("parentId", JsonValue::string(*m_parentId));
    p.emplace_back("style", styleToJson(m_style));
    p.emplace_back("geom", JsonValue::object(std::move(geom)));
    return envelope(JsonValue::object(std::move(p)));
}

inline std::unique_ptr<DocEdit> CreatePrimitiveEdit::clone() const
{
    return std::make_unique<CreatePrimitiveEdit>(*this);
}

inline std::unique_ptr<CreatePrimitiveEdit>
CreatePrimitiveEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<CreatePrimitiveEdit>();
    fillMeta(*e, envelope);
    e->setNodeId(payload.getString("id"));
    e->setParentId(parentIdFromJson(payload));
    e->setStyle(styleFromJson(payload.get("style")));
    const JsonValue *geom = payload.get("geom");
    if (!geom || !geom->isObject())
        return e;
    const std::string gk = geom->getString("kind");
    if (gk == "line")
        e->setLine(geom->getNumber("x1"), geom->getNumber("y1"), geom->getNumber("x2"),
                   geom->getNumber("y2"));
    else if (gk == "ellipse")
        e->setEllipse(geom->getNumber("cx"), geom->getNumber("cy"), geom->getNumber("rx"),
                      geom->getNumber("ry"));
    else
        e->setRect(geom->getNumber("x"), geom->getNumber("y"), geom->getNumber("w"),
                   geom->getNumber("h"));
    return e;
}

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

inline void collectCapturedRestores(const DeviceDocument &doc, const std::vector<std::string> &ids,
                                    CompoundEdit &out, const std::string &forwardOpId)
{
    struct Cap {
        std::string id;
        DeviceDocument::NodePlace place;
        DocNode body;
    };
    std::vector<Cap> caps;
    auto consider = [&](const std::string &cid) {
        const DocNode *n = doc.find(cid);
        DeviceDocument::NodePlace pl;
        if (!n || !doc.findPlace(cid, &pl))
            return;
        for (const auto &c : caps) {
            if (c.id == cid)
                return;
        }
        caps.push_back({cid, pl, *n});
    };
    for (const auto &cid : ids)
        consider(cid);
    std::sort(caps.begin(), caps.end(),
              [](const Cap &a, const Cap &b) { return a.place.index < b.place.index; });
    for (const auto &c : caps) {
        auto r = std::make_unique<ReparentEdit>(
            ReparentEdit::restore(c.id, c.place.parentId, c.place.index, c.body));
        r->setId(forwardOpId);
        r->setUndo(true);
        out.addPart(std::move(r));
    }
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

inline ApplyResult CreateSmartGroupEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    doc.requireUnique(m_nodeId);
    for (const auto &cid : m_captureIds) {
        DocNode discarded;
        if (!doc.detachInk(cid, &discarded))
            throw std::runtime_error(std::string("capture_missing:") + cid);
    }
    DocNode n;
    n.id = m_nodeId;
    n.kind = NodeKind::SmartGroup;
    n.smartBounds = m_bounds;
    n.transform = m_transform;
    n.inkScaleMode = m_inkScaleMode.empty() ? "fixedInk" : m_inkScaleMode;
    n.children = m_children;
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
    return e;
}

inline ApplyResult JoinSmartGroupEdit::doApply(DeviceDocument &doc)
{
    DocNode *sg = doc.mutableFind(m_smartGroupId);
    if (!sg || sg->kind != NodeKind::SmartGroup)
        throw std::runtime_error(std::string("not_smart_group:") + m_smartGroupId);
    DocNode detached;
    if (!doc.detachInk(m_inkId, &detached))
        throw std::runtime_error(std::string("join_missing:") + m_inkId);
    const SmartTransform &t = sg->transform;
    const bool fixedInk = sg->inkScaleMode == "fixedInk";
    const double sx = t.scaleX != 0 ? t.scaleX : 1.0;
    const double sy = t.scaleY != 0 ? t.scaleY : 1.0;
    for (auto &s : detached.samples) {
        if (fixedInk) {
            s.x = s.x - t.x;
            s.y = s.y - t.y;
        } else {
            s.x = (s.x - t.x) / sx;
            s.y = (s.y - t.y) / sy;
        }
    }
    detached.role = "content";
    const double w = sg->smartBounds.width != 0 ? sg->smartBounds.width : 1.0;
    const double h = sg->smartBounds.height != 0 ? sg->smartBounds.height : 1.0;
    double minX = std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    for (const auto &s : detached.samples) {
        minX = std::min(minX, s.x);
        minY = std::min(minY, s.y);
        maxX = std::max(maxX, s.x);
        maxY = std::max(maxY, s.y);
    }
    const double cx = std::isfinite(minX) ? (minX + maxX) / 2.0 : 0.0;
    const double cy = std::isfinite(minY) ? (minY + maxY) / 2.0 : 0.0;
    detached.layoutOffset = {(cx - sg->smartBounds.x) / w, (cy - sg->smartBounds.y) / h};
    sg->children.push_back(std::move(detached));
    return {true, {}};
}

inline std::unique_ptr<DocEdit> JoinSmartGroupEdit::generateUndo(const DeviceDocument &doc) const
{
    auto e = makeRestoreEdit(doc, m_inkId, m_id);
    if (e)
        return e;
    return clone();
}

inline JsonValue JoinSmartGroupEdit::serialize() const
{
    return envelope(JsonValue::object({
        {"inkId", JsonValue::string(m_inkId)},
        {"smartGroupId", JsonValue::string(m_smartGroupId)},
    }));
}

inline std::unique_ptr<DocEdit> JoinSmartGroupEdit::clone() const
{
    return std::make_unique<JoinSmartGroupEdit>(*this);
}

inline std::unique_ptr<JoinSmartGroupEdit>
JoinSmartGroupEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<JoinSmartGroupEdit>(payload.getString("inkId"),
                                                  payload.getString("smartGroupId"));
    fillMeta(*e, envelope);
    return e;
}

inline std::unique_ptr<SetSmartTransformEdit>
SetSmartTransformEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<SetSmartTransformEdit>();
    fillMeta(*e, envelope);
    e->setNodeId(payload.getString("id"));
    if (const JsonValue *t = payload.get("transform"))
        e->setTo(transformFromJson(t), nullptr);
    if (const JsonValue *b = payload.get("bounds"); b && b->isObject()) {
        const SmartBounds bb = boundsFromJson(b);
        e->setTo(e->toT(), &bb);
    }
    return e;
}

inline ApplyResult SetSmartTransformEdit::doApply(DeviceDocument &doc)
{
    DocNode *node = doc.mutableFind(m_nodeId);
    if (!node || node->kind != NodeKind::SmartGroup)
        throw std::runtime_error(std::string("not_smart_group:") + m_nodeId);
    node->transform = m_toT;
    if (m_hasToBounds)
        node->smartBounds = m_toB;
    return {true, {}};
}

inline std::unique_ptr<DocEdit> SetSmartTransformEdit::generateUndo(const DeviceDocument &doc) const
{
    auto u = std::make_unique<SetSmartTransformEdit>();
    u->setId(m_id);
    u->setUndo(true);
    u->setNodeId(m_nodeId);
    if (m_hasFrom) {
        u->setTo(m_fromT, &m_fromB);
        u->setFrom(m_toT, m_toB);
        return u;
    }
    const DocNode *n = doc.find(m_nodeId);
    if (n)
        u->setTo(n->transform, &n->smartBounds);
    return u;
}

inline JsonValue SetSmartTransformEdit::serialize() const
{
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(m_nodeId));
    payload.emplace_back("transform", transformToJson(m_toT));
    if (m_hasToBounds)
        payload.emplace_back("bounds", boundsToJson(m_toB));
    return envelope(JsonValue::object(std::move(payload)));
}

inline std::unique_ptr<DocEdit> SetSmartTransformEdit::clone() const
{
    return std::make_unique<SetSmartTransformEdit>(*this);
}

inline ApplyResult SetInkScaleModeEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    DocNode *node = doc.mutableFind(m_nodeId);
    if (!node || node->kind != NodeKind::SmartGroup)
        throw std::runtime_error(std::string("not_smart_group:") + m_nodeId);
    if (m_mode != "withBounds" && m_mode != "fixedInk")
        throw std::runtime_error(std::string("bad_ink_scale_mode:") + m_mode);
    node->inkScaleMode = m_mode;
    return {true, {}};
}

inline std::unique_ptr<DocEdit> SetInkScaleModeEdit::generateUndo(const DeviceDocument &doc) const
{
    auto u = std::make_unique<SetInkScaleModeEdit>();
    u->setId(m_id);
    u->setUndo(true);
    const DocNode *n = doc.find(m_nodeId);
    const std::string old = m_hasOld ? m_oldMode : (n ? n->inkScaleMode : std::string("fixedInk"));
    u->setNodeId(m_nodeId);
    u->setMode(old);
    return u;
}

inline JsonValue SetInkScaleModeEdit::serialize() const
{
    return envelope(JsonValue::object({
        {"id", JsonValue::string(m_nodeId)},
        {"inkScaleMode", JsonValue::string(m_mode)},
    }));
}

inline std::unique_ptr<DocEdit> SetInkScaleModeEdit::clone() const
{
    return std::make_unique<SetInkScaleModeEdit>(*this);
}

inline std::unique_ptr<SetInkScaleModeEdit>
SetInkScaleModeEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<SetInkScaleModeEdit>(payload.getString("id"),
                                                   payload.getString("inkScaleMode"));
    fillMeta(*e, envelope);
    return e;
}

inline ApplyResult SetInkSamplesEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    DocNode *n = doc.mutableFind(m_nodeId);
    if (!n || n->kind != NodeKind::Ink)
        throw std::runtime_error(std::string("not_ink:") + m_nodeId);
    n->samples = m_samples;
    return {true, {}};
}

inline std::unique_ptr<DocEdit> SetInkSamplesEdit::generateUndo(const DeviceDocument &doc) const
{
    auto u = std::make_unique<SetInkSamplesEdit>();
    u->setId(m_id);
    u->setUndo(true);
    u->setNodeId(m_nodeId);
    const DocNode *n = doc.find(m_nodeId);
    if (n)
        u->setSamples(n->samples);
    return u;
}

inline JsonValue SetInkSamplesEdit::serialize() const
{
    return envelope(JsonValue::object({
        {"id", JsonValue::string(m_nodeId)},
        {"samples", samplesToJsonArray(m_samples)},
    }));
}

inline std::unique_ptr<DocEdit> SetInkSamplesEdit::clone() const
{
    return std::make_unique<SetInkSamplesEdit>(*this);
}

inline std::unique_ptr<SetInkSamplesEdit>
SetInkSamplesEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<SetInkSamplesEdit>(payload.getString("id"),
                                                 samplesFromJsonArray(payload.get("samples")));
    fillMeta(*e, envelope);
    return e;
}

inline ApplyResult CompoundEdit::doApply(DeviceDocument &doc)
{
    for (auto &p : m_parts) {
        if (!p)
            continue;
        const ApplyResult r = p->doApply(doc);
        if (!r.applied)
            throw std::runtime_error(r.reason.empty() ? "compound_part_failed" : r.reason);
    }
    return {true, {}};
}

inline std::unique_ptr<DocEdit> CompoundEdit::generateUndo(const DeviceDocument &doc) const
{
    auto u = std::make_unique<CompoundEdit>();
    u->setId(m_id);
    u->setUndo(true);
    for (auto it = m_parts.rbegin(); it != m_parts.rend(); ++it) {
        if (*it)
            u->addPart((*it)->generateUndo(doc));
    }
    return u;
}

inline JsonValue CompoundEdit::serialize() const
{
    JsonValue::Array ops;
    for (const auto &p : m_parts) {
        if (p)
            ops.push_back(p->serialize());
    }
    return envelope(JsonValue::object({{"ops", JsonValue::array(std::move(ops))}}));
}

inline std::unique_ptr<DocEdit> CompoundEdit::clone() const
{
    auto e = std::make_unique<CompoundEdit>();
    copyMetaTo(*e);
    for (const auto &p : m_parts) {
        if (p)
            e->addPart(p->clone());
    }
    return e;
}

inline std::vector<std::string> CompoundEdit::targets() const
{
    std::vector<std::string> ts;
    for (const auto &p : m_parts) {
        if (!p)
            continue;
        for (const auto &id : p->targets())
            addTargetId(ts, id);
    }
    return ts;
}

inline std::unique_ptr<CompoundEdit> CompoundEdit::fromPayload(const JsonValue &envelope,
                                                              const JsonValue &payload)
{
    auto e = std::make_unique<CompoundEdit>();
    fillMeta(*e, envelope);
    if (const JsonValue *ops = payload.get("ops"); ops && ops->isArray()) {
        for (const auto &j : ops->asArray())
            e->addPart(DocEdit::fromJson(j));
    }
    return e;
}

} // namespace document
} // namespace epaper
