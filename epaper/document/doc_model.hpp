#pragma once
/**
 * Device document model types and JSON helpers (no apply / undo).
 * @implements [SRS-EP-07] DeviceDocument tree types
 * @implements [SRS-EP-09] lastOpId; inverse entry shape
 */

#include "json_value.hpp"
#include "operations/edit_kinds.hpp"

#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
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

struct ConnectorAnchor {
    std::string nodeId;
    std::string kind = "edge";
    int edge = 0;
    double t = 0;
    double drawnN = 1;
    double drawnE = 0;
    double drawnBoxX = 1;
    double drawnBoxY = 0;
    double localX = 0;
    double localY = 0;
    bool hasLocal = false;
};

struct ConnectorRestPt {
    double x = 0;
    double y = 0;
};

struct ConnectorRestOff {
    double s = 0;
    double d = 0;
};

/** @implements [SRS-EP-18] last live world pose cache (D39) */
struct ConnectorEndPose {
    double x = 0;
    double y = 0;
    double fx = 1;
    double fy = 0;
    bool valid = false;
};

struct DocNode {
    std::string id;
    /**
     * Last document-semantic mutation; not serialized on the wire.
     * @implements [SRS-EP-09] node lastOpId
     */
    std::string lastOpId;
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
    ConnectorAnchor fromAnchor;
    ConnectorAnchor toAnchor;
    std::string warpStyle;
    std::vector<ConnectorRestPt> restSpine;
    std::vector<ConnectorRestOff> restOffsets;
    std::vector<ConnectorRestPt> warpedSamples; // derived; not an op
    ConnectorEndPose fromPose; // last live world pose (D39)
    ConnectorEndPose toPose;
    bool connectorInvalid = false;
    std::vector<DocNode> children;
};

struct ApplyResult {
    bool applied = false;
    std::string reason;
};

/** @implements [SRS-EP-07] undo ring depth 20 */
constexpr int kUndoRingDepth = 20;

/**
 * Node this forward edit versioned, plus lastOpId before that edit.
 * @implements [SRS-EP-09] undo target { nodeId, prevLastOpId }
 */
struct UndoTarget {
    std::string nodeId;
    std::string prevLastOpId;
};

struct UndoResult {
    bool restored = false;
    bool latched = false;
    bool noop = false;
    /** F20: lastOpId mismatch — entry consumed, tree unchanged, 0 redo. */
    bool skipped = false;
};

inline bool isStructuralOp(const std::string &type)
{
    return isStructuralKind(type.c_str());
}

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
 * In-memory doc_change (not TCP). `op` is a serialized DocEdit envelope.
 * @implements [SRS-EP-09] publish queue entry
 */
struct DocChange {
    int seq = 0;
    int baseSeq = 0;
    std::string opId;
    JsonValue op;
    std::int64_t committedAtMs = 0;
};

/** @implements [SRS-EP-08] doc_change wire envelope */
inline JsonValue docChangeToJson(const DocChange &ch)
{
    JsonValue::Object o;
    o.emplace_back("type", JsonValue::string("doc_change"));
    o.emplace_back("seq", JsonValue::number(ch.seq));
    o.emplace_back("opId", JsonValue::string(ch.opId));
    o.emplace_back("op", ch.op);
    o.emplace_back("baseSeq", JsonValue::number(ch.baseSeq));
    return JsonValue::object(std::move(o));
}

/** @implements [SRS-EP-08] SRS-IN-09 closed op.type list */
inline bool isClosedTransmitOp(const std::string &type)
{
    return isClosedTransmitKind(type.c_str());
}

inline JsonValue transformToJson(const SmartTransform &t)
{
    JsonValue::Object o;
    o.emplace_back("x", JsonValue::number(t.x));
    o.emplace_back("y", JsonValue::number(t.y));
    o.emplace_back("rotation", JsonValue::number(t.rotation));
    o.emplace_back("scaleX", JsonValue::number(t.scaleX));
    o.emplace_back("scaleY", JsonValue::number(t.scaleY));
    return JsonValue::object(std::move(o));
}

inline SmartTransform transformFromJson(const JsonValue *p)
{
    SmartTransform t;
    if (!p || !p->isObject())
        return t;
    t.x = p->getNumber("x");
    t.y = p->getNumber("y");
    t.rotation = p->getNumber("rotation");
    t.scaleX = p->has("scaleX") ? p->getNumber("scaleX") : 1;
    t.scaleY = p->has("scaleY") ? p->getNumber("scaleY") : 1;
    return t;
}

inline JsonValue boundsToJson(const SmartBounds &b)
{
    JsonValue::Object o;
    o.emplace_back("x", JsonValue::number(b.x));
    o.emplace_back("y", JsonValue::number(b.y));
    o.emplace_back("width", JsonValue::number(b.width));
    o.emplace_back("height", JsonValue::number(b.height));
    return JsonValue::object(std::move(o));
}

inline SmartBounds boundsFromJson(const JsonValue *p)
{
    SmartBounds b;
    if (!p || !p->isObject())
        return b;
    b.x = p->getNumber("x");
    b.y = p->getNumber("y");
    b.width = p->getNumber("width");
    b.height = p->getNumber("height");
    return b;
}

} // namespace document
} // namespace epaper
