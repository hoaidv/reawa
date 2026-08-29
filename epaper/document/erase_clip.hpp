#pragma once

/**
 * Geometric ink clip: polyline ∩ capsule or even-odd polygon.
 * @implements [SRS-EP-55] clip remnants
 * @implements [ADR-0034] remnant split, no chords
 */

#include "doc_model.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace epaper {
namespace document {

constexpr double kEraseBrushDiameterMm = 8.0;
constexpr double kEraseBrushRadiusMm = 4.0;
constexpr double kEraseHoverStrokeMm = 0.5;
constexpr double kEraseRemnantFloorMm = 1.0;
constexpr double kEraseClipScanMm = 0.25;

struct ErasePt {
    double x = 0;
    double y = 0;
};

struct ClipRegion {
    enum class Kind { Capsule, Polygon };
    Kind kind = Kind::Capsule;
    std::vector<ErasePt> path;
    double radius = kEraseBrushRadiusMm;
};

inline ClipRegion capsuleRegion(std::vector<ErasePt> spine, double radius = kEraseBrushRadiusMm)
{
    ClipRegion r;
    r.kind = ClipRegion::Kind::Capsule;
    r.path = std::move(spine);
    r.radius = radius;
    return r;
}

inline ClipRegion polygonRegion(std::vector<ErasePt> poly)
{
    ClipRegion r;
    r.kind = ClipRegion::Kind::Polygon;
    r.path = std::move(poly);
    r.radius = 0;
    return r;
}

inline double eraseDistPointSeg(double px, double py, double ax, double ay, double bx, double by)
{
    const double vx = bx - ax;
    const double vy = by - ay;
    const double L2 = vx * vx + vy * vy;
    double t = 0;
    if (L2 > 1e-12)
        t = std::max(0.0, std::min(1.0, ((px - ax) * vx + (py - ay) * vy) / L2));
    return std::hypot(px - (ax + t * vx), py - (ay + t * vy));
}

inline double eraseDistPointPolyline(double x, double y, const std::vector<ErasePt> &poly)
{
    if (poly.empty())
        return 1e300;
    if (poly.size() == 1)
        return std::hypot(x - poly[0].x, y - poly[0].y);
    double best = 1e300;
    for (size_t i = 1; i < poly.size(); ++i)
        best = std::min(best, eraseDistPointSeg(x, y, poly[i - 1].x, poly[i - 1].y, poly[i].x,
                                                poly[i].y));
    return best;
}

inline bool erasePointInPolygon(double x, double y, const std::vector<ErasePt> &poly)
{
    if (poly.size() < 3)
        return false;
    bool inside = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const double xi = poly[i].x, yi = poly[i].y;
        const double xj = poly[j].x, yj = poly[j].y;
        const bool intersect =
            ((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / ((yj - yi) + 0.0) + xi);
        if (intersect)
            inside = !inside;
    }
    return inside;
}

inline bool clipRegionContains(const ClipRegion &r, double x, double y)
{
    if (r.kind == ClipRegion::Kind::Capsule)
        return eraseDistPointPolyline(x, y, r.path) <= r.radius + 1e-9;
    return erasePointInPolygon(x, y, r.path);
}

inline InkSample lerpInkSample(const InkSample &a, const InkSample &b, double t)
{
    InkSample s;
    s.x = a.x + (b.x - a.x) * t;
    s.y = a.y + (b.y - a.y) * t;
    if (a.pressure && b.pressure)
        s.pressure = *a.pressure + (*b.pressure - *a.pressure) * t;
    else if (a.pressure)
        s.pressure = a.pressure;
    else if (b.pressure)
        s.pressure = b.pressure;
    if (a.t && b.t)
        s.t = *a.t + (*b.t - *a.t) * t;
    else if (a.t)
        s.t = a.t;
    else if (b.t)
        s.t = b.t;
    return s;
}

inline double polylineArcLength(const std::vector<InkSample> &s)
{
    double len = 0;
    for (size_t i = 1; i < s.size(); ++i)
        len += std::hypot(s[i].x - s[i - 1].x, s[i].y - s[i - 1].y);
    return len;
}

inline InkSample crossingOnSegment(const InkSample &a, const InkSample &b, const ClipRegion &r,
                                   bool aInside)
{
    double lo = 0;
    double hi = 1;
    for (int i = 0; i < 32; ++i) {
        const double mid = 0.5 * (lo + hi);
        const InkSample p = lerpInkSample(a, b, mid);
        const bool ins = clipRegionContains(r, p.x, p.y);
        if (ins == aInside)
            lo = mid;
        else
            hi = mid;
    }
    return lerpInkSample(a, b, 0.5 * (lo + hi));
}

struct ClipResult {
    bool hit = false;
    std::vector<std::vector<InkSample>> remnants;
};

/**
 * Split an Ink polyline against a clip region. Inside the region is erased.
 * Remnants keep original vertices plus segment ∩ region; 0 chords.
 */
inline ClipResult clipInkPolyline(const std::vector<InkSample> &samples, const ClipRegion &region)
{
    ClipResult out;
    if (samples.size() < 2)
        return out;

    auto contains = [&](const InkSample &s) { return clipRegionContains(region, s.x, s.y); };

    auto segmentCrossings = [&](const InkSample &a, const InkSample &b) {
        std::vector<double> ts;
        const double len = std::hypot(b.x - a.x, b.y - a.y);
        const int nsub = std::max(1, static_cast<int>(std::ceil(len / kEraseClipScanMm)));
        bool prev = contains(a);
        InkSample prevS = a;
        for (int k = 1; k <= nsub; ++k) {
            const double t = static_cast<double>(k) / static_cast<double>(nsub);
            const InkSample p = lerpInkSample(a, b, t);
            const bool ins = contains(p);
            if (ins != prev) {
                const InkSample x = crossingOnSegment(prevS, p, region, prev);
                const double dx = b.x - a.x;
                const double dy = b.y - a.y;
                const double denom = dx * dx + dy * dy;
                double xt = t;
                if (denom > 1e-18)
                    xt = ((x.x - a.x) * dx + (x.y - a.y) * dy) / denom;
                ts.push_back(std::max(0.0, std::min(1.0, xt)));
            }
            prev = ins;
            prevS = p;
        }
        return ts;
    };

    std::vector<InkSample> cur;
    auto flushRemnant = [&]() {
        if (cur.size() >= 2 && polylineArcLength(cur) + 1e-9 >= kEraseRemnantFloorMm)
            out.remnants.push_back(cur);
        cur.clear();
    };

    if (contains(samples[0]))
        out.hit = true;
    else
        cur.push_back(samples[0]);

    for (size_t i = 1; i < samples.size(); ++i) {
        const InkSample &a = samples[i - 1];
        const InkSample &b = samples[i];
        if (contains(b))
            out.hit = true;
        const auto ts = segmentCrossings(a, b);
        bool prevInside = contains(a);
        for (double t : ts) {
            out.hit = true;
            const InkSample x = lerpInkSample(a, b, t);
            if (!prevInside) {
                cur.push_back(x);
                flushRemnant();
            } else {
                cur.push_back(x);
            }
            prevInside = !prevInside;
        }
        if (!prevInside)
            cur.push_back(b);
    }
    flushRemnant();
    if (!out.hit)
        out.remnants.clear();
    return out;
}

} // namespace document
} // namespace epaper
