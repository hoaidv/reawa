#pragma once
/**
 * In-memory device document tree + SRS-IN-09 op apply.
 * @implements [SRS-EP-07] DeviceDocument tree and op apply
 * @implements [SRS-EP-09] device-local tree; double coords; sample retention
 *
 * Undo ring and publish queue are not here (STORY-EP-015 / STORY-EP-020).
 */

#include "json_value.hpp"

#include <algorithm>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace epaper {
namespace document {

struct Style {
    std::string stroke = "#1C2430";
    double strokeWidth = 2;
    std::optional<std::string> fill;
};

struct InkSample {
    double x = 0;
    double y = 0;
    std::optional<double> pressure;
    std::optional<double> tiltX;
    std::optional<double> tiltY;
    std::optional<double> t;
    std::optional<double> timestamp;
    std::optional<double> distance;
    std::map<std::string, JsonValue> extras;
};

struct Aabb {
    double minX = 0;
    double minY = 0;
    double maxX = 0;
    double maxY = 0;
};

struct SmartBounds {
    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;
};

struct SmartTransform {
    double x = 0;
    double y = 0;
    double rotation = 0;
    double scaleX = 1;
    double scaleY = 1;
};

enum class NodeKind {
    Ink,
    Text,
    Primitive,
    Group,
    Frame,
    Connector,
    SmartGroup,
};

enum class PrimitiveKind { Line, Rect, Ellipse };

struct TextRun {
    std::string text;
    bool bold = false;
};

struct DocNode {
    std::string id;
    NodeKind kind = NodeKind::Ink;
    Style style;
    std::vector<InkSample> samples;
    std::optional<std::string> role; // content | boundary
    std::optional<std::pair<double, double>> layoutOffset; // u, v
    Aabb box;
    Aabb bounds;
    std::vector<TextRun> runs;
    PrimitiveKind geomKind = PrimitiveKind::Rect;
    double gx = 0, gy = 0, gw = 0, gh = 0;
    double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    double cx = 0, cy = 0, rx = 0, ry = 0;
    SmartBounds smartBounds;
    SmartTransform transform;
    std::string inkScaleMode = "withBounds";
    std::string fromNodeId;
    std::string toNodeId;
    bool connectorInvalid = false;
    std::vector<DocNode> children;
};

struct DocOp {
    std::string opId;
    std::string type;
    std::string source;
    JsonValue payload;
    std::optional<double> ts;
};

struct ApplyResult {
    bool applied = false;
    std::string reason;
};

inline Style defaultStyle()
{
    return Style{};
}

inline Style styleFromJson(const JsonValue *p)
{
    Style s = defaultStyle();
    if (!p || !p->isObject())
        return s;
    if (p->has("stroke") && p->get("stroke")->isString())
        s.stroke = p->get("stroke")->asString();
    if (p->has("strokeWidth") && p->get("strokeWidth")->isNumber())
        s.strokeWidth = p->get("strokeWidth")->asNumber();
    if (p->has("fill") && p->get("fill")->isString())
        s.fill = p->get("fill")->asString();
    return s;
}

inline std::optional<double> optNumber(const JsonValue &obj, const char *key)
{
    const JsonValue *x = obj.get(key);
    if (!x || !x->isNumber())
        return std::nullopt;
    return x->asNumber();
}

inline InkSample sampleFromJson(const JsonValue &s)
{
    InkSample out;
    out.x = s.getNumber("x");
    out.y = s.getNumber("y");
    out.pressure = optNumber(s, "pressure");
    out.tiltX = optNumber(s, "tiltX");
    out.tiltY = optNumber(s, "tiltY");
    out.t = optNumber(s, "t");
    out.timestamp = optNumber(s, "timestamp");
    out.distance = optNumber(s, "distance");
    out.extras = extrasFromJson(s.get("extras"));
    return out;
}

inline JsonValue sampleToJson(const InkSample &s)
{
    JsonValue::Object o;
    o.emplace_back("x", JsonValue::number(s.x));
    o.emplace_back("y", JsonValue::number(s.y));
    auto pushOpt = [&](const char *k, const std::optional<double> &v) {
        if (v)
            o.emplace_back(k, JsonValue::number(*v));
    };
    pushOpt("pressure", s.pressure);
    pushOpt("tiltX", s.tiltX);
    pushOpt("tiltY", s.tiltY);
    pushOpt("t", s.t);
    pushOpt("timestamp", s.timestamp);
    pushOpt("distance", s.distance);
    if (!s.extras.empty()) {
        JsonValue::Object ex;
        for (const auto &kv : s.extras)
            ex.emplace_back(kv.first, kv.second);
        o.emplace_back("extras", JsonValue::object(std::move(ex)));
    }
    return JsonValue::object(std::move(o));
}

inline JsonValue styleToJson(const Style &s)
{
    JsonValue::Object o;
    o.emplace_back("stroke", JsonValue::string(s.stroke));
    o.emplace_back("strokeWidth", JsonValue::number(s.strokeWidth));
    if (s.fill)
        o.emplace_back("fill", JsonValue::string(*s.fill));
    return JsonValue::object(std::move(o));
}

inline Aabb aabbFromJson(const JsonValue *p)
{
    Aabb a;
    if (!p || !p->isObject())
        return a;
    a.minX = p->getNumber("minX");
    a.minY = p->getNumber("minY");
    a.maxX = p->getNumber("maxX");
    a.maxY = p->getNumber("maxY");
    return a;
}

inline JsonValue aabbToJson(const Aabb &a)
{
    JsonValue::Object o;
    o.emplace_back("minX", JsonValue::number(a.minX));
    o.emplace_back("minY", JsonValue::number(a.minY));
    o.emplace_back("maxX", JsonValue::number(a.maxX));
    o.emplace_back("maxY", JsonValue::number(a.maxY));
    return JsonValue::object(std::move(o));
}

/**
 * Working document. Session memory only.
 * @implements [SRS-EP-07] in-memory DeviceDocument
 */
class DeviceDocument {
public:
    std::vector<DocNode> rootChildren;
    std::string status = "open";

    ApplyResult applyOp(const DocOp &op)
    {
        if (m_applied.count(op.opId))
            return {false, "duplicate_opId"};
        try {
            if (op.type == "create_frame")
                opCreateFrame(op.payload);
            else if (op.type == "create_group")
                opCreateGroup(op.payload);
            else if (op.type == "create_text")
                opCreateText(op.payload);
            else if (op.type == "create_primitive")
                opCreatePrimitive(op.payload);
            else if (op.type == "append_ink")
                opAppendInk(op.payload);
            else if (op.type == "create_connector")
                opCreateConnector(op.payload);
            else if (op.type == "create_smart_group")
                opCreateSmartGroup(op.payload);
            else if (op.type == "set_smart_transform")
                opSetSmartTransform(op.payload);
            else if (op.type == "set_ink_scale_mode")
                opSetInkScaleMode(op.payload);
            else if (op.type == "reparent")
                opReparent(op.payload);
            else if (op.type == "remove_node")
                opRemoveNode(op.payload);
            else
                return {false, std::string("unknown_type:") + op.type};
        } catch (const std::exception &e) {
            return {false, e.what()};
        }
        m_applied.insert(op.opId);
        status = "dirty";
        return {true, {}};
    }

    const DocNode *find(const std::string &id) const { return findMut(id); }

    int inkCount() const
    {
        int n = 0;
        walk(rootChildren, [&](const DocNode &node) {
            if (node.kind == NodeKind::Ink)
                ++n;
        });
        return n;
    }

    int nodeCount() const
    {
        int n = 0;
        walk(rootChildren, [&](const DocNode &) { ++n; });
        return n;
    }

    std::vector<std::string> allIds() const
    {
        std::vector<std::string> ids;
        walk(rootChildren, [&](const DocNode &node) { ids.push_back(node.id); });
        return ids;
    }

    JsonValue toJSON() const
    {
        JsonValue::Object o;
        o.emplace_back("version", JsonValue::number(1));
        o.emplace_back("status", JsonValue::string(status));
        JsonValue::Array kids;
        for (const auto &c : rootChildren)
            kids.push_back(nodeToJson(c));
        o.emplace_back("rootChildren", JsonValue::array(std::move(kids)));
        return JsonValue::object(std::move(o));
    }

    std::string snapshotString() const { return stringify(toJSON()); }

    bool hasOp(const std::string &opId) const { return m_applied.count(opId) > 0; }

    /** Paint-order walk (sibling order). */
    template <typename Fn>
    void forEachPaintNode(Fn &&fn) const
    {
        walkPaint(rootChildren, fn);
    }

private:
    std::set<std::string> m_applied;

    template <typename Fn>
    static void walk(const std::vector<DocNode> &nodes, Fn &&fn)
    {
        for (const auto &n : nodes) {
            fn(n);
            if (!n.children.empty())
                walk(n.children, fn);
        }
    }

    template <typename Fn>
    static void walkPaint(const std::vector<DocNode> &nodes, Fn &&fn)
    {
        for (const auto &n : nodes) {
            fn(n);
            if (n.kind == NodeKind::Frame || n.kind == NodeKind::Group
                || n.kind == NodeKind::SmartGroup)
                walkPaint(n.children, fn);
        }
    }

    DocNode *findMut(const std::string &id) const
    {
        return findIn(const_cast<std::vector<DocNode> &>(rootChildren), id);
    }

    static DocNode *findIn(std::vector<DocNode> &nodes, const std::string &id)
    {
        for (auto &n : nodes) {
            if (n.id == id)
                return &n;
            if (auto *c = findIn(n.children, id))
                return c;
        }
        return nullptr;
    }

    void assertUniqueId(const std::string &id)
    {
        if (findMut(id))
            throw std::runtime_error(std::string("duplicate_id:") + id);
    }

    static bool isContainer(const DocNode *n)
    {
        return n && (n->kind == NodeKind::Frame || n->kind == NodeKind::Group);
    }

    void insertUnder(const std::optional<std::string> &parentId, DocNode node)
    {
        if (!parentId || parentId->empty()) {
            rootChildren.push_back(std::move(node));
            return;
        }
        DocNode *parent = findMut(*parentId);
        if (!isContainer(parent))
            throw std::runtime_error(std::string("bad_parent:") + *parentId);
        if (node.kind == NodeKind::Frame)
            throw std::runtime_error("frame_not_under_container");
        parent->children.push_back(std::move(node));
    }

    std::optional<std::string> parentIdOf(const JsonValue &p) const
    {
        const JsonValue *x = p.get("parentId");
        if (!x || !x->isString())
            return std::nullopt;
        return x->asString();
    }

    std::string requireId(const JsonValue &p) const
    {
        const JsonValue *x = p.get("id");
        if (!x || !x->isString())
            throw std::runtime_error("missing_id");
        return x->asString();
    }

    void opCreateFrame(const JsonValue &p)
    {
        const std::string id = requireId(p);
        assertUniqueId(id);
        DocNode n;
        n.id = id;
        n.kind = NodeKind::Frame;
        n.bounds = aabbFromJson(p.get("bounds"));
        rootChildren.push_back(std::move(n));
    }

    void opCreateGroup(const JsonValue &p)
    {
        const std::string id = requireId(p);
        assertUniqueId(id);
        DocNode n;
        n.id = id;
        n.kind = NodeKind::Group;
        insertUnder(parentIdOf(p), std::move(n));
    }

    void opCreateText(const JsonValue &p)
    {
        const std::string id = requireId(p);
        assertUniqueId(id);
        DocNode n;
        n.id = id;
        n.kind = NodeKind::Text;
        n.box = aabbFromJson(p.get("box"));
        n.style = styleFromJson(p.get("style"));
        if (const JsonValue *runs = p.get("runs"); runs && runs->isArray()) {
            for (const auto &r : runs->asArray()) {
                TextRun tr;
                tr.text = r.getString("text");
                const JsonValue *b = r.get("bold");
                tr.bold = b && b->isBool() && b->asBool();
                n.runs.push_back(tr);
            }
        } else {
            n.runs.push_back(TextRun{});
        }
        insertUnder(parentIdOf(p), std::move(n));
    }

    void opCreatePrimitive(const JsonValue &p)
    {
        const std::string id = requireId(p);
        assertUniqueId(id);
        DocNode n;
        n.id = id;
        n.kind = NodeKind::Primitive;
        n.style = styleFromJson(p.get("style"));
        const JsonValue *geom = p.get("geom");
        if (!geom || !geom->isObject())
            throw std::runtime_error("missing_geom");
        const std::string gk = geom->getString("kind");
        if (gk == "line") {
            n.geomKind = PrimitiveKind::Line;
            n.x1 = geom->getNumber("x1");
            n.y1 = geom->getNumber("y1");
            n.x2 = geom->getNumber("x2");
            n.y2 = geom->getNumber("y2");
        } else if (gk == "ellipse") {
            n.geomKind = PrimitiveKind::Ellipse;
            n.cx = geom->getNumber("cx");
            n.cy = geom->getNumber("cy");
            n.rx = geom->getNumber("rx");
            n.ry = geom->getNumber("ry");
        } else {
            n.geomKind = PrimitiveKind::Rect;
            n.gx = geom->getNumber("x");
            n.gy = geom->getNumber("y");
            n.gw = geom->getNumber("w");
            n.gh = geom->getNumber("h");
        }
        insertUnder(parentIdOf(p), std::move(n));
    }

    /** @implements [SRS-EP-07] append_ink — finished stroke → Ink node */
    void opAppendInk(const JsonValue &p)
    {
        const std::string id = requireId(p);
        assertUniqueId(id);
        DocNode n;
        n.id = id;
        n.kind = NodeKind::Ink;
        n.style = styleFromJson(p.get("style"));
        if (const JsonValue *role = p.get("role"); role && role->isString())
            n.role = role->asString();
        const JsonValue *samples = p.get("samples");
        if (!samples || !samples->isArray())
            throw std::runtime_error("missing_samples");
        for (const auto &s : samples->asArray())
            n.samples.push_back(sampleFromJson(s));
        insertUnder(parentIdOf(p), std::move(n));
    }

    void opCreateConnector(const JsonValue &p)
    {
        const std::string id = requireId(p);
        assertUniqueId(id);
        DocNode n;
        n.id = id;
        n.kind = NodeKind::Connector;
        const JsonValue *from = p.get("from");
        const JsonValue *to = p.get("to");
        n.fromNodeId = from && from->isObject() ? from->getString("nodeId") : std::string();
        n.toNodeId = to && to->isObject() ? to->getString("nodeId") : std::string();
        n.connectorInvalid = !findMut(n.fromNodeId) || !findMut(n.toNodeId);
        insertUnder(parentIdOf(p), std::move(n));
    }

    void opCreateSmartGroup(const JsonValue &p)
    {
        const std::string id = requireId(p);
        assertUniqueId(id);
        DocNode n;
        n.id = id;
        n.kind = NodeKind::SmartGroup;
        if (const JsonValue *b = p.get("bounds"); b && b->isObject()) {
            n.smartBounds.x = b->getNumber("x");
            n.smartBounds.y = b->getNumber("y");
            n.smartBounds.width = b->getNumber("width");
            n.smartBounds.height = b->getNumber("height");
        }
        if (const JsonValue *t = p.get("transform"); t && t->isObject()) {
            n.transform.x = t->getNumber("x");
            n.transform.y = t->getNumber("y");
            n.transform.rotation = t->getNumber("rotation");
            n.transform.scaleX = t->has("scaleX") ? t->getNumber("scaleX") : 1;
            n.transform.scaleY = t->has("scaleY") ? t->getNumber("scaleY") : 1;
        }
        // Infini apply fallback is fixedInk when the field is omitted.
        n.inkScaleMode = p.getString("inkScaleMode", "fixedInk");
        if (const JsonValue *ch = p.get("children"); ch && ch->isArray()) {
            for (const auto &c : ch->asArray()) {
                DocNode ink;
                ink.id = c.getString("id");
                ink.kind = NodeKind::Ink;
                ink.style = styleFromJson(c.get("style"));
                if (const JsonValue *role = c.get("role"); role && role->isString())
                    ink.role = role->asString();
                if (const JsonValue *samples = c.get("samples"); samples && samples->isArray()) {
                    for (const auto &s : samples->asArray())
                        ink.samples.push_back(sampleFromJson(s));
                }
                n.children.push_back(std::move(ink));
            }
        }
        insertUnder(parentIdOf(p), std::move(n));
    }

    void opSetSmartTransform(const JsonValue &p)
    {
        const std::string id = requireId(p);
        DocNode *node = findMut(id);
        if (!node || node->kind != NodeKind::SmartGroup)
            throw std::runtime_error(std::string("not_smart_group:") + id);
        if (const JsonValue *t = p.get("transform"); t && t->isObject()) {
            node->transform.x = t->getNumber("x");
            node->transform.y = t->getNumber("y");
            node->transform.rotation = t->getNumber("rotation");
            node->transform.scaleX = t->has("scaleX") ? t->getNumber("scaleX") : node->transform.scaleX;
            node->transform.scaleY = t->has("scaleY") ? t->getNumber("scaleY") : node->transform.scaleY;
        }
        if (const JsonValue *b = p.get("bounds"); b && b->isObject()) {
            node->smartBounds.x = b->getNumber("x");
            node->smartBounds.y = b->getNumber("y");
            node->smartBounds.width = b->getNumber("width");
            node->smartBounds.height = b->getNumber("height");
        }
    }

    void opSetInkScaleMode(const JsonValue &p)
    {
        const std::string id = requireId(p);
        DocNode *node = findMut(id);
        if (!node || node->kind != NodeKind::SmartGroup)
            throw std::runtime_error(std::string("not_smart_group:") + id);
        const std::string mode = p.getString("inkScaleMode");
        if (mode != "withBounds" && mode != "fixedInk")
            throw std::runtime_error(std::string("bad_ink_scale_mode:") + mode);
        node->inkScaleMode = mode;
    }

    /** @implements [SRS-EP-07] reparent (device closed set; not shipped as a gesture this story) */
    void opReparent(const JsonValue &p)
    {
        const std::string id = requireId(p);
        const std::string newParentId = p.getString("newParentId");
        DocNode *node = findMut(id);
        if (!node)
            throw std::runtime_error(std::string("missing:") + id);
        DocNode moved = *node;
        if (!removeId(rootChildren, id))
            throw std::runtime_error(std::string("missing:") + id);
        insertUnder(newParentId.empty() ? std::nullopt : std::optional<std::string>(newParentId),
                    std::move(moved));
    }

    void opRemoveNode(const JsonValue &p)
    {
        const std::string id = requireId(p);
        if (!removeId(rootChildren, id))
            throw std::runtime_error(std::string("missing:") + id);
    }

    static bool removeId(std::vector<DocNode> &nodes, const std::string &id)
    {
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].id == id) {
                nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(i));
                return true;
            }
            if (removeId(nodes[i].children, id))
                return true;
        }
        return false;
    }

    static JsonValue nodeToJson(const DocNode &n)
    {
        JsonValue::Object o;
        o.emplace_back("id", JsonValue::string(n.id));
        switch (n.kind) {
        case NodeKind::Ink: {
            o.emplace_back("kind", JsonValue::string("ink"));
            JsonValue::Array samples;
            for (const auto &s : n.samples)
                samples.push_back(sampleToJson(s));
            o.emplace_back("samples", JsonValue::array(std::move(samples)));
            o.emplace_back("style", styleToJson(n.style));
            if (n.role)
                o.emplace_back("role", JsonValue::string(*n.role));
            break;
        }
        case NodeKind::Text: {
            o.emplace_back("kind", JsonValue::string("text"));
            o.emplace_back("box", aabbToJson(n.box));
            JsonValue::Array runs;
            for (const auto &r : n.runs) {
                JsonValue::Object ro;
                ro.emplace_back("text", JsonValue::string(r.text));
                if (r.bold)
                    ro.emplace_back("bold", JsonValue::boolean(true));
                runs.push_back(JsonValue::object(std::move(ro)));
            }
            o.emplace_back("runs", JsonValue::array(std::move(runs)));
            o.emplace_back("style", styleToJson(n.style));
            break;
        }
        case NodeKind::Primitive: {
            o.emplace_back("kind", JsonValue::string("primitive"));
            JsonValue::Object g;
            if (n.geomKind == PrimitiveKind::Line) {
                g.emplace_back("kind", JsonValue::string("line"));
                g.emplace_back("x1", JsonValue::number(n.x1));
                g.emplace_back("y1", JsonValue::number(n.y1));
                g.emplace_back("x2", JsonValue::number(n.x2));
                g.emplace_back("y2", JsonValue::number(n.y2));
            } else if (n.geomKind == PrimitiveKind::Ellipse) {
                g.emplace_back("kind", JsonValue::string("ellipse"));
                g.emplace_back("cx", JsonValue::number(n.cx));
                g.emplace_back("cy", JsonValue::number(n.cy));
                g.emplace_back("rx", JsonValue::number(n.rx));
                g.emplace_back("ry", JsonValue::number(n.ry));
            } else {
                g.emplace_back("kind", JsonValue::string("rect"));
                g.emplace_back("x", JsonValue::number(n.gx));
                g.emplace_back("y", JsonValue::number(n.gy));
                g.emplace_back("w", JsonValue::number(n.gw));
                g.emplace_back("h", JsonValue::number(n.gh));
            }
            o.emplace_back("geom", JsonValue::object(std::move(g)));
            o.emplace_back("style", styleToJson(n.style));
            break;
        }
        case NodeKind::Group: {
            o.emplace_back("kind", JsonValue::string("group"));
            JsonValue::Array kids;
            for (const auto &c : n.children)
                kids.push_back(nodeToJson(c));
            o.emplace_back("children", JsonValue::array(std::move(kids)));
            break;
        }
        case NodeKind::Frame: {
            o.emplace_back("kind", JsonValue::string("frame"));
            o.emplace_back("bounds", aabbToJson(n.bounds));
            JsonValue::Array kids;
            for (const auto &c : n.children)
                kids.push_back(nodeToJson(c));
            o.emplace_back("children", JsonValue::array(std::move(kids)));
            break;
        }
        case NodeKind::Connector: {
            o.emplace_back("kind", JsonValue::string("connector"));
            JsonValue::Object from;
            from.emplace_back("nodeId", JsonValue::string(n.fromNodeId));
            JsonValue::Object to;
            to.emplace_back("nodeId", JsonValue::string(n.toNodeId));
            o.emplace_back("from", JsonValue::object(std::move(from)));
            o.emplace_back("to", JsonValue::object(std::move(to)));
            o.emplace_back("invalid", JsonValue::boolean(n.connectorInvalid));
            break;
        }
        case NodeKind::SmartGroup: {
            o.emplace_back("kind", JsonValue::string("smart_group"));
            JsonValue::Object b;
            b.emplace_back("x", JsonValue::number(n.smartBounds.x));
            b.emplace_back("y", JsonValue::number(n.smartBounds.y));
            b.emplace_back("width", JsonValue::number(n.smartBounds.width));
            b.emplace_back("height", JsonValue::number(n.smartBounds.height));
            o.emplace_back("bounds", JsonValue::object(std::move(b)));
            JsonValue::Object t;
            t.emplace_back("x", JsonValue::number(n.transform.x));
            t.emplace_back("y", JsonValue::number(n.transform.y));
            t.emplace_back("rotation", JsonValue::number(n.transform.rotation));
            t.emplace_back("scaleX", JsonValue::number(n.transform.scaleX));
            t.emplace_back("scaleY", JsonValue::number(n.transform.scaleY));
            o.emplace_back("transform", JsonValue::object(std::move(t)));
            o.emplace_back("inkScaleMode", JsonValue::string(n.inkScaleMode));
            JsonValue::Array kids;
            for (const auto &c : n.children)
                kids.push_back(nodeToJson(c));
            o.emplace_back("children", JsonValue::array(std::move(kids)));
            break;
        }
        }
        return JsonValue::object(std::move(o));
    }
};

inline DocOp opFromJson(const JsonValue &j)
{
    DocOp op;
    op.opId = j.getString("opId");
    op.type = j.getString("type");
    op.source = j.getString("source");
    if (const JsonValue *p = j.get("payload"))
        op.payload = *p;
    op.ts = optNumber(j, "ts");
    return op;
}

} // namespace document
} // namespace epaper
