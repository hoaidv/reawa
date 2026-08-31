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
#include <memory>
#include <string>
#include <vector>

namespace epaper {
namespace document {

constexpr double kMinConnectorWorld = 48;
/** R_SNAP to boundary ink. R_JOIN is tight (6 u). UX2 window = last 3 free inks (B–A–C). */
constexpr double kConnectorSnapWorld = 24;
constexpr double kConnectorJoinWorld = 6;
/** Convex-hull area / L² — not AABB (a diagonal's AABB / L² is ~0.5). Wiggles must pass. */
constexpr double kPathLikeHullOverLen2 = 0.5;
constexpr int kConnectorChainMax = 3;

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

/** Newest free inks in paint order (skips SmartGroup / Connector / membership). */
inline std::vector<const DocNode *> lastFreeInks(const std::vector<DocNode> &root, int n)
{
    std::vector<const DocNode *> all;
    collectFreeInks(root, all);
    if (n <= 0 || all.empty())
        return {};
    if (int(all.size()) <= n)
        return all;
    return std::vector<const DocNode *>(all.end() - n, all.end());
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
    /** Segment end indices in the polylines that were tested (a[ia-1]–a[ia]). */
    size_t ia = 0;
    size_t ib = 0;
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

struct CoarsePoly {
    std::vector<InkSample> pts;
    std::vector<size_t> orig;
};

inline CoarsePoly coarsenForJoinIndexed(const std::vector<InkSample> &s, double minStep)
{
    CoarsePoly c;
    if (s.empty())
        return c;
    c.pts.push_back(s.front());
    c.orig.push_back(0);
    for (size_t i = 1; i + 1 < s.size(); ++i) {
        if (std::hypot(s[i].x - c.pts.back().x, s[i].y - c.pts.back().y) >= minStep) {
            c.pts.push_back(s[i]);
            c.orig.push_back(i);
        }
    }
    c.pts.push_back(s.back());
    c.orig.push_back(s.size() - 1);
    return c;
}

inline SegHit polylineIntersectDense(const std::vector<InkSample> &a,
                                     const std::vector<InkSample> &b,
                                     double joinTol = kConnectorJoinWorld,
                                     size_t aLo = 1, size_t aHi = static_cast<size_t>(-1),
                                     size_t bLo = 1, size_t bHi = static_cast<size_t>(-1))
{
    SegHit best;
    double bestT = 1e300;
    auto consider = [&](Vec2 p, double score, size_t ia, size_t ib) {
        if (!best.ok || score < bestT) {
            best.ok = true;
            best.p = p;
            bestT = score;
            best.ia = ia;
            best.ib = ib;
        }
    };
    if (aHi > a.size())
        aHi = a.size();
    if (bHi > b.size())
        bHi = b.size();
    aLo = std::max(aLo, size_t(1));
    bLo = std::max(bLo, size_t(1));
    for (size_t i = aLo; i < aHi; ++i) {
        const Vec2 a0{a[i - 1].x, a[i - 1].y};
        const Vec2 a1{a[i].x, a[i].y};
        for (size_t j = bLo; j < bHi; ++j) {
            const Vec2 b0{b[j - 1].x, b[j - 1].y};
            const Vec2 b1{b[j].x, b[j].y};
            const SegHit h = segmentIntersect(a0, a1, b0, b1);
            if (h.ok) {
                consider(h.p, 0, i, j);
                return best;
            }
        }
    }
    auto tHits = [&](const std::vector<InkSample> &poly, Vec2 end, bool polyIsA) {
        const size_t lo = polyIsA ? aLo : bLo;
        const size_t hi = polyIsA ? aHi : bHi;
        for (size_t i = lo; i < hi; ++i) {
            Vec2 q;
            const double d = distPointSegClamped(end, {poly[i - 1].x, poly[i - 1].y},
                                                 {poly[i].x, poly[i].y}, &q);
            if (d <= joinTol)
                consider(q, 1 + d, polyIsA ? i : 0, polyIsA ? 0 : i);
        }
    };
    if (a.size() >= 2 && b.size() >= 2) {
        tHits(b, {a.front().x, a.front().y}, false);
        tHits(b, {a.back().x, a.back().y}, false);
        tHits(a, {b.front().x, b.front().y}, true);
        tHits(a, {b.back().x, b.back().y}, true);
        if (best.ok)
            return best;
        for (size_t i = aLo; i < aHi; ++i) {
            const Vec2 a0{a[i - 1].x, a[i - 1].y};
            const Vec2 a1{a[i].x, a[i].y};
            for (size_t j = bLo; j < bHi; ++j) {
                Vec2 pa, pb;
                const double d = distSegSeg(a0, a1, {b[j - 1].x, b[j - 1].y}, {b[j].x, b[j].y}, &pa, &pb);
                if (d <= joinTol) {
                    consider({(pa.x + pb.x) * 0.5, (pa.y + pb.y) * 0.5}, 2 + d, i, j);
                    return best;
                }
            }
        }
    }
    return best;
}

/**
 * Every free-ink pair on the page reaches this test, and a naive dense form is
 * O(|a|*|b|): two 1900-sample strokes cost seconds on device, which is what
 * froze pen-up (grids with 2+ SmartGroups). Two rejection stages run first.
 *   1. Padded sample bounds — no point of one can be within tolerance of the
 *      other, so no branch below can fire.
 *   2. A coarsened re-test. Dropping samples moves the path by at most one
 *      step, so the filter runs at a tolerance inflated by 2 steps and can
 *      only over-accept, never hide a real touch. A hit is confirmed on a
 *      small original-sample window around the coarse segments — not a second
 *      full dense pass (that re-froze crossing grid lines).
 */
inline SegHit polylineIntersect(const std::vector<InkSample> &a, const std::vector<InkSample> &b)
{
    if (!sampleBoundsMayTouch(a, b, kConnectorJoinWorld))
        return SegHit{};
    const double step = kConnectorJoinWorld * 0.25;
    const CoarsePoly ca = coarsenForJoinIndexed(a, step);
    const CoarsePoly cb = coarsenForJoinIndexed(b, step);
    if (ca.pts.size() < a.size() || cb.pts.size() < b.size()) {
        const SegHit coarse =
            polylineIntersectDense(ca.pts, cb.pts, kConnectorJoinWorld + 2.0 * step);
        if (!coarse.ok)
            return SegHit{};
        const size_t iaPrev = coarse.ia > 0 ? coarse.ia - 1 : 0;
        const size_t ibPrev = coarse.ib > 0 ? coarse.ib - 1 : 0;
        const size_t ia0 = ca.orig[std::min(iaPrev, ca.orig.size() - 1)];
        const size_t ia1 = ca.orig[std::min(coarse.ia, ca.orig.size() - 1)];
        const size_t ib0 = cb.orig[std::min(ibPrev, cb.orig.size() - 1)];
        const size_t ib1 = cb.orig[std::min(coarse.ib, cb.orig.size() - 1)];
        const size_t aLo = ia0 > 2 ? ia0 - 1 : 1;
        const size_t bLo = ib0 > 2 ? ib0 - 1 : 1;
        const size_t aHi = std::min(a.size(), ia1 + 3);
        const size_t bHi = std::min(b.size(), ib1 + 3);
        const SegHit exact = polylineIntersectDense(a, b, kConnectorJoinWorld, aLo, aHi, bLo, bHi);
        if (exact.ok)
            return exact;
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

struct InkEnds {
    const DocNode *gFirst = nullptr;
    const DocNode *gLast = nullptr;
    int nShapeEnds() const { return (gFirst ? 1 : 0) + (gLast ? 1 : 0); }
    const DocNode *onlyGroup() const
    {
        if (gFirst && !gLast)
            return gFirst;
        if (gLast && !gFirst)
            return gLast;
        return nullptr;
    }
};

inline InkEnds classifyFreeInk(const DocNode &ink, const std::vector<const DocNode *> &groups)
{
    InkEnds e;
    if (ink.samples.size() < 2)
        return e;
    e.gFirst = nearestSnapGroup(groups, ink.samples.front().x, ink.samples.front().y);
    e.gLast = nearestSnapGroup(groups, ink.samples.back().x, ink.samples.back().y);
    return e;
}

inline bool chainBindsTwoGroups(const ChainBuild &b, const std::vector<const DocNode *> &groups)
{
    if (b.concat.size() < 2)
        return false;
    const DocNode *fromG = nearestSnapGroup(groups, b.concat.front().x, b.concat.front().y);
    if (!fromG)
        return false;
    const std::string fid = fromG->id;
    const DocNode *toG = nearestSnapGroup(groups, b.concat.back().x, b.concat.back().y, &fid);
    return toG && toG->id != fromG->id;
}

inline bool concatOriented(const std::vector<const DocNode *> &nodes,
                           const std::vector<const DocNode *> &groups, ChainBuild *out)
{
    for (int sf = 0; sf < 2; ++sf) {
        for (int ef = 0; ef < 2; ++ef) {
            if (concatPath(nodes, sf != 0, ef != 0, out) && chainBindsTwoGroups(*out, groups))
                return true;
        }
    }
    return false;
}

/** Last 3 free inks: B-A-C, else two arms B-C that join. Else current stroke. */
inline ChainBuild assembleChain(const std::vector<const DocNode *> &freeInks, int curIdx,
                                const std::vector<const DocNode *> &groups)
{
    ChainBuild best;
    const int nAll = int(freeInks.size());
    std::vector<const DocNode *> cand;
    int curLocal = -1;
    for (int i = 0; i < nAll; ++i) {
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
    std::vector<InkEnds> role(static_cast<size_t>(m));
    for (int i = 0; i < m; ++i)
        role[size_t(i)] = classifyFreeInk(*cand[size_t(i)], groups);

    if (m == 3) {
        for (int ai = 0; ai < 3; ++ai) {
            if (role[size_t(ai)].nShapeEnds() != 0)
                continue;
            int arms[2];
            int na = 0;
            bool okArms = true;
            for (int j = 0; j < 3; ++j) {
                if (j == ai)
                    continue;
                if (role[size_t(j)].nShapeEnds() != 1) {
                    okArms = false;
                    break;
                }
                arms[na++] = j;
            }
            if (!okArms || na != 2)
                continue;
            const DocNode *gB = role[size_t(arms[0])].onlyGroup();
            const DocNode *gC = role[size_t(arms[1])].onlyGroup();
            if (!gB || !gC || gB->id == gC->id)
                continue;
            if (!strokesJoin(*cand[size_t(ai)], *cand[size_t(arms[0])])
                || !strokesJoin(*cand[size_t(ai)], *cand[size_t(arms[1])]))
                continue;
            const std::vector<const DocNode *> bac = {cand[size_t(arms[0])], cand[size_t(ai)],
                                                      cand[size_t(arms[1])]};
            if (concatOriented(bac, groups, &best))
                return best;
            const std::vector<const DocNode *> cab = {cand[size_t(arms[1])], cand[size_t(ai)],
                                                      cand[size_t(arms[0])]};
            if (concatOriented(cab, groups, &best))
                return best;
        }
    }

    if (m >= 2 && role[size_t(curLocal)].nShapeEnds() == 1) {
        const DocNode *gCur = role[size_t(curLocal)].onlyGroup();
        if (gCur) {
            for (int j = m - 1; j >= 0; --j) {
                if (j == curLocal)
                    continue;
                if (role[size_t(j)].nShapeEnds() != 1)
                    continue;
                const DocNode *gO = role[size_t(j)].onlyGroup();
                if (!gO || gO->id == gCur->id)
                    continue;
                if (!strokesJoin(*cand[size_t(curLocal)], *cand[size_t(j)]))
                    continue;
                const std::vector<const DocNode *> bc = {cand[size_t(j)], cand[size_t(curLocal)]};
                if (concatOriented(bc, groups, &best))
                    return best;
                const std::vector<const DocNode *> cb = {cand[size_t(curLocal)], cand[size_t(j)]};
                if (concatOriented(cb, groups, &best))
                    return best;
            }
        }
    }

    concatPath({cand[size_t(curLocal)]}, true, false, &best);
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

inline ConnectorAnchor pickStrokeAnchor(const DocNode &sg, double x, double y, const RestVec &drawn)
{
    ConnectorAnchor a;
    a.nodeId = sg.id;
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
    a.kind = centre ? "centre" : "edge";
    a.edge = edge;
    a.t = t;
    a.localX = loc.x;
    a.localY = loc.y;
    a.hasLocal = true;
    a.drawnN = unitD.x * n.x + unitD.y * n.y;
    a.drawnE = unitD.x * e.x + unitD.y * e.y;
    a.drawnBoxX = unitD.x * ex.x + unitD.y * ex.y;
    a.drawnBoxY = unitD.x * ey.x + unitD.y * ey.y;
    return a;
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

    std::vector<const DocNode *> freeInks = lastFreeInks(doc.rootChildren, kConnectorChainMax);
    int curIdx = -1;
    for (int i = 0; i < int(freeInks.size()); ++i) {
        if (freeInks[size_t(i)]->id == strokeId)
            curIdx = i;
    }
    if (curIdx < 0) {
        freeInks = {cur};
        curIdx = 0;
    }

    auto rnd = [](double x) { return std::to_string(int(std::lround(x))); };
    std::string diag = "n=" + std::to_string(freeInks.size()) + " ids=";
    for (size_t i = 0; i < freeInks.size(); ++i) {
        if (i)
            diag += ",";
        diag += freeInks[i]->id;
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

    const double ax = rest.spine.size() >= 2 ? rest.spine.front().x : concat.front().x;
    const double ay = rest.spine.size() >= 2 ? rest.spine.front().y : concat.front().y;
    const double bx = rest.spine.size() >= 2 ? rest.spine.back().x : concat.back().x;
    const double by = rest.spine.size() >= 2 ? rest.spine.back().y : concat.back().y;
    ConnectorAnchor from = pickStrokeAnchor(*fromG, ax, ay, drawnFrom);
    ConnectorAnchor to = pickStrokeAnchor(*toG, bx, by, drawnTo);

    std::vector<std::string> cap;
    for (const auto &id : bodyIds)
        cap.push_back(id);

    const std::string cid = doc.generateNodeId();
    CreateConnectorEdit edit;
    edit.setId(std::string("create_connector:") + cid);
    edit.setSource("epaper");
    edit.setNodeId(cid);
    edit.setFrom(std::move(from));
    edit.setTo(std::move(to));
    edit.setWarpStyle(rest.warpStyle);
    std::vector<ConnectorRestPt> spine;
    for (const auto &p : rest.spine)
        spine.push_back({p.x, p.y});
    std::vector<ConnectorRestOff> offsets;
    for (const auto &p : rest.offsets)
        offsets.push_back({p.s, p.d});
    edit.setRestSpine(std::move(spine));
    edit.setRestOffsets(std::move(offsets));
    edit.setCaptureIds(std::move(cap));
    const ApplyResult r = doc.commitEdit(edit);
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
