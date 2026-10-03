#pragma once
/**
 * Rest spine S and (s,d) offsets at recognition. Never re-baked later.
 * @implements [SRS-EP-17] rest shape + warpStyle from inflection count of S
 * @implements [ADR-0020] resample kRestResampleWorld, binomial kRestSigmaWorld, pin ends
 */

#include "connector_warp_params.hpp"
#include "device_document.hpp"

#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace epaper {
namespace document {

struct RestVec {
    double x = 0;
    double y = 0;
};

struct RestOffset {
    double s = 0;
    double d = 0;
};

struct RestShape {
    std::vector<RestVec> spine;
    std::vector<RestOffset> offsets;
    int inflections = 0;
    std::string warpStyle = "morph";
};

inline double restLen(const std::vector<RestVec> &p)
{
    double L = 0;
    for (size_t i = 1; i < p.size(); ++i)
        L += std::hypot(p[i].x - p[i - 1].x, p[i].y - p[i - 1].y);
    return L;
}

inline std::vector<RestVec> resampleArc(const std::vector<RestVec> &p, double spacing)
{
    std::vector<RestVec> out;
    if (p.size() < 2) {
        return p;
    }
    const double L = restLen(p);
    if (L < 1e-6)
        return {p.front(), p.back()};
    const int n = std::max(2, int(std::lround(L / spacing)) + 1);
    out.reserve(size_t(n));
    out.push_back(p.front());
    size_t seg = 1;
    double segStart = 0;
    for (int k = 1; k < n - 1; ++k) {
        const double target = (double(k) / double(n - 1)) * L;
        while (seg < p.size()) {
            const double sl = std::hypot(p[seg].x - p[seg - 1].x, p[seg].y - p[seg - 1].y);
            if (segStart + sl >= target)
                break;
            segStart += sl;
            ++seg;
        }
        if (seg >= p.size())
            break;
        const double sl = std::hypot(p[seg].x - p[seg - 1].x, p[seg].y - p[seg - 1].y);
        const double t = sl > 1e-9 ? (target - segStart) / sl : 0;
        RestVec q;
        q.x = p[seg - 1].x + t * (p[seg].x - p[seg - 1].x);
        q.y = p[seg - 1].y + t * (p[seg].y - p[seg - 1].y);
        out.push_back(q);
    }
    out.push_back(p.back());
    return out;
}

inline void binomialSmoothPinned(std::vector<RestVec> &s, int passes)
{
    if (s.size() < 3)
        return;
    for (int p = 0; p < passes; ++p) {
        std::vector<RestVec> next = s;
        for (size_t i = 1; i + 1 < s.size(); ++i) {
            next[i].x = (s[i - 1].x + 2.0 * s[i].x + s[i + 1].x) * 0.25;
            next[i].y = (s[i - 1].y + 2.0 * s[i].y + s[i + 1].y) * 0.25;
        }
        s.swap(next);
    }
}

inline int countInflections(const std::vector<RestVec> &s)
{
    int n = 0;
    int lastSign = 0;
    for (size_t i = 1; i + 1 < s.size(); ++i) {
        const double ax = s[i].x - s[i - 1].x;
        const double ay = s[i].y - s[i - 1].y;
        const double bx = s[i + 1].x - s[i].x;
        const double by = s[i + 1].y - s[i].y;
        const double cross = ax * by - ay * bx;
        const double mag = std::hypot(ax, ay) * std::hypot(bx, by);
        if (mag < 1e-9 || std::abs(cross) < 1e-6 * mag)
            continue;
        const int sign = cross > 0 ? 1 : -1;
        if (lastSign != 0 && sign != lastSign)
            ++n;
        lastSign = sign;
    }
    return n;
}

inline RestOffset projectOnSpine(const RestVec &p, const std::vector<RestVec> &s)
{
    RestOffset o;
    if (s.size() < 2)
        return o;
    double best = 1e300;
    double acc = 0;
    const double L = restLen(s);
    for (size_t i = 1; i < s.size(); ++i) {
        const double vx = s[i].x - s[i - 1].x;
        const double vy = s[i].y - s[i - 1].y;
        const double sl = std::hypot(vx, vy);
        double t = 0;
        if (sl > 1e-9)
            t = ((p.x - s[i - 1].x) * vx + (p.y - s[i - 1].y) * vy) / (sl * sl);
        t = std::max(0.0, std::min(1.0, t));
        const RestVec q{s[i - 1].x + t * vx, s[i - 1].y + t * vy};
        const double dx = p.x - q.x;
        const double dy = p.y - q.y;
        const double d2 = dx * dx + dy * dy;
        if (d2 < best) {
            best = d2;
            o.s = L > 1e-9 ? (acc + t * sl) / L : 0;
            const double nx = sl > 1e-9 ? -vy / sl : 0;
            const double ny = sl > 1e-9 ? vx / sl : 0;
            o.d = dx * nx + dy * ny;
        }
        acc += sl;
    }
    return o;
}

inline int restSmoothPasses()
{
    const double sp = kRestResampleWorld;
    const double sig = kRestSigmaWorld;
    if (sp < 1e-12)
        return 1;
    return std::max(1, int(std::lround(2.0 * sig * sig / (sp * sp))));
}

/** Concatenate raw polylines in draw order; no joining segments. */
inline RestShape buildRestShape(const std::vector<std::vector<InkSample>> &strokes)
{
    RestShape r;
    std::vector<RestVec> raw;
    for (const auto &st : strokes) {
        for (const auto &s : st)
            raw.push_back({s.x, s.y});
    }
    if (raw.size() < 2)
        return r;
    auto spaced = resampleArc(raw, kRestResampleWorld);
    binomialSmoothPinned(spaced, restSmoothPasses());
    r.spine = resampleArc(spaced, kRestResampleWorld);
    r.inflections = countInflections(r.spine);
    r.warpStyle = r.inflections <= kInflectionCubicMax ? "cubic" : "morph";
    r.offsets.reserve(raw.size());
    for (const auto &p : raw)
        r.offsets.push_back(projectOnSpine(p, r.spine));
    return r;
}

} // namespace document
} // namespace epaper
