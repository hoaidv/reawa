#pragma once
/**
 * Open-stroke connector recognition at pen-up (ADR-0022 step 3).
 * Snap and inside tests use the SmartGroup **boundary ink** polyline.
 * @implements [SRS-EP-17] UX1/UX2 guards, create_connector, warpStyle from S
 */

#include "device_document.hpp"
#include "membership.hpp"
#include "recognize_enclose.hpp"
#include "rest_shape.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace epaper {
namespace document {

constexpr double kMinConnectorWorld = 48;
/** R_SNAP / R_JOIN v1 — named in SRS; numeric start is half MIN_CONNECTOR_WORLD. */
constexpr double kConnectorSnapWorld = 24;
constexpr double kConnectorJoinWorld = 24;
constexpr double kJoinMaxTurnDeg = 60;
/** Convex-hull area / L² — not AABB (a diagonal's AABB / L² is ~0.5). Wiggles must pass. */
constexpr double kPathLikeHullOverLen2 = 0.5;
constexpr int kConnectorChainLookback = 8;

enum class ConnectorKind {
    Created,
    None,
};

struct ConnectorResult {
    ConnectorKind kind = ConnectorKind::None;
    std::string reason;
    std::string connectorId;
    std::string fromId;
    std::string toId;
    std::string warpStyle;
    std::vector<std::string> bodyIds;
};

inline double distPointAabb(double x, double y, const SmartBounds &b)
{
    const double maxX = b.x + b.width;
    const double maxY = b.y + b.height;
    const double dx = std::max({b.x - x, 0.0, x - maxX});
    const double dy = std::max({b.y - y, 0.0, y - maxY});
    if (dx == 0 && dy == 0)
        return 0;
    if (dx == 0)
        return dy;
    if (dy == 0)
        return dx;
    return std::hypot(dx, dy);
}

inline bool sampleInBounds(double x, double y, const SmartBounds &b)
{
    return distPointAabb(x, y, b) == 0;
}

/** First boundary-role ink of a SmartGroup, world polyline. Empty if none. */
inline std::vector<Vec2> smartGroupBoundaryWorld(const DocNode &sg)
{
    std::vector<Vec2> poly;
    for (const auto &c : sg.children) {
        if (c.kind != NodeKind::Ink)
            continue;
        const std::string role = c.role ? *c.role : std::string("content");
        if (role != "boundary" || c.samples.size() < 2)
            continue;
        poly.reserve(c.samples.size());
        for (const auto &s : c.samples) {
            const Vec2 w = smartLocalToWorld(s.x, s.y, sg, "boundary", std::nullopt, nullptr);
            poly.push_back(w);
        }
        break;
    }
    return poly;
}

inline double distPointSeg(double px, double py, double ax, double ay, double bx, double by)
{
    const double vx = bx - ax;
    const double vy = by - ay;
    const double L2 = vx * vx + vy * vy;
    double t = 0;
    if (L2 > 1e-12)
        t = std::max(0.0, std::min(1.0, ((px - ax) * vx + (py - ay) * vy) / L2));
    return std::hypot(px - (ax + t * vx), py - (ay + t * vy));
}

inline double distPointPolyline(double x, double y, const std::vector<Vec2> &poly)
{
    if (poly.size() < 2)
        return 1e300;
    double best = 1e300;
    for (size_t i = 1; i < poly.size(); ++i)
        best = std::min(best, distPointSeg(x, y, poly[i - 1].x, poly[i - 1].y, poly[i].x, poly[i].y));
    return best;
}

/** Even-odd fill; closes the ring if first≠last. */
inline bool pointInBoundary(double x, double y, const std::vector<Vec2> &poly)
{
    if (poly.size() < 3)
        return false;
    const Vec2 &f = poly.front();
    const Vec2 &b = poly.back();
    const bool closed = std::hypot(f.x - b.x, f.y - b.y) < 1e-6;
    const size_t n = closed ? poly.size() - 1 : poly.size();
    if (n < 3)
        return false;
    bool inside = false;
    size_t j = n - 1;
    for (size_t i = 0; i < n; ++i) {
        const double yi = poly[i].y;
        const double yj = poly[j].y;
        const double xi = poly[i].x;
        const double xj = poly[j].x;
        if ((yi > y) != (yj > y)) {
            const double xHit = (xj - xi) * (y - yi) / (yj - yi + 1e-30) + xi;
            if (x < xHit)
                inside = !inside;
        }
        j = i;
    }
    return inside;
}

/** Snap/inside vs boundary ink. AABB only if the group has no boundary stroke. */
inline double distToSmartGroup(const DocNode &sg, double x, double y)
{
    const auto poly = smartGroupBoundaryWorld(sg);
    if (poly.size() >= 2) {
        if (pointInBoundary(x, y, poly))
            return 0;
        return distPointPolyline(x, y, poly);
    }
    return distPointAabb(x, y, smartGroupWorldBounds(sg));
}

inline bool sampleInsideSmartGroup(const DocNode &sg, double x, double y)
{
    const auto poly = smartGroupBoundaryWorld(sg);
    if (poly.size() >= 3)
        return pointInBoundary(x, y, poly);
    return sampleInBounds(x, y, smartGroupWorldBounds(sg));
}

inline const DocNode *nearestSnapGroup(const std::vector<const DocNode *> &groups, double x,
                                       double y, double *distOut = nullptr)
{
    const DocNode *best = nullptr;
    double bestD = 1e300;
    for (const DocNode *sg : groups) {
        const double d = distToSmartGroup(*sg, x, y);
        if (d <= kConnectorSnapWorld && d < bestD) {
            bestD = d;
            best = sg;
        }
    }
    if (distOut)
        *distOut = best ? bestD : 1e300;
    return best;
}

inline void collectFreeInks(const std::vector<DocNode> &nodes, std::vector<const DocNode *> &out)
{
    for (const auto &n : nodes) {
        if (n.kind == NodeKind::Ink)
            out.push_back(&n);
        if (n.kind == NodeKind::Frame || n.kind == NodeKind::Group)
            collectFreeInks(n.children, out);
    }
}

inline Vec2 inkEnd(const DocNode &n, bool first)
{
    if (n.samples.empty())
        return {};
    const InkSample &s = first ? n.samples.front() : n.samples.back();
    return {s.x, s.y};
}

inline Vec2 inkTan(const DocNode &n, bool atStart)
{
    if (n.samples.size() < 2)
        return {1, 0};
    if (atStart) {
        return {n.samples[1].x - n.samples[0].x, n.samples[1].y - n.samples[0].y};
    }
    const size_t i = n.samples.size();
    return {n.samples[i - 1].x - n.samples[i - 2].x, n.samples[i - 1].y - n.samples[i - 2].y};
}

inline double turnDeg(Vec2 a, Vec2 b)
{
    const double la = std::hypot(a.x, a.y);
    const double lb = std::hypot(b.x, b.y);
    if (la < 1e-9 || lb < 1e-9)
        return 180;
    double c = (a.x * b.x + a.y * b.y) / (la * lb);
    c = std::max(-1.0, std::min(1.0, c));
    return std::acos(c) * 180.0 / 3.14159265358979323846;
}

inline bool strokesJoin(const DocNode &a, const DocNode &b)
{
    struct Pair {
        bool aFirst;
        bool bFirst;
    };
    const Pair pairs[] = {{false, true}, {false, false}, {true, true}, {true, false}};
    for (const Pair &p : pairs) {
        const Vec2 pa = inkEnd(a, p.aFirst);
        const Vec2 pb = inkEnd(b, p.bFirst);
        if (std::hypot(pa.x - pb.x, pa.y - pb.y) > kConnectorJoinWorld)
            continue;
        Vec2 ta = inkTan(a, p.aFirst);
        Vec2 tb = inkTan(b, p.bFirst);
        if (p.aFirst) {
            ta.x = -ta.x;
            ta.y = -ta.y;
        }
        if (!p.bFirst) {
            tb.x = -tb.x;
            tb.y = -tb.y;
        }
        if (turnDeg(ta, tb) <= kJoinMaxTurnDeg)
            return true;
    }
    return false;
}

inline double samplesLength(const std::vector<InkSample> &s)
{
    double L = 0;
    for (size_t i = 1; i < s.size(); ++i)
        L += std::hypot(s[i].x - s[i - 1].x, s[i].y - s[i - 1].y);
    return L;
}

inline double convexHullArea(const std::vector<InkSample> &s)
{
    if (s.size() < 3)
        return 0;
    std::vector<std::pair<double, double>> p;
    p.reserve(s.size());
    for (const auto &q : s)
        p.push_back({q.x, q.y});
    std::sort(p.begin(), p.end());
    p.erase(std::unique(p.begin(), p.end(),
                        [](const auto &a, const auto &b) {
                            return std::hypot(a.first - b.first, a.second - b.second) < 1e-9;
                        }),
            p.end());
    if (p.size() < 3)
        return 0;
    auto cross = [](const std::pair<double, double> &o, const std::pair<double, double> &a,
                    const std::pair<double, double> &b) {
        return (a.first - o.first) * (b.second - o.second) - (a.second - o.second) * (b.first - o.first);
    };
    std::vector<std::pair<double, double>> h;
    for (const auto &pt : p) {
        while (h.size() >= 2 && cross(h[h.size() - 2], h.back(), pt) <= 0)
            h.pop_back();
        h.push_back(pt);
    }
    const size_t lower = h.size();
    for (int i = int(p.size()) - 2; i >= 0; --i) {
        while (h.size() > lower && cross(h[h.size() - 2], h.back(), p[size_t(i)]) <= 0)
            h.pop_back();
        h.push_back(p[size_t(i)]);
    }
    if (h.size() < 3)
        return 0;
    h.pop_back();
    double area = 0;
    for (size_t i = 0; i < h.size(); ++i) {
        const auto &a = h[i];
        const auto &b = h[(i + 1) % h.size()];
        area += a.first * b.second - b.first * a.second;
    }
    return std::abs(area) * 0.5;
}

inline double hullAreaOverLen2(const std::vector<InkSample> &s)
{
    const double L = samplesLength(s);
    if (L < 1e-6)
        return 1;
    return convexHullArea(s) / (L * L);
}

inline void pickEdgeAnchor(const SmartBounds &b, double x, double y, const RestVec &drawn,
                           JsonValue::Object *o)
{
    const double maxX = b.x + b.width;
    const double maxY = b.y + b.height;
    const double d[4] = {
        std::abs(y - b.y),
        std::abs(x - maxX),
        std::abs(y - maxY),
        std::abs(x - b.x),
    };
    int edge = 0;
    for (int i = 1; i < 4; ++i) {
        if (d[i] < d[edge])
            edge = i;
    }
    double t = 0;
    RestVec n{0, -1};
    RestVec e{1, 0};
    if (edge == 0) {
        t = b.width > 1e-9 ? (x - b.x) / b.width : 0;
        n = {0, -1};
        e = {1, 0};
    } else if (edge == 1) {
        t = b.height > 1e-9 ? (y - b.y) / b.height : 0;
        n = {1, 0};
        e = {0, 1};
    } else if (edge == 2) {
        t = b.width > 1e-9 ? (maxX - x) / b.width : 0;
        n = {0, 1};
        e = {-1, 0};
    } else {
        t = b.height > 1e-9 ? (maxY - y) / b.height : 0;
        n = {-1, 0};
        e = {0, -1};
    }
    t = std::max(0.0, std::min(1.0, t));
    o->emplace_back("kind", JsonValue::string("edge"));
    o->emplace_back("edge", JsonValue::number(edge));
    o->emplace_back("t", JsonValue::number(t));
    JsonValue::Object loc;
    loc.emplace_back("n", JsonValue::number(drawn.x * n.x + drawn.y * n.y));
    loc.emplace_back("e", JsonValue::number(drawn.x * e.x + drawn.y * e.y));
    o->emplace_back("drawnEdgeLocal", JsonValue::object(std::move(loc)));
}

inline JsonValue restShapeToJson(const RestShape &r)
{
    JsonValue::Array spine;
    for (const auto &p : r.spine) {
        JsonValue::Object o;
        o.emplace_back("x", JsonValue::number(p.x));
        o.emplace_back("y", JsonValue::number(p.y));
        spine.push_back(JsonValue::object(std::move(o)));
    }
    JsonValue::Array off;
    for (const auto &p : r.offsets) {
        JsonValue::Object o;
        o.emplace_back("s", JsonValue::number(p.s));
        o.emplace_back("d", JsonValue::number(p.d));
        off.push_back(JsonValue::object(std::move(o)));
    }
    JsonValue::Object o;
    o.emplace_back("spine", JsonValue::array(std::move(spine)));
    o.emplace_back("offsets", JsonValue::array(std::move(off)));
    o.emplace_back("inflections", JsonValue::number(r.inflections));
    return JsonValue::object(std::move(o));
}

inline bool bodyOutsideGuards(const std::vector<InkSample> &s, const DocNode &a, const DocNode &c,
                              const std::vector<const DocNode *> &groups)
{
    const int n = int(s.size());
    if (n <= 0)
        return false;
    int i0 = 0;
    while (i0 < n && sampleInsideSmartGroup(a, s[size_t(i0)].x, s[size_t(i0)].y))
        ++i0;
    int i1 = n;
    while (i1 > i0 && sampleInsideSmartGroup(c, s[size_t(i1 - 1)].x, s[size_t(i1 - 1)].y))
        --i1;
    const int m = i1 - i0;
    if (m <= 0)
        return false;
    int inA = 0, inC = 0, outAll = 0;
    for (int i = i0; i < i1; ++i) {
        const auto &p = s[size_t(i)];
        const bool ia = sampleInsideSmartGroup(a, p.x, p.y);
        const bool ic = sampleInsideSmartGroup(c, p.x, p.y);
        if (ia)
            ++inA;
        if (ic)
            ++inC;
        bool inAny = ia || ic;
        if (!inAny) {
            for (const DocNode *g : groups) {
                if (g->id == a.id || g->id == c.id)
                    continue;
                if (sampleInsideSmartGroup(*g, p.x, p.y)) {
                    inAny = true;
                    break;
                }
            }
        }
        if (!inAny)
            ++outAll;
    }
    return (double(inA) / m) <= 0.20 && (double(inC) / m) <= 0.20 && (double(outAll) / m) >= 0.60;
}

inline ConnectorResult tryRecognizeConnector(DeviceDocument &doc, const std::string &strokeId)
{
    ConnectorResult out;
    const DocNode *cur = doc.find(strokeId);
    if (!cur || cur->kind != NodeKind::Ink) {
        out.reason = "not_ink";
        return out;
    }
    if (cur->samples.size() < 2) {
        out.reason = "too_small";
        return out;
    }

    std::vector<const DocNode *> groups;
    smartGroupsInPaintOrder(doc.rootChildren, groups);
    if (groups.size() < 2) {
        out.reason = "need_two_groups";
        return out;
    }

    std::vector<const DocNode *> freeInks;
    collectFreeInks(doc.rootChildren, freeInks);
    int curIdx = -1;
    for (int i = 0; i < int(freeInks.size()); ++i) {
        if (freeInks[size_t(i)]->id == strokeId)
            curIdx = i;
    }
    if (curIdx < 0) {
        out.reason = "not_free";
        return out;
    }

    std::vector<const DocNode *> chain;
    chain.push_back(freeInks[size_t(curIdx)]);
    const int look = std::max(0, curIdx - (kConnectorChainLookback - 1));
    for (int i = curIdx - 1; i >= look; --i) {
        if (strokesJoin(*freeInks[size_t(i)], *chain.front()))
            chain.insert(chain.begin(), freeInks[size_t(i)]);
        else
            break;
    }

    std::vector<InkSample> concat;
    std::vector<std::vector<InkSample>> strokes;
    std::vector<std::string> bodyIds;
    for (const DocNode *n : chain) {
        bodyIds.push_back(n->id);
        strokes.push_back(n->samples);
        concat.insert(concat.end(), n->samples.begin(), n->samples.end());
    }

    const double L = samplesLength(concat);
    if (L < kMinConnectorWorld) {
        out.reason = "too_small";
        return out;
    }
    if (hullAreaOverLen2(concat) > kPathLikeHullOverLen2) {
        out.reason = "not_path_like";
        return out;
    }

    const DocNode *fromG = nearestSnapGroup(groups, concat.front().x, concat.front().y);
    const DocNode *toG = nearestSnapGroup(groups, concat.back().x, concat.back().y);
    if (!fromG || !toG || fromG->id == toG->id) {
        out.reason = "no_two_bindings";
        return out;
    }
    if (!bodyOutsideGuards(concat, *fromG, *toG, groups)) {
        out.reason = "body_inside";
        return out;
    }

    const RestShape rest = buildRestShape(strokes);
    RestVec drawnFrom{1, 0};
    RestVec drawnTo{-1, 0};
    if (rest.spine.size() >= 2) {
        drawnFrom = {rest.spine[1].x - rest.spine[0].x, rest.spine[1].y - rest.spine[0].y};
        const size_t ns = rest.spine.size();
        drawnTo = {rest.spine[ns - 2].x - rest.spine[ns - 1].x,
                   rest.spine[ns - 2].y - rest.spine[ns - 1].y};
    }

    JsonValue::Object from;
    from.emplace_back("nodeId", JsonValue::string(fromG->id));
    pickEdgeAnchor(smartGroupWorldBounds(*fromG), concat.front().x, concat.front().y, drawnFrom,
                   &from);
    JsonValue::Object to;
    to.emplace_back("nodeId", JsonValue::string(toG->id));
    pickEdgeAnchor(smartGroupWorldBounds(*toG), concat.back().x, concat.back().y, drawnTo, &to);

    JsonValue::Array cap;
    for (const auto &id : bodyIds)
        cap.push_back(JsonValue::string(id));

    const std::string cid = std::string("conn_") + strokeId;
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(cid));
    payload.emplace_back("from", JsonValue::object(std::move(from)));
    payload.emplace_back("to", JsonValue::object(std::move(to)));
    payload.emplace_back("warpStyle", JsonValue::string(rest.warpStyle));
    payload.emplace_back("restShape", restShapeToJson(rest));
    payload.emplace_back("captureIds", JsonValue::array(std::move(cap)));
    payload.emplace_back("terminal", JsonValue::object({}));

    DocOp op;
    op.opId = std::string("create_connector:") + cid;
    op.type = "create_connector";
    op.source = "epaper";
    op.payload = JsonValue::object(std::move(payload));
    const ApplyResult r = doc.commitOp(op);
    if (!r.applied) {
        out.reason = r.reason.empty() ? "commit_failed" : r.reason;
        return out;
    }
    out.kind = ConnectorKind::Created;
    out.reason = "none";
    out.connectorId = cid;
    out.fromId = fromG->id;
    out.toId = toG->id;
    out.warpStyle = rest.warpStyle;
    out.bodyIds = std::move(bodyIds);
    return out;
}

} // namespace document
} // namespace epaper
