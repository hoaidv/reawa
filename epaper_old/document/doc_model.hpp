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

/**
 * How one connector end sits on a bound SmartGroup.
 * @implements [ADR-0020] §2 anchors; [SRS-EP-18]
 *
 * One attach point + one drawn leave, stored in local frames so they ride the
 * node's transform — not several competing world positions. Kind / edge / t
 * are chosen at recognition and never re-picked when the box moves.
 *
 * Two leave frames hold the same unit vector: a coordinate system on the
 * chosen face (drawnN, drawnE), and one on the box axes (drawnBoxX, drawnBoxY).
 * Neither is a world angle.
 *
 * Wire: drawnEdgeLocal {n,e}, drawnBoxLocal {x,y}, optional local {x,y}.
 */
/** Face-frame sample. +n outward (or facing at centre); +e along the face CCW. */
struct EndpointInkPt {
    double n = 0;
    double e = 0;
};

/** One Path B decoration stroke on a connector end. */
struct EndpointInkStroke {
    std::vector<EndpointInkPt> pts;
};

inline std::vector<EndpointInkStroke> styleInkFromJson(const JsonValue *a)
{
    std::vector<EndpointInkStroke> out;
    if (!a || !a->isArray())
        return out;
    for (const auto &st : a->asArray()) {
        const JsonValue *pts = st.get("pts");
        if (!pts || !pts->isArray())
            continue;
        EndpointInkStroke stroke;
        for (const auto &p : pts->asArray()) {
            EndpointInkPt pt;
            pt.n = p.getNumber("n", 0);
            pt.e = p.getNumber("e", 0);
            stroke.pts.push_back(pt);
        }
        if (!stroke.pts.empty())
            out.push_back(std::move(stroke));
    }
    return out;
}

inline JsonValue styleInkToJson(const std::vector<EndpointInkStroke> &strokes)
{
    JsonValue::Array a;
    for (const auto &st : strokes) {
        JsonValue::Array pts;
        for (const auto &p : st.pts) {
            JsonValue::Object o;
            o.emplace_back("n", JsonValue::number(p.n));
            o.emplace_back("e", JsonValue::number(p.e));
            pts.push_back(JsonValue::object(std::move(o)));
        }
        JsonValue::Object so;
        so.emplace_back("pts", JsonValue::array(std::move(pts)));
        a.push_back(JsonValue::object(std::move(so)));
    }
    return JsonValue::array(std::move(a));
}

struct ConnectorAnchor {
    /** Bound SmartGroup id (same as fromNodeId / toNodeId on the connector). */
    std::string nodeId;
    /**
     * Where the attach coordinate system is planted.
     * "edge": on a box face (see edge, t). "centre": at the box centre — no face.
     * Chosen at recognition; only the creator may change it. Centre is picked
     * when d(center) < kCentreVsBoundary · d(boundary ink).
     */
    std::string kind = "edge";
    /**
     * Which face the attach coordinate system sits on. 0..3, NEWS in local box
     * space (not a world-axis side after the node rotates). Recognition picks
     * the nearest side of the world AABB; after that this index means that local
     * side. Ignored when kind is centre.
     *   0 = N (top)     t left → right
     *   1 = E (right)   t top → bottom
     *   2 = S (bottom)  t right → left
     *   3 = W (left)    t bottom → top
     * Corners of the transformed local bounds: 0 TL, 1 TR, 2 BR, 3 BL, CCW.
     */
    int edge = 0;
    /**
     * Where on that face the origin sits, t ∈ [0, 1].
     * P = corner[edge] + (corner[(edge+1)%4] − corner[edge]) · t.
     * 0 = start corner, 0.5 = midpoint, 1 = end corner. Unused for centre.
     */
    double t = 0;
    /**
     * Face-local leave, first component. A coordinate system is put at the
     * chosen face: origin at the attach point, +N = outward perpendicular to
     * the face, +E = along the face CCW. Leave is the unit vector (drawnN,
     * drawnE) in that frame. (1, 0) means leaving perpendicularly to the face.
     * The doodle's angle off the face rides rotate/scale; it is not vs world-north.
     * Wire: drawnEdgeLocal.n
     */
    double drawnN = 1;
    /**
     * Face-local leave, second component — along the face, CCW (corner[edge] →
     * corner[edge+1]). (0, 1) means leaving along the face; (0, −1) the other way.
     * World facing after a box move: drawnN·n' + drawnE·e' with the current (n, e).
     * Wire: drawnEdgeLocal.e
     */
    double drawnE = 0;
    /**
     * Box-local leave, first component. A second coordinate system is put on
     * the SmartGroup itself: +X = local +X (corner 0 → 1), +Y = local +Y
     * (corner 0 → 3). Same leave as (drawnN, drawnE), expressed in that box
     * frame as unit vector (drawnBoxX, drawnBoxY). Live warp reconstructs
     * facing from this pair. Wire: drawnBoxLocal.x
     */
    double drawnBoxX = 1;
    /**
     * Box-local leave, second component. For a centre end there is no face
     * frame; this box-frame leave is clamped to a 60° cone about the ray
     * toward the other end's centre. Wire: drawnBoxLocal.y
     */
    double drawnBoxY = 0;
    /**
     * Attach point in SmartGroup local space (a third coordinate system: the
     * node's own local XY). Live warp prefers this over edge+t when hasLocal,
     * so the pin rides the transform. Wire: local.x
     */
    double localX = 0;
    /** Attach point, SmartGroup-local Y. Wire: local.y */
    double localY = 0;
    /** True when localX/Y were stored (recognition always sets this). */
    bool hasLocal = false;
    /**
     * Path B decoration: ordered strokes in the live face frame.
     * Paint rotates by the delta between stored leave and re-warped leave.
     * @implements [SRS-EP-35] ConnectorAnchor.styleInk
     */
    std::vector<EndpointInkStroke> styleInk;
};

/** Rest-spine sample, or a derived warped sample, in world XY. */
struct ConnectorRestPt {
    double x = 0;
    double y = 0;
};

/**
 * One raw-ink sample in the rest-spine coordinate system: origin along S,
 * +s = along the spine from the from-end, +d = signed perpendicular.
 */
struct ConnectorRestOff {
    /** How far along rest spine S, normalized to [0, 1] (0 = from-end, 1 = to-end). */
    double s = 0;
    /** Signed offset off S, world u (never scaled later). */
    double d = 0;
};

/**
 * Last live world pose of one connector end. World XY — not a box-local frame.
 * Used when the bound nodeId is missing (delete); not an op.
 * @implements [SRS-EP-18] last live world pose cache (D39)
 */
struct ConnectorEndPose {
    double x = 0;     // world attach
    double y = 0;
    double fx = 1;    // unit leave in world (same role as (drawnN, drawnE), after transform)
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
    /**
     * Invisible enclose polygon (group-local). Never clipped; missing on old groups.
     * @implements [SRS-EP-55] SmartGroup boundary polyline
     */
    std::vector<InkSample> boundaryPolyline;
    /**
     * Connector fields. Not a spatial parent: visible geometry is a pure function
     * of {rest shape, two endpoint states, warpStyle} — [ADR-0020] / [SRS-EP-18].
     * Rest shape is baked once at recognition (I1: never rewritten from a warp).
     */
    std::string fromNodeId; // bound SmartGroup (v1); missing → fromPose
    std::string toNodeId;
    ConnectorAnchor fromAnchor; // attach + leave on the from-node (face or centre frame)
    ConnectorAnchor toAnchor;
    std::string warpStyle; // "morph" (Ink) | "cubic" (Curve)
    std::vector<ConnectorRestPt> restSpine;     // S in world XY; warp coordinate system's backbone
    std::vector<ConnectorRestOff> restOffsets;  // body ink in S's (s, d) frame
    std::vector<ConnectorRestPt> warpedSamples; // derived world polyline; not an op
    /** Derived world polylines of both ends' styleInk; not an op. */
    std::vector<std::vector<ConnectorRestPt>> warpedStyleInk;
    ConnectorEndPose fromPose; // last live world pose (D39) — world XY, not box-local
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

/** Closed copy of a polyline (appends first sample when the ring is open). */
inline std::vector<InkSample> closedPolylineCopy(const std::vector<InkSample> &in)
{
    std::vector<InkSample> out = in;
    if (out.size() < 2)
        return out;
    const InkSample &a = out.front();
    const InkSample &b = out.back();
    if (std::hypot(a.x - b.x, a.y - b.y) > 1e-6)
        out.push_back(a);
    return out;
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
