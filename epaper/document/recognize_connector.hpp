#pragma once
/**
 * Open-stroke connector recognition at pen-up (ADR-0022 step 3).
 * Snap and inside tests use the SmartGroup **boundary ink** polyline.
 * @implements [SRS-EP-17] UX1/UX2 guards, create_connector, warpStyle from S
 */

#include "connector_warp.hpp"
#include "device_document.hpp"
#include "membership.hpp"
#include "recognize_enclose.hpp"
#include "rest_shape.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace epaper {
namespace document {

constexpr double kMinConnectorWorld = 48;
/** R_SNAP to boundary ink. R_JOIN is tight (6 u). Chain ≤5 free inks, any draw order. */
constexpr double kConnectorSnapWorld = 24;
constexpr double kConnectorJoinWorld = 6;
/** Convex-hull area / L² — not AABB (a diagonal's AABB / L² is ~0.5). Wiggles must pass. */
constexpr double kPathLikeHullOverLen2 = 0.5;
constexpr int kConnectorChainMax = 5;

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
    /** Device-log detail for [conn] (does not affect verdict). */
    std::string diag;
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
                                       double y, const std::string *excludeId = nullptr)
{
    const DocNode *best = nullptr;
    double bestD = 1e300;
    for (const DocNode *sg : groups) {
        if (excludeId && sg->id == *excludeId)
            continue;
        const double d = distToSmartGroup(*sg, x, y);
        if (d <= kConnectorSnapWorld && d < bestD) {
            bestD = d;
            best = sg;
        }
    }
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

/** Newest root inks until a SmartGroup/Connector/other, max 5. Does not skip over a box. */
inline std::vector<const DocNode *> consecutiveTailInks(const std::vector<DocNode> &root, std::string *stopWhy)
{
    std::vector<const DocNode *> tail;
    for (int i = int(root.size()) - 1; i >= 0 && int(tail.size()) < kConnectorChainMax; --i) {
        const DocNode &n = root[size_t(i)];
        if (n.kind == NodeKind::Ink) {
            tail.push_back(&n);
            continue;
        }
        if (stopWhy) {
            if (n.kind == NodeKind::SmartGroup)
                *stopWhy = "sg:" + n.id;
            else if (n.kind == NodeKind::Connector)
                *stopWhy = "conn:" + n.id;
            else
                *stopWhy = n.id;
        }
        break;
    }
    std::reverse(tail.begin(), tail.end());
    return tail;
}

inline Vec2 inkEnd(const DocNode &n, bool first)
{
    if (n.samples.empty())
        return {};
    const InkSample &s = first ? n.samples.front() : n.samples.back();
    return {s.x, s.y};
}

inline bool endsJoin(Vec2 a, Vec2 b)
{
    return std::hypot(a.x - b.x, a.y - b.y) <= kConnectorJoinWorld;
}

struct SegHit {
    bool ok = false;
    Vec2 p;
};

inline SegHit segmentIntersect(Vec2 a, Vec2 b, Vec2 c, Vec2 d)
{
    SegHit h;
    const double ax = b.x - a.x, ay = b.y - a.y;
    const double cx = d.x - c.x, cy = d.y - c.y;
    const double den = ax * cy - ay * cx;
    if (std::abs(den) < 1e-12)
        return h;
    const double t = ((c.x - a.x) * cy - (c.y - a.y) * cx) / den;
    const double u = ((c.x - a.x) * ay - (c.y - a.y) * ax) / den;
    if (t < -1e-6 || t > 1 + 1e-6 || u < -1e-6 || u > 1 + 1e-6)
        return h;
    h.ok = true;
    h.p = {a.x + t * ax, a.y + t * ay};
    return h;
}

inline double distPointSegClamped(Vec2 p, Vec2 a, Vec2 b, Vec2 *proj)
{
    const double vx = b.x - a.x, vy = b.y - a.y;
    const double L2 = vx * vx + vy * vy;
    double t = 0;
    if (L2 > 1e-12)
        t = std::max(0.0, std::min(1.0, ((p.x - a.x) * vx + (p.y - a.y) * vy) / L2));
    const Vec2 q{a.x + t * vx, a.y + t * vy};
    if (proj)
        *proj = q;
    return std::hypot(p.x - q.x, p.y - q.y);
}

/** Closest points on two segments. */
inline double distSegSeg(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec2 *pa, Vec2 *pb)
{
    const double ux = b.x - a.x, uy = b.y - a.y;
    const double vx = d.x - c.x, vy = d.y - c.y;
    const double wx = a.x - c.x, wy = a.y - c.y;
    const double uu = ux * ux + uy * uy;
    const double vv = vx * vx + vy * vy;
    const double uv = ux * vx + uy * vy;
    const double uw = ux * wx + uy * wy;
    const double vw = vx * wx + vy * wy;
    const double den = uu * vv - uv * uv;
    double sc, tc;
    if (den < 1e-18) {
        sc = 0;
        tc = vv > 1e-18 ? vw / vv : 0;
    } else {
        sc = (uv * vw - vv * uw) / den;
        tc = (uu * vw - uv * uw) / den;
    }
    sc = std::max(0.0, std::min(1.0, sc));
    tc = std::max(0.0, std::min(1.0, tc));
    const Vec2 p{a.x + sc * ux, a.y + sc * uy};
    const Vec2 q{c.x + tc * vx, c.y + tc * vy};
    if (pa)
        *pa = p;
    if (pb)
        *pb = q;
    return std::hypot(p.x - q.x, p.y - q.y);
}

/** Proper cross first; else T-junction (endpoint onto the other stroke, ≤ R_JOIN). */
/** Padded sample-bounds overlap. Every hit below needs a point of @p a within
 *  kConnectorJoinWorld of a point of @p b, so disjoint bounds cannot hit. */
inline bool sampleBoundsMayTouch(const std::vector<InkSample> &a, const std::vector<InkSample> &b,
                                 double pad)
{
    if (a.empty() || b.empty())
        return false;
    double ax0 = a[0].x, ay0 = a[0].y, ax1 = a[0].x, ay1 = a[0].y;
    for (const InkSample &s : a) {
        ax0 = std::min(ax0, s.x);
        ay0 = std::min(ay0, s.y);
        ax1 = std::max(ax1, s.x);
        ay1 = std::max(ay1, s.y);
    }
    double bx0 = b[0].x, by0 = b[0].y, bx1 = b[0].x, by1 = b[0].y;
    for (const InkSample &s : b) {
        bx0 = std::min(bx0, s.x);
        by0 = std::min(by0, s.y);
        bx1 = std::max(bx1, s.x);
        by1 = std::max(by1, s.y);
    }
    return ax0 - pad <= bx1 && bx0 - pad <= ax1 && ay0 - pad <= by1 && by0 - pad <= ay1;
}

/** Drops samples closer together than @p minStep, keeping both endpoints. */
inline std::vector<InkSample> coarsenForJoin(const std::vector<InkSample> &s, double minStep)
{
    if (s.size() <= 2)
        return s;
    std::vector<InkSample> out;
    out.reserve(s.size());
    out.push_back(s.front());
    for (size_t i = 1; i + 1 < s.size(); ++i) {
        if (std::hypot(s[i].x - out.back().x, s[i].y - out.back().y) >= minStep)
            out.push_back(s[i]);
    }
    out.push_back(s.back());
    return out;
}

inline SegHit polylineIntersectDense(const std::vector<InkSample> &a,
                                     const std::vector<InkSample> &b,
                                     double joinTol = kConnectorJoinWorld)
{
    SegHit best;
    double bestT = 1e300;
    auto consider = [&](Vec2 p, double score) {
        if (!best.ok || score < bestT) {
            best.ok = true;
            best.p = p;
            bestT = score;
        }
    };
    for (size_t i = 1; i < a.size(); ++i) {
        const Vec2 a0{a[i - 1].x, a[i - 1].y};
        const Vec2 a1{a[i].x, a[i].y};
        for (size_t j = 1; j < b.size(); ++j) {
            const Vec2 b0{b[j - 1].x, b[j - 1].y};
            const Vec2 b1{b[j].x, b[j].y};
            const SegHit h = segmentIntersect(a0, a1, b0, b1);
            if (h.ok)
                consider(h.p, 0);
        }
    }
    if (best.ok && bestT == 0)
        return best;
    auto tHits = [&](const std::vector<InkSample> &poly, Vec2 end) {
        for (size_t i = 1; i < poly.size(); ++i) {
            Vec2 q;
            const double d = distPointSegClamped(end, {poly[i - 1].x, poly[i - 1].y},
                                                 {poly[i].x, poly[i].y}, &q);
            if (d <= joinTol)
                consider(q, 1 + d);
        }
    };
    if (a.size() >= 2 && b.size() >= 2) {
        tHits(b, {a.front().x, a.front().y});
        tHits(b, {a.back().x, a.back().y});
        tHits(a, {b.front().x, b.front().y});
        tHits(a, {b.back().x, b.back().y});
        for (size_t i = 1; i < a.size(); ++i) {
            const Vec2 a0{a[i - 1].x, a[i - 1].y};
            const Vec2 a1{a[i].x, a[i].y};
            for (size_t j = 1; j < b.size(); ++j) {
                Vec2 pa, pb;
                const double d = distSegSeg(a0, a1, {b[j - 1].x, b[j - 1].y}, {b[j].x, b[j].y}, &pa, &pb);
                if (d <= joinTol)
                    consider({(pa.x + pb.x) * 0.5, (pa.y + pb.y) * 0.5}, 2 + d);
            }
        }
    }
    return best;
}

/**
 * Every free-ink pair on the page reaches this test, and the dense form is
 * O(|a|*|b|): two 1900-sample strokes cost seconds on device, which is what
 * froze pen-up. Two rejection stages run first, and only the exact test can
 * return a hit, so verdicts and join points are unchanged.
 *   1. Padded sample bounds — no point of one can be within tolerance of the
 *      other, so no branch below can fire.
 *   2. A coarsened re-test. Dropping samples moves the path by at most one
 *      step, so the filter runs at a tolerance inflated by 2 steps and can
 *      only over-accept, never hide a real touch.
 */
inline SegHit polylineIntersect(const std::vector<InkSample> &a, const std::vector<InkSample> &b)
{
    if (!sampleBoundsMayTouch(a, b, kConnectorJoinWorld))
        return SegHit{};
    const double step = kConnectorJoinWorld * 0.25;
    const std::vector<InkSample> ca = coarsenForJoin(a, step);
    const std::vector<InkSample> cb = coarsenForJoin(b, step);
    if (ca.size() < a.size() || cb.size() < b.size()) {
        if (!polylineIntersectDense(ca, cb, kConnectorJoinWorld + 2.0 * step).ok)
            return SegHit{};
    }
    return polylineIntersectDense(a, b);
}

/** Endpoint half of strokesJoin, for callers that already ruled out a cross. */
inline bool strokesJoinEnds(const DocNode &a, const DocNode &b)
{
    const Vec2 a0 = inkEnd(a, true);
    const Vec2 a1 = inkEnd(a, false);
    const Vec2 b0 = inkEnd(b, true);
    const Vec2 b1 = inkEnd(b, false);
    return endsJoin(a0, b0) || endsJoin(a0, b1) || endsJoin(a1, b0) || endsJoin(a1, b1);
}

inline bool strokesJoin(const DocNode &a, const DocNode &b)
{
    if (polylineIntersect(a.samples, b.samples).ok)
        return true;
    return strokesJoinEnds(a, b);
}

inline bool strokesJoinCross(const DocNode &a, const DocNode &b)
{
    return polylineIntersect(a.samples, b.samples).ok;
}

inline std::string joinKind(const DocNode &a, const DocNode &b)
{
    if (polylineIntersect(a.samples, b.samples).ok)
        return "cross";
    if (strokesJoinEnds(a, b))
        return "rjoin";
    return "none";
}

inline double polyParam(const std::vector<InkSample> &s, Vec2 p)
{
    double best = 1e300;
    double bestS = 0;
    double acc = 0;
    for (size_t i = 1; i < s.size(); ++i) {
        Vec2 q;
        const Vec2 a{s[i - 1].x, s[i - 1].y};
        const Vec2 b{s[i].x, s[i].y};
        const double d = distPointSegClamped(p, a, b, &q);
        const double sl = std::hypot(b.x - a.x, b.y - a.y);
        double t = 0;
        if (sl > 1e-12)
            t = std::hypot(q.x - a.x, q.y - a.y) / sl;
        if (d < best) {
            best = d;
            bestS = acc + t * sl;
        }
        acc += sl;
    }
    return bestS;
}

inline std::vector<InkSample> walkPoly(const std::vector<InkSample> &s, Vec2 from, Vec2 to)
{
    std::vector<InkSample> out;
    if (s.size() < 2)
        return out;
    const double sf = polyParam(s, from);
    const double st = polyParam(s, to);
    struct Knot {
        double s;
        InkSample sm;
    };
    std::vector<Knot> k;
    double acc = 0;
    k.push_back({0, s.front()});
    for (size_t i = 1; i < s.size(); ++i) {
        acc += std::hypot(s[i].x - s[i - 1].x, s[i].y - s[i - 1].y);
        k.push_back({acc, s[i]});
    }
    auto at = [&](double par) {
        par = std::max(0.0, std::min(acc, par));
        for (size_t i = 1; i < k.size(); ++i) {
            if (par <= k[i].s + 1e-9) {
                const double span = k[i].s - k[i - 1].s;
                const double t = span > 1e-12 ? (par - k[i - 1].s) / span : 0;
                InkSample sm = k[i - 1].sm;
                sm.x = k[i - 1].sm.x + t * (k[i].sm.x - k[i - 1].sm.x);
                sm.y = k[i - 1].sm.y + t * (k[i].sm.y - k[i - 1].sm.y);
                return sm;
            }
        }
        return k.back().sm;
    };
    const double lo = std::min(sf, st);
    const double hi = std::max(sf, st);
    out.push_back(at(sf));
    if (st >= sf) {
        for (const auto &n : k) {
            if (n.s > lo + 0.05 && n.s < hi - 0.05)
                out.push_back(n.sm);
        }
        out.push_back(at(st));
    } else {
        for (int i = int(k.size()) - 1; i >= 0; --i) {
            if (k[size_t(i)].s < hi - 0.05 && k[size_t(i)].s > lo + 0.05)
                out.push_back(k[size_t(i)].sm);
        }
        out.push_back(at(st));
    }
    std::vector<InkSample> clean;
    for (const auto &p : out) {
        if (clean.empty() || std::hypot(p.x - clean.back().x, p.y - clean.back().y) > 0.05)
            clean.push_back(p);
    }
    if (clean.size() < 2)
        clean.push_back(at(st));
    return clean;
}

inline Vec2 joinPoint(const DocNode &a, const DocNode &b)
{
    const SegHit h = polylineIntersect(a.samples, b.samples);
    if (h.ok)
        return h.p;
    const Vec2 a0 = inkEnd(a, true), a1 = inkEnd(a, false);
    const Vec2 b0 = inkEnd(b, true), b1 = inkEnd(b, false);
    Vec2 best = a1;
    double bd = 1e300;
    const Vec2 as[2] = {a0, a1};
    const Vec2 bs[2] = {b0, b1};
    for (const Vec2 &pa : as) {
        for (const Vec2 &pb : bs) {
            const double d = std::hypot(pa.x - pb.x, pa.y - pb.y);
            if (d < bd) {
                bd = d;
                best = {(pa.x + pb.x) * 0.5, (pa.y + pb.y) * 0.5};
            }
        }
    }
    return best;
}

struct ChainBuild {
    std::vector<const DocNode *> path;
    std::vector<InkSample> concat;
    std::vector<std::vector<InkSample>> strokes;
    std::vector<std::string> bodyIds;
};

inline bool concatPath(const std::vector<const DocNode *> &path, bool startFirst, bool endFirst,
                       ChainBuild *out)
{
    out->path = path;
    out->concat.clear();
    out->strokes.clear();
    out->bodyIds.clear();
    if (path.empty())
        return false;
    for (const DocNode *n : path)
        out->bodyIds.push_back(n->id);

    auto appendWalk = [&](const DocNode *n, Vec2 from, Vec2 to) {
        auto piece = walkPoly(n->samples, from, to);
        if (piece.empty())
            return;
        out->strokes.push_back(piece);
        if (!out->concat.empty() && !piece.empty()) {
            if (std::hypot(out->concat.back().x - piece.front().x,
                           out->concat.back().y - piece.front().y) < 0.05)
                piece.erase(piece.begin());
        }
        out->concat.insert(out->concat.end(), piece.begin(), piece.end());
    };

    if (path.size() == 1) {
        out->concat = path[0]->samples;
        out->strokes = {path[0]->samples};
        return out->concat.size() >= 2;
    }

    Vec2 cur = inkEnd(*path[0], startFirst);
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        const Vec2 jp = joinPoint(*path[i], *path[i + 1]);
        appendWalk(path[i], cur, jp);
        cur = jp;
    }
    appendWalk(path.back(), cur, inkEnd(*path.back(), endFirst));
    return out->concat.size() >= 2;
}

/** Last ≤5 free inks, undirected endpoint graph, any draw order. Prefer longest path that binds A≠B. */
inline ChainBuild assembleChain(const std::vector<const DocNode *> &freeInks, int curIdx,
                                const std::vector<const DocNode *> &groups)
{
    ChainBuild best;
    const int nAll = int(freeInks.size());
    const int start = std::max(0, nAll - kConnectorChainMax);
    std::vector<const DocNode *> cand;
    int curLocal = -1;
    for (int i = start; i < nAll; ++i) {
        if (i == curIdx)
            curLocal = int(cand.size());
        cand.push_back(freeInks[size_t(i)]);
    }
    if (curLocal < 0) {
        cand.clear();
        cand.push_back(freeInks[size_t(curIdx)]);
        curLocal = 0;
    }
    const int m = int(cand.size());
    std::vector<std::vector<int>> adj(static_cast<size_t>(m));
    for (int i = 0; i < m; ++i) {
        std::vector<int> cross, ends;
        for (int j = 0; j < m; ++j) {
            if (i == j)
                continue;
            if (strokesJoinCross(*cand[size_t(i)], *cand[size_t(j)]))
                cross.push_back(j);
            // Not strokesJoin: the cross test above is the expensive half of it.
            else if (strokesJoinEnds(*cand[size_t(i)], *cand[size_t(j)]))
                ends.push_back(j);
        }
        adj[size_t(i)] = std::move(cross);
        adj[size_t(i)].insert(adj[size_t(i)].end(), ends.begin(), ends.end());
    }

    auto binds = [&](const ChainBuild &b) -> bool {
        if (b.concat.size() < 2)
            return false;
        const DocNode *fromG = nearestSnapGroup(groups, b.concat.front().x, b.concat.front().y);
        if (!fromG)
            return false;
        const std::string fid = fromG->id;
        const DocNode *toG = nearestSnapGroup(groups, b.concat.back().x, b.concat.back().y, &fid);
        return toG && toG->id != fromG->id;
    };

    std::vector<int> path;
    std::vector<char> used(size_t(m), 0);
    int bestLen = 0;
    std::function<void()> dfs;
    dfs = [&]() {
        if (int(path.size()) > bestLen) {
            bool hasCur = false;
            for (int i : path) {
                if (i == curLocal)
                    hasCur = true;
            }
            if (hasCur) {
                std::vector<const DocNode *> nodes;
                for (int i : path)
                    nodes.push_back(cand[size_t(i)]);
                ChainBuild built;
                bool ok = false;
                for (int sf = 0; sf < 2 && !ok; ++sf) {
                    for (int ef = 0; ef < 2 && !ok; ++ef) {
                        if (concatPath(nodes, sf != 0, ef != 0, &built) && binds(built))
                            ok = true;
                    }
                }
                if (ok) {
                    bestLen = int(path.size());
                    best = std::move(built);
                }
            }
        }
        if (int(path.size()) >= m)
            return;
        const int last = path.back();
        for (int nei : adj[size_t(last)]) {
            if (used[size_t(nei)])
                continue;
            used[size_t(nei)] = 1;
            path.push_back(nei);
            dfs();
            path.pop_back();
            used[size_t(nei)] = 0;
        }
    };
    for (int s = 0; s < m; ++s) {
        path = {s};
        std::fill(used.begin(), used.end(), 0);
        used[size_t(s)] = 1;
        dfs();
    }
    if (best.path.empty()) {
        concatPath({cand[size_t(curLocal)]}, true, false, &best);
    }
    return best;
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

inline void pickStrokeAnchor(const DocNode &sg, double x, double y, const RestVec &drawn,
                             JsonValue::Object *o)
{
    const Vec2 loc = smartWorldToLocal(x, y, sg, "boundary");
    const auto poly = smartGroupBoundaryWorld(sg);
    double dPoly = 1e300;
    if (poly.size() >= 2)
        dPoly = distPointPolyline(x, y, poly);
    else
        dPoly = distPointAabb(x, y, smartGroupWorldBounds(sg));
    const SmartBounds wb = smartGroupWorldBounds(sg);
    const double cx = wb.x + wb.width * 0.5;
    const double cy = wb.y + wb.height * 0.5;
    const double dCenter = std::hypot(x - cx, y - cy);
    const bool centre = dCenter + 1e-9 < kCentreVsBoundary * dPoly;

    const double maxX = wb.x + wb.width;
    const double maxY = wb.y + wb.height;
    const double d[4] = {
        std::abs(y - wb.y),
        std::abs(x - maxX),
        std::abs(y - maxY),
        std::abs(x - wb.x),
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
        t = wb.width > 1e-9 ? (x - wb.x) / wb.width : 0;
        n = {0, -1};
        e = {1, 0};
    } else if (edge == 1) {
        t = wb.height > 1e-9 ? (y - wb.y) / wb.height : 0;
        n = {1, 0};
        e = {0, 1};
    } else if (edge == 2) {
        t = wb.width > 1e-9 ? (maxX - x) / wb.width : 0;
        n = {0, 1};
        e = {-1, 0};
    } else {
        t = wb.height > 1e-9 ? (maxY - y) / wb.height : 0;
        n = {-1, 0};
        e = {0, -1};
    }
    t = std::max(0.0, std::min(1.0, t));
    const double dl = std::hypot(drawn.x, drawn.y);
    const RestVec unitD = dl > 1e-9 ? RestVec{drawn.x / dl, drawn.y / dl} : RestVec{1, 0};
    const Vec2 c0 = smartLocalToWorld(sg.smartBounds.x, sg.smartBounds.y, sg, "boundary",
                                      std::nullopt, nullptr);
    const Vec2 c1 = smartLocalToWorld(sg.smartBounds.x + sg.smartBounds.width, sg.smartBounds.y, sg,
                                      "boundary", std::nullopt, nullptr);
    const Vec2 c3 = smartLocalToWorld(sg.smartBounds.x, sg.smartBounds.y + sg.smartBounds.height, sg,
                                      "boundary", std::nullopt, nullptr);
    RestVec ex{c1.x - c0.x, c1.y - c0.y};
    RestVec ey{c3.x - c0.x, c3.y - c0.y};
    const double lex = std::hypot(ex.x, ex.y);
    const double ley = std::hypot(ey.x, ey.y);
    if (lex > 1e-9) {
        ex.x /= lex;
        ex.y /= lex;
    }
    if (ley > 1e-9) {
        ey.x /= ley;
        ey.y /= ley;
    }
    o->emplace_back("kind", JsonValue::string(centre ? "centre" : "edge"));
    o->emplace_back("edge", JsonValue::number(edge));
    o->emplace_back("t", JsonValue::number(t));
    JsonValue::Object local;
    local.emplace_back("x", JsonValue::number(loc.x));
    local.emplace_back("y", JsonValue::number(loc.y));
    o->emplace_back("local", JsonValue::object(std::move(local)));
    JsonValue::Object locN;
    locN.emplace_back("n", JsonValue::number(unitD.x * n.x + unitD.y * n.y));
    locN.emplace_back("e", JsonValue::number(unitD.x * e.x + unitD.y * e.y));
    o->emplace_back("drawnEdgeLocal", JsonValue::object(std::move(locN)));
    JsonValue::Object box;
    box.emplace_back("x", JsonValue::number(unitD.x * ex.x + unitD.y * ex.y));
    box.emplace_back("y", JsonValue::number(unitD.x * ey.x + unitD.y * ey.y));
    o->emplace_back("drawnBoxLocal", JsonValue::object(std::move(box)));
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

    std::string stopWhy;
    std::vector<const DocNode *> freeInks = consecutiveTailInks(doc.rootChildren, &stopWhy);
    int curIdx = -1;
    for (int i = 0; i < int(freeInks.size()); ++i) {
        if (freeInks[size_t(i)]->id == strokeId)
            curIdx = i;
    }
    if (curIdx < 0) {
        freeInks = {cur};
        curIdx = 0;
        if (stopWhy.empty())
            stopWhy = "not_in_tail";
    }

    auto rnd = [](double x) { return std::to_string(int(std::lround(x))); };
    std::string diag = "n=" + std::to_string(freeInks.size()) + " ids=";
    for (size_t i = 0; i < freeInks.size(); ++i) {
        if (i)
            diag += ",";
        diag += freeInks[i]->id;
    }
    if (!stopWhy.empty())
        diag += " stop=" + stopWhy;
    for (size_t i = 0; i < freeInks.size(); ++i) {
        for (size_t j = i + 1; j < freeInks.size(); ++j) {
            diag += " " + freeInks[i]->id + "~" + freeInks[j]->id + "="
                    + joinKind(*freeInks[i], *freeInks[j]);
        }
    }
    out.diag = diag;

    ChainBuild chain = assembleChain(freeInks, curIdx, groups);
    std::vector<InkSample> concat = std::move(chain.concat);
    std::vector<std::vector<InkSample>> strokes = std::move(chain.strokes);
    std::vector<std::string> bodyIds = std::move(chain.bodyIds);

    out.diag = diag;
    if (!bodyIds.empty()) {
        out.diag += " path=";
        for (size_t i = 0; i < bodyIds.size(); ++i) {
            if (i)
                out.diag += ",";
            out.diag += bodyIds[i];
        }
    }
    const double L = samplesLength(concat);
    if (concat.size() >= 1) {
        const DocNode *nf = nullptr, *nb = nullptr;
        double d0 = 1e300, d1 = 1e300;
        for (const DocNode *sg : groups) {
            const double da = distToSmartGroup(*sg, concat.front().x, concat.front().y);
            if (da < d0) {
                d0 = da;
                nf = sg;
            }
            const double db = distToSmartGroup(*sg, concat.back().x, concat.back().y);
            if (db < d1) {
                d1 = db;
                nb = sg;
            }
        }
        out.diag += " d0=" + rnd(d0) + "@" + (nf ? nf->id : "-");
        out.diag += " d1=" + rnd(d1) + "@" + (nb ? nb->id : "-");
        out.diag += " L=" + rnd(L);
    }
    if (L < kMinConnectorWorld) {
        out.reason = "too_small";
        return out;
    }
    if (hullAreaOverLen2(concat) > kPathLikeHullOverLen2) {
        out.reason = "not_path_like";
        return out;
    }

    const DocNode *fromG = nearestSnapGroup(groups, concat.front().x, concat.front().y);
    const std::string fromId = fromG ? fromG->id : std::string();
    const DocNode *toG = nearestSnapGroup(groups, concat.back().x, concat.back().y,
                                          fromG ? &fromId : nullptr);
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
    const double ax = rest.spine.size() >= 2 ? rest.spine.front().x : concat.front().x;
    const double ay = rest.spine.size() >= 2 ? rest.spine.front().y : concat.front().y;
    const double bx = rest.spine.size() >= 2 ? rest.spine.back().x : concat.back().x;
    const double by = rest.spine.size() >= 2 ? rest.spine.back().y : concat.back().y;
    pickStrokeAnchor(*fromG, ax, ay, drawnFrom, &from);
    JsonValue::Object to;
    to.emplace_back("nodeId", JsonValue::string(toG->id));
    pickStrokeAnchor(*toG, bx, by, drawnTo, &to);

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
    refreshAllConnectorWarps(doc);
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
