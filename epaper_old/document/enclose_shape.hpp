#pragma once
/**
 * Empty-boundary enclose: primitive-shape gate (SRS-EP-10).
 * With content, size ≥ 28 is enough. Empty boxes need ≥ 36 and a near-primitive
 * outline so letters (O, D, P, Q, G, C) and scribbles stay ink.
 */
#include "ingest_stroke.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace epaper {
namespace document {

constexpr double kMinEncloseWithContent = 36;
constexpr double kMinEncloseEmpty = 42;
/** Backward alias: content-path floor (human 2026-08-15). */
constexpr double kMinEncloseWorld = kMinEncloseWithContent;

constexpr double kShapeDpFrac = 0.14;
constexpr double kCuspPeelFrac = 0.20;
constexpr double kCircularityCircle = 0.64;
constexpr double kCircularityEllipse = 0.45;
constexpr double kFillMinNgon = 0.22;
constexpr double kFillMinQuad = 0.32;
constexpr double kFillHandQuad = 0.42;
constexpr double kSideCvRegular = 0.55;
constexpr double kParallelAbsDot = 0.68;
constexpr double kPerpAbsDot = 0.52;
constexpr double kSquareAspectMax = 1.35;
constexpr double kRadiusCvEllipse = 0.40;

struct EmptyShapeVerdict {
    bool ok = false;
    std::string name = "none";
    int nVerts = 0;
    double circularity = 0;
    double fill = 0;

    std::string diag() const
    {
        return "shape=" + name + " n=" + std::to_string(nVerts) + " circ="
            + std::to_string(int(std::lround(circularity * 100))) + " fill="
            + std::to_string(int(std::lround(fill * 100)));
    }
};

namespace shape_detail {

struct P {
    double x = 0;
    double y = 0;
};

inline double dist(P a, P b)
{
    return std::hypot(a.x - b.x, a.y - b.y);
}

inline double dotU(P a, P b)
{
    const double na = std::hypot(a.x, a.y);
    const double nb = std::hypot(b.x, b.y);
    if (na < 1e-9 || nb < 1e-9)
        return 0;
    return (a.x * b.x + a.y * b.y) / (na * nb);
}

inline double peri(const std::vector<P> &p)
{
    if (p.size() < 2)
        return 0;
    double L = 0;
    for (size_t i = 1; i < p.size(); ++i)
        L += dist(p[i - 1], p[i]);
    return L;
}

inline double shoelace(const std::vector<P> &p)
{
    if (p.size() < 3)
        return 0;
    double a = 0;
    for (size_t i = 0; i < p.size(); ++i) {
        const P &u = p[i];
        const P &v = p[(i + 1) % p.size()];
        a += u.x * v.y - v.x * u.y;
    }
    return std::abs(a) * 0.5;
}

inline double pointSegDist(P p, P a, P b)
{
    const double vx = b.x - a.x;
    const double vy = b.y - a.y;
    const double l2 = vx * vx + vy * vy;
    if (l2 < 1e-12)
        return dist(p, a);
    double t = ((p.x - a.x) * vx + (p.y - a.y) * vy) / l2;
    t = std::max(0.0, std::min(1.0, t));
    return std::hypot(p.x - (a.x + t * vx), p.y - (a.y + t * vy));
}

inline void dpRec(const std::vector<P> &pts, int a, int b, double eps, std::vector<char> *keep)
{
    if (b <= a + 1)
        return;
    double maxd = -1;
    int idx = a;
    for (int i = a + 1; i < b; ++i) {
        const double d = pointSegDist(pts[size_t(i)], pts[size_t(a)], pts[size_t(b)]);
        if (d > maxd) {
            maxd = d;
            idx = i;
        }
    }
    if (maxd > eps) {
        (*keep)[size_t(idx)] = 1;
        dpRec(pts, a, idx, eps, keep);
        dpRec(pts, idx, b, eps, keep);
    }
}

inline std::vector<P> simplifyDp(const std::vector<P> &pts, double eps)
{
    if (pts.size() < 3)
        return pts;
    std::vector<char> keep(pts.size(), 0);
    keep.front() = 1;
    keep.back() = 1;
    dpRec(pts, 0, int(pts.size()) - 1, eps, &keep);
    std::vector<P> out;
    for (size_t i = 0; i < pts.size(); ++i)
        if (keep[i])
            out.push_back(pts[i]);
    return out;
}

inline std::vector<P> closedRing(std::vector<P> p)
{
    if (p.size() >= 2 && dist(p.front(), p.back()) < 1e-6)
        p.pop_back();
    return p;
}

inline double mean(const std::vector<double> &v)
{
    if (v.empty())
        return 0;
    double s = 0;
    for (double x : v)
        s += x;
    return s / double(v.size());
}

inline double cv(const std::vector<double> &v)
{
    if (v.size() < 2)
        return 0;
    const double m = mean(v);
    if (m < 1e-9)
        return 1;
    double var = 0;
    for (double x : v)
        var += (x - m) * (x - m);
    var /= double(v.size());
    return std::sqrt(var) / m;
}

inline std::vector<double> sideLens(const std::vector<P> &ring)
{
    std::vector<double> s;
    const int n = int(ring.size());
    s.reserve(size_t(n));
    for (int i = 0; i < n; ++i)
        s.push_back(dist(ring[size_t(i)], ring[size_t((i + 1) % n)]));
    return s;
}

inline bool parallelOpp(const std::vector<P> &r)
{
    if (r.size() != 4)
        return false;
    P e0{r[1].x - r[0].x, r[1].y - r[0].y};
    P e1{r[2].x - r[1].x, r[2].y - r[1].y};
    P e2{r[3].x - r[2].x, r[3].y - r[2].y};
    P e3{r[0].x - r[3].x, r[0].y - r[3].y};
    return std::abs(dotU(e0, e2)) >= kParallelAbsDot && std::abs(dotU(e1, e3)) >= kParallelAbsDot;
}

inline bool adjacentPerp(const std::vector<P> &r)
{
    if (r.size() != 4)
        return false;
    P e0{r[1].x - r[0].x, r[1].y - r[0].y};
    P e1{r[2].x - r[1].x, r[2].y - r[1].y};
    return std::abs(dotU(e0, e1)) <= kPerpAbsDot;
}

/** Drop shallow extra cusps (hand-drawn square is never exactly 4 corners). */
inline std::vector<P> peelShallowCusps(std::vector<P> ring, double eps, int target)
{
    while (int(ring.size()) > target) {
        const int n = int(ring.size());
        int best = -1;
        double bestD = eps;
        for (int i = 0; i < n; ++i) {
            const P &prev = ring[size_t((i - 1 + n) % n)];
            const P &cur = ring[size_t(i)];
            const P &next = ring[size_t((i + 1) % n)];
            const double d = pointSegDist(cur, prev, next);
            if (d <= bestD) {
                bestD = d;
                best = i;
            }
        }
        if (best < 0)
            break;
        ring.erase(ring.begin() + best);
    }
    return ring;
}

inline const char *nameQuad(const std::vector<P> &ring, double fill, double aspect)
{
    if (ring.size() != 4 || fill < kFillMinQuad)
        return nullptr;
    const auto sides = sideLens(ring);
    const bool regularSides = cv(sides) <= kSideCvRegular;
    const bool para = parallelOpp(ring);
    const bool rectish = para && adjacentPerp(ring);
    if (rectish && regularSides && aspect <= kSquareAspectMax)
        return "square";
    if (rectish)
        return "rectangle";
    if (regularSides && !rectish)
        return (aspect <= kSquareAspectMax) ? "diamond" : "parallelogram";
    if (para)
        return "parallelogram";
    if (fill >= kFillHandQuad)
        return "rectangle";
    return nullptr;
}

} // namespace shape_detail

/** @implements [SRS-EP-10] empty-boundary primitive gate */
inline EmptyShapeVerdict classifyEmptyBoundaryShape(const std::vector<InkSample> &samples,
                                                    double aabbX, double aabbY, double aabbW,
                                                    double aabbH)
{
    using namespace shape_detail;
    EmptyShapeVerdict v;
    std::vector<P> raw;
    raw.reserve(samples.size());
    for (const auto &s : samples)
        raw.push_back({s.x, s.y});
    if (raw.size() < 3)
        return v;

    const double w = std::max(1e-6, aabbW);
    const double h = std::max(1e-6, aabbH);
    const double aabbArea = w * h;
    const std::vector<P> ringFull = closedRing(raw);
    const double area = shoelace(ringFull);
    const double pr = peri(raw);
    v.circularity = (pr > 1e-6) ? (4.0 * std::acos(-1.0) * area / (pr * pr)) : 0;
    v.fill = area / aabbArea;

    auto radiusCv = [&]() {
        const P c{(aabbX + w * 0.5), (aabbY + h * 0.5)};
        std::vector<double> rs;
        rs.reserve(ringFull.size());
        for (const auto &p : ringFull) {
            const double nx = (p.x - c.x) / (w * 0.5);
            const double ny = (p.y - c.y) / (h * 0.5);
            rs.push_back(std::hypot(nx, ny));
        }
        return cv(rs);
    };

    if (v.circularity >= kCircularityCircle) {
        v.ok = true;
        v.name = "circle";
        v.nVerts = 0;
        return v;
    }
    if (v.circularity >= kCircularityEllipse && radiusCv() <= kRadiusCvEllipse) {
        v.ok = true;
        v.name = "ellipse";
        v.nVerts = 0;
        return v;
    }

    const double shorter = std::min(w, h);
    std::vector<P> simp = simplifyDp(raw, kShapeDpFrac * shorter);
    std::vector<P> ring = closedRing(simp);
    v.nVerts = int(ring.size());
    if (v.nVerts < 3)
        return v;

    // Hand-drawn primitives pick up extra cusps. Peel shallow ones toward 4, then 3.
    if (v.nVerts > 4)
        ring = peelShallowCusps(ring, kCuspPeelFrac * shorter, 4);
    if (int(ring.size()) > 4 && int(ring.size()) <= 8)
        ring = peelShallowCusps(ring, kCuspPeelFrac * shorter * 1.25, 4);
    v.nVerts = int(ring.size());

    const auto sides = sideLens(ring);
    const bool regularSides = cv(sides) <= kSideCvRegular;
    const double aspect = std::max(w, h) / std::min(w, h);

    if (v.nVerts == 3 && v.fill >= kFillMinNgon && regularSides) {
        v.ok = true;
        v.name = "triangle";
        return v;
    }
    if (v.nVerts == 4) {
        if (const char *nm = nameQuad(ring, v.fill, aspect)) {
            v.ok = true;
            v.name = nm;
            return v;
        }
        return v;
    }
    if ((v.nVerts == 5 || v.nVerts == 6 || v.nVerts == 8) && v.fill >= kFillMinNgon && regularSides) {
        v.ok = true;
        if (v.nVerts == 5)
            v.name = "pentagon";
        else if (v.nVerts == 6)
            v.name = "hexagon";
        else
            v.name = "octagon";
        return v;
    }
    return v;
}

} // namespace document
} // namespace epaper
