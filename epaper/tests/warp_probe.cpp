/**
 * EXP-0002 rounds 1-2 — connector-ink warp probe.
 *
 * SPIKE CODE. Host-only: no Qt, no third-party libraries, no network, no device.
 * Throwaway sandbox artefact for the warp naturalness question (BS-0001 D3/D5/D8/D16/D17/D26).
 * Carries no traceability annotations on purpose: this is not a shipping path and must be
 * re-implemented docs-first if any of it is ever promoted.
 *
 * Stores a drawn connector body as a rest shape in spine coordinates (s, d) and warps it
 * from two endpoint anchors under four variants:
 *   A    similarity (uniform scale + rotation from the chord)
 *   B    anisotropic (chord-only scale, normal offsets untouched)
 *   A+C  A, then a Hermite tangent blend over the first/last T of arc length
 *   B+C  B, then the same blend
 *
 * Round 2 adds: aligned/reversed anchor partition (D26), the centre-bind escape hatch (W7),
 * degenerate clamp policies, a departure stub decoupled from the blend length, and a W5
 * metric that scores the blend region against an absolute minimum-radius bar.
 *
 * Writes an SVG contact sheet plus index.html at the recommended defaults, and prints the
 * sweeps and W2..W7 to stdout.
 */

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------- tunables

constexpr double kPi = 3.14159265358979323846;
constexpr double kResampleWorld = 2.0;    // uniform arc-length spacing of the spine
constexpr double kSpineSigmaWorld = 6.0;  // smoothing sigma used to derive the spine
constexpr int kVariantCount = 4;

// W5 (round 2). Inside the blend region the spine is a Hermite, not the creator's line, so
// it is scored against an absolute bar: the warp may not bend the ink tighter than this
// radius. Outside it, the rest-relative bar from round 1 still applies.
constexpr double kBlendMinRadiusWorld = 12.0;
constexpr double kBlendMinRadiusTightWorld = 8.0;   // reported for sensitivity only
constexpr double kBlendMinRadiusLooseWorld = 16.0;  // reported for sensitivity only
// A blend spanning S of arc cannot be held to a radius larger than about S/2 (a quarter
// turn over S already needs radius 0.64*S), so on very short connectors the bar relaxes
// with the span it is scoring. Without this the ~3 u degenerate case reports ~50 cusps
// that no parameter choice could remove.
constexpr double kBlendRadiusSpanFactor = 0.5;

// Departure stub cap. The stub lives inside a blend region whose arc length is T*L', so the
// cap is expressed against that span; at the recommended T = 0.15 this is 0.225 * L'.
constexpr double kStubCapFactor = 1.5;

// Recommended defaults — set from the round-2 sweeps printed below.
constexpr double kDefaultT = 0.15;
constexpr bool kDefaultStubAbsolute = false;
constexpr double kDefaultStubK = 1.5;
constexpr double kDefaultStubWorld = 12.0;
constexpr bool kDefaultLocalParam = true;
constexpr bool kDefaultScaleFloor = false;
constexpr double kDefaultScaleFloorValue = 0.15;
constexpr bool kDefaultDTaper = false;

// ---------------------------------------------------------------- vec2

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
Vec2 operator*(Vec2 a, double k) { return {a.x * k, a.y * k}; }
double dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
double cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }
double vlen(Vec2 a) { return std::sqrt(dot(a, a)); }
Vec2 unit(Vec2 a)
{
    const double l = vlen(a);
    return l > 1e-12 ? Vec2{a.x / l, a.y / l} : Vec2{1.0, 0.0};
}
Vec2 leftNormal(Vec2 t) { return {-t.y, t.x}; }
Vec2 rot2(Vec2 v, double th)
{
    const double c = std::cos(th);
    const double s = std::sin(th);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}
double angleBetweenDeg(Vec2 a, Vec2 b)
{
    const Vec2 ua = unit(a);
    const Vec2 ub = unit(b);
    double c = dot(ua, ub);
    c = std::max(-1.0, std::min(1.0, c));
    return std::acos(c) * 180.0 / kPi;
}

using Poly = std::vector<Vec2>;

// ---------------------------------------------------------------- polyline helpers

std::vector<double> arcTable(const Poly &p)
{
    std::vector<double> cum(p.size(), 0.0);
    for (size_t i = 1; i < p.size(); ++i)
        cum[i] = cum[i - 1] + vlen(p[i] - p[i - 1]);
    return cum;
}

// Unit tangent of segment [i, i+1]; walks outward past degenerate segments so a
// collapsed spine still yields a usable frame.
Vec2 segTangentAt(const Poly &p, size_t i)
{
    const size_t n = p.size();
    if (n < 2)
        return {1.0, 0.0};
    for (size_t j = i + 1; j < n; ++j) {
        const Vec2 d = p[j] - p[i];
        if (dot(d, d) > 1e-24)
            return unit(d);
    }
    for (size_t j = i; j-- > 0;) {
        const Vec2 d = p[i] - p[j];
        if (dot(d, d) > 1e-24)
            return unit(d);
    }
    return {1.0, 0.0};
}

struct Frame {
    Vec2 p;
    Vec2 t;
};

Frame frameAtArc(const Poly &p, const std::vector<double> &cum, double arc)
{
    if (p.empty())
        return {{0.0, 0.0}, {1.0, 0.0}};
    if (p.size() < 2)
        return {p[0], {1.0, 0.0}};
    const double total = cum.back();
    if (!(arc > 0.0))
        return {p.front(), segTangentAt(p, 0)};
    if (arc >= total)
        return {p.back(), segTangentAt(p, p.size() - 2)};
    size_t lo = 0;
    size_t hi = p.size() - 1;
    while (lo + 1 < hi) {
        const size_t mid = (lo + hi) / 2;
        if (cum[mid] <= arc)
            lo = mid;
        else
            hi = mid;
    }
    const double segLen = cum[lo + 1] - cum[lo];
    const double u = segLen > 1e-12 ? (arc - cum[lo]) / segLen : 0.0;
    return {p[lo] + (p[lo + 1] - p[lo]) * u, segTangentAt(p, lo)};
}

Poly resampleUniform(const Poly &src, double spacing)
{
    Poly clean;
    clean.reserve(src.size());
    for (const Vec2 &v : src) {
        if (clean.empty() || vlen(v - clean.back()) > 1e-9)
            clean.push_back(v);
    }
    if (clean.size() < 2)
        return clean;
    const std::vector<double> cum = arcTable(clean);
    const double total = cum.back();
    if (total <= 1e-9)
        return clean;
    const int n = std::max(4, static_cast<int>(std::llround(total / spacing)) + 1);
    Poly out;
    out.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const double arc = total * static_cast<double>(i) / static_cast<double>(n - 1);
        out.push_back(frameAtArc(clean, cum, arc).p);
    }
    out.front() = clean.front();
    out.back() = clean.back();
    return out;
}

// Repeated binomial [1,2,1]/4 approximating a Gaussian of the requested sigma.
// Endpoints are pinned throughout, so the smoothing tapers naturally at the ends.
Poly smoothToSigma(const Poly &src, double sigmaWorld, double spacing)
{
    if (src.size() < 3)
        return src;
    const int passes = std::max(1, static_cast<int>(std::llround(2.0 * sigmaWorld * sigmaWorld
                                                                 / (spacing * spacing))));
    Poly cur = src;
    Poly next = src;
    for (int pass = 0; pass < passes; ++pass) {
        for (size_t i = 1; i + 1 < cur.size(); ++i)
            next[i] = (cur[i - 1] + cur[i] * 2.0 + cur[i + 1]) * 0.25;
        cur.swap(next);
    }
    cur.front() = src.front();
    cur.back() = src.back();
    return cur;
}

double maxDeviation(const Poly &a, const Poly &b)
{
    if (a.size() != b.size())
        return 1e300;
    double m = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
        m = std::max(m, vlen(a[i] - b[i]));
    return m;
}

bool properCross(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4)
{
    const double d1 = cross(p4 - p3, p1 - p3);
    const double d2 = cross(p4 - p3, p2 - p3);
    const double d3 = cross(p2 - p1, p3 - p1);
    const double d4 = cross(p2 - p1, p4 - p1);
    return ((d1 > 0.0) != (d2 > 0.0)) && ((d3 > 0.0) != (d4 > 0.0));
}

// ---------------------------------------------------------------- rest shape

struct SD {
    double s = 0.0;  // normalized arc-length position along the rest spine, [0,1]
    double d = 0.0;  // signed perpendicular offset, world units, absolute by default (D8)
};

SD projectToSpine(const Poly &S, const std::vector<double> &cum, Vec2 p)
{
    double best = 1e300;
    double bestArc = 0.0;
    double bestD = 0.0;
    for (size_t i = 0; i + 1 < S.size(); ++i) {
        const Vec2 seg = S[i + 1] - S[i];
        const double l2 = dot(seg, seg);
        if (l2 < 1e-18)
            continue;
        double u = dot(p - S[i], seg) / l2;
        u = std::max(0.0, std::min(1.0, u));
        const Vec2 q = S[i] + seg * u;
        const Vec2 r = p - q;
        const double dist2 = dot(r, r);
        if (dist2 < best) {
            best = dist2;
            bestArc = cum[i] + u * std::sqrt(l2);
            bestD = dot(r, leftNormal(segTangentAt(S, i)));
        }
    }
    const double total = cum.empty() ? 0.0 : cum.back();
    SD out;
    out.s = total > 1e-12 ? bestArc / total : 0.0;
    out.d = bestD;
    return out;
}

struct RestShape {
    Poly raw;                        // strokes concatenated in draw order
    std::vector<size_t> strokeEnds;  // exclusive end index of each source stroke
    Poly spine;
    std::vector<double> cum;
    double spineLen = 0.0;
    std::vector<SD> sd;
    Vec2 chord{};
    double chordLen = 0.0;
    double restMaxTurnDeg = 0.0;
};

double maxTurnDeg(const Poly &p)
{
    double m = 0.0;
    for (size_t i = 1; i + 1 < p.size(); ++i) {
        const Vec2 a = p[i] - p[i - 1];
        const Vec2 b = p[i + 1] - p[i];
        if (dot(a, a) < 1e-20 || dot(b, b) < 1e-20)
            continue;
        m = std::max(m, angleBetweenDeg(a, b));
    }
    return m;
}

RestShape buildRestShape(const std::vector<Poly> &strokes)
{
    RestShape rs;
    for (const Poly &s : strokes) {
        rs.raw.insert(rs.raw.end(), s.begin(), s.end());
        rs.strokeEnds.push_back(rs.raw.size());
    }
    const Poly r = resampleUniform(rs.raw, kResampleWorld);
    const Poly sm = smoothToSigma(r, kSpineSigmaWorld, kResampleWorld);
    rs.spine = resampleUniform(sm, kResampleWorld);
    rs.cum = arcTable(rs.spine);
    rs.spineLen = rs.cum.empty() ? 0.0 : rs.cum.back();
    rs.sd.reserve(rs.raw.size());
    for (const Vec2 &p : rs.raw)
        rs.sd.push_back(projectToSpine(rs.spine, rs.cum, p));
    rs.chord = rs.spine.back() - rs.spine.front();
    rs.chordLen = vlen(rs.chord);
    rs.restMaxTurnDeg = maxTurnDeg(rs.spine);
    return rs;
}

// ---------------------------------------------------------------- warp

struct WarpParams {
    double T = kDefaultT;         // blend span, fraction of arc length
    bool blendAbsolute = false;   // instead derive T from an absolute blend length
    double blendWorld = 90.0;
    bool stubAbsolute = true;     // departure stub in world units vs multiple of T*L'
    double stubK = 1.0;           // relative mode: outer handle = stubK * T * L'
    double stubWorld = kDefaultStubWorld;
    double stubCapFactor = kStubCapFactor;  // outer handle <= stubCapFactor * T * L'
    bool stubAppliesToInner = false;        // if false the inner handle stays at T*L'
    // Place samples by the pre-blend arc parameterization, so the blend cannot shift ink
    // outside its own region through a change in total arc length.
    bool localParam = kDefaultLocalParam;
    bool scaleFloor = false;
    double scaleFloorValue = kDefaultScaleFloorValue;
    bool dTaper = false;
    // Variant B keeps the spine's perpendicular extent unscaled, so under compression the
    // spike lives in the spine, where a d-taper cannot reach it. This is the only policy
    // tested that can touch it.
    bool anisoTaper = false;
};

WarpParams defaultParams()
{
    WarpParams p;
    p.T = kDefaultT;
    p.stubAbsolute = kDefaultStubAbsolute;
    p.stubK = kDefaultStubK;
    p.stubWorld = kDefaultStubWorld;
    p.localParam = kDefaultLocalParam;
    p.scaleFloor = kDefaultScaleFloor;
    p.dTaper = kDefaultDTaper;
    return p;
}

Poly warpSpineSimilarity(const RestShape &rs, Vec2 p0, Vec2 p1, double scale)
{
    const Vec2 cNew = p1 - p0;
    const double lNew = vlen(cNew);
    const double th = (lNew > 1e-9 && rs.chordLen > 1e-9)
        ? std::atan2(cNew.y, cNew.x) - std::atan2(rs.chord.y, rs.chord.x)
        : 0.0;
    Poly out(rs.spine.size());
    const Vec2 o = rs.spine.front();
    for (size_t i = 0; i < rs.spine.size(); ++i)
        out[i] = p0 + rot2((rs.spine[i] - o) * scale, th);
    return out;
}

Poly warpSpineAniso(const RestShape &rs, Vec2 p0, Vec2 p1, double scale, double vFactor)
{
    const Vec2 cNew = p1 - p0;
    const double lNew = vlen(cNew);
    const Vec2 uh = rs.chordLen > 1e-9 ? rs.chord * (1.0 / rs.chordLen) : Vec2{1.0, 0.0};
    const Vec2 nh = leftNormal(uh);
    const Vec2 uh2 = lNew > 1e-9 ? cNew * (1.0 / lNew) : uh;
    const Vec2 nh2 = leftNormal(uh2);
    Poly out(rs.spine.size());
    const Vec2 o = rs.spine.front();
    for (size_t i = 0; i < rs.spine.size(); ++i) {
        const Vec2 r = rs.spine[i] - o;
        out[i] = p0 + uh2 * (dot(r, uh) * scale) + nh2 * (dot(r, nh) * vFactor);
    }
    return out;
}

Vec2 hermite(Vec2 p0, Vec2 m0, Vec2 p1, Vec2 m1, double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    return p0 * (2 * t3 - 3 * t2 + 1) + m0 * (t3 - 2 * t2 + t) + p1 * (-2 * t3 + 3 * t2)
        + m1 * (t3 - t2);
}

// Variant C. Replaces the first and last T of arc length with a cubic Hermite that leaves
// p0 along f0 and arrives at p1 along -f1. `outerMag` is the departure stub (how straight
// the line leaves the face); `innerMag` matches the untouched spine's own speed.
Poly tangentBlend(const Poly &sp, Vec2 p0, Vec2 f0, Vec2 p1, Vec2 f1, double T,
                  double outerMag, double innerMag)
{
    if (sp.size() < 6)
        return sp;
    const std::vector<double> cum = arcTable(sp);
    const double total = cum.back();
    if (total < 1e-9)
        return sp;
    const Frame fT = frameAtArc(sp, cum, T * total);
    const Frame fE = frameAtArc(sp, cum, (1.0 - T) * total);
    const Vec2 m0 = f0 * outerMag;
    const Vec2 m1 = fT.t * innerMag;
    const Vec2 n0 = fE.t * innerMag;
    const Vec2 n1 = f1 * -outerMag;
    Poly out = sp;
    for (size_t i = 0; i < sp.size(); ++i) {
        const double s = cum[i] / total;
        if (s < T)
            out[i] = hermite(p0, m0, fT.p, m1, s / T);
        else if (s > 1.0 - T)
            out[i] = hermite(fE.p, n0, p1, n1, (s - (1.0 - T)) / T);
    }
    out.front() = p0;
    out.back() = p1;
    return out;
}

// `paramSpine` supplies the arc-length parameterization, `geomSpine` the geometry. They are
// the same array unless localParam is on, in which case the pre-blend spine parameterizes
// and the blended spine positions — the two share indices, so middle samples are untouched.
Poly placeSamples(const Poly &paramSpine, const Poly &geomSpine, const std::vector<SD> &sd,
                  double dFactor)
{
    const std::vector<double> cum = arcTable(paramSpine);
    const double total = cum.empty() ? 0.0 : cum.back();
    Poly out(sd.size());
    for (size_t i = 0; i < sd.size(); ++i) {
        const double arc = sd[i].s * total;
        size_t lo = 0;
        double u = 0.0;
        if (paramSpine.size() >= 2 && total > 1e-12) {
            if (arc >= total) {
                lo = paramSpine.size() - 2;
                u = 1.0;
            } else if (arc > 0.0) {
                size_t hi = paramSpine.size() - 1;
                while (lo + 1 < hi) {
                    const size_t mid = (lo + hi) / 2;
                    if (cum[mid] <= arc)
                        lo = mid;
                    else
                        hi = mid;
                }
                const double segLen = cum[lo + 1] - cum[lo];
                u = segLen > 1e-12 ? (arc - cum[lo]) / segLen : 0.0;
            }
        }
        const Vec2 p = geomSpine.size() >= 2
            ? geomSpine[lo] + (geomSpine[lo + 1] - geomSpine[lo]) * u
            : (geomSpine.empty() ? Vec2{} : geomSpine[0]);
        const Vec2 t = geomSpine.size() >= 2 ? segTangentAt(geomSpine, lo) : Vec2{1.0, 0.0};
        out[i] = p + leftNormal(t) * (sd[i].d * dFactor);
    }
    return out;
}

enum Variant { kA = 0, kB = 1, kAC = 2, kBC = 3 };
const char *variantName(int v)
{
    static const char *names[kVariantCount] = {"A", "B", "A+C", "B+C"};
    return names[v];
}
bool variantBlends(int v) { return v == kAC || v == kBC; }

struct WarpResult {
    Poly spine;
    Poly spineUnblended;
    Poly samples;
    Poly samplesUnblended;
    double rawScale = 1.0;
    double usedScale = 1.0;
    double dFactor = 1.0;
    double lPrime = 0.0;
    double stubLen = 0.0;
    double tEff = 0.0;
    double endpointErr = 0.0;  // |spine.back() - p1|, non-zero only when a clamp detaches it
};

WarpResult warpConnector(const RestShape &rs, Vec2 p0, Vec2 f0, Vec2 p1, Vec2 f1, int v,
                         const WarpParams &pm, bool wantUnblended = true)
{
    WarpResult out;
    out.rawScale = rs.chordLen > 1e-9 ? vlen(p1 - p0) / rs.chordLen : 1.0;
    out.usedScale = pm.scaleFloor ? std::max(out.rawScale, pm.scaleFloorValue) : out.rawScale;
    out.dFactor = pm.dTaper ? std::min(1.0, out.usedScale) : 1.0;

    out.spineUnblended = (v == kB || v == kBC)
        ? warpSpineAniso(rs, p0, p1, out.usedScale,
                         pm.anisoTaper ? std::min(1.0, out.usedScale) : 1.0)
        : warpSpineSimilarity(rs, p0, p1, out.usedScale);
    out.lPrime = arcTable(out.spineUnblended).back();
    out.tEff = pm.T;

    out.spine = out.spineUnblended;
    if (variantBlends(v)) {
        if (pm.blendAbsolute && out.lPrime > 1e-9)
            out.tEff = std::max(0.05, std::min(0.35, pm.blendWorld / out.lPrime));
        const double span = out.tEff * out.lPrime;
        out.stubLen = pm.stubAbsolute ? std::min(pm.stubWorld, pm.stubCapFactor * span)
                                      : pm.stubK * span;
        const double inner = pm.stubAppliesToInner ? out.stubLen : span;
        out.spine = tangentBlend(out.spine, p0, f0, p1, f1, out.tEff, out.stubLen, inner);
    }
    const Poly &param = pm.localParam ? out.spineUnblended : out.spine;
    out.samples = placeSamples(param, out.spine, rs.sd, out.dFactor);
    if (wantUnblended)
        out.samplesUnblended =
            placeSamples(out.spineUnblended, out.spineUnblended, rs.sd, out.dFactor);
    out.endpointErr = vlen(out.spine.back() - p1);
    return out;
}

// ---------------------------------------------------------------- boxes and anchors

struct Aabb {
    double minX = 0.0, minY = 0.0, maxX = 0.0, maxY = 0.0;
    bool contains(Vec2 p) const
    {
        return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
    }
};

struct Box {
    Vec2 c{};
    double hw = 0.0;
    double hh = 0.0;
    double rot = 0.0;

    std::array<Vec2, 4> corners() const
    {
        const Vec2 local[4] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
        std::array<Vec2, 4> out{};
        for (int i = 0; i < 4; ++i)
            out[static_cast<size_t>(i)] = c + rot2(local[i], rot);
        return out;
    }

    Aabb aabb() const
    {
        const auto cs = corners();
        Aabb b{cs[0].x, cs[0].y, cs[0].x, cs[0].y};
        for (const Vec2 &p : cs) {
            b.minX = std::min(b.minX, p.x);
            b.minY = std::min(b.minY, p.y);
            b.maxX = std::max(b.maxX, p.x);
            b.maxY = std::max(b.maxY, p.y);
        }
        return b;
    }
};

enum class AnchorKind { Edge, Centre };

struct Anchor {
    AnchorKind kind = AnchorKind::Edge;
    int edge = 0;   // 0 top, 1 right, 2 bottom, 3 left (y-down world)
    double t = 0.5; // position along that edge, preserved across move/resize/rotate
};

Vec2 anchorPoint(const Box &b, const Anchor &a)
{
    if (a.kind == AnchorKind::Centre)
        return b.c;
    const auto cs = b.corners();
    const Vec2 p = cs[static_cast<size_t>(a.edge)];
    const Vec2 q = cs[static_cast<size_t>((a.edge + 1) % 4)];
    return p + (q - p) * a.t;
}

// D26: an edge anchor's facing is permanently that edge's outward normal and is never
// re-selected; a centre anchor derives its facing as the ray toward the peer.
Vec2 anchorFacing(const Box &b, const Anchor &a, Vec2 towardOther)
{
    if (a.kind == AnchorKind::Centre)
        return unit(towardOther - b.c);
    static const Vec2 nLocal[4] = {{0.0, -1.0}, {1.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0}};
    return unit(rot2(nLocal[a.edge], b.rot));
}

Vec2 aabbExit(Vec2 inside, Vec2 outside, const Aabb &b)
{
    Vec2 lo = inside;
    Vec2 hi = outside;
    for (int i = 0; i < 48; ++i) {
        const Vec2 mid = (lo + hi) * 0.5;
        if (b.contains(mid))
            lo = mid;
        else
            hi = mid;
    }
    return hi;
}

size_t leadingInsideCount(const Poly &pts, const Aabb &b)
{
    size_t n = 0;
    while (n < pts.size() && b.contains(pts[n]))
        ++n;
    return n;
}

size_t trailingInsideCount(const Poly &pts, const Aabb &b)
{
    size_t n = 0;
    while (n < pts.size() && b.contains(pts[pts.size() - 1 - n]))
        ++n;
    return n;
}

// D17: a centre-bound end clips the ink at the box boundary.
void clipEnds(Poly &pts, const Aabb *startBox, const Aabb *endBox)
{
    if (startBox) {
        const size_t first = leadingInsideCount(pts, *startBox);
        if (first > 0 && first < pts.size()) {
            const Vec2 hit = aabbExit(pts[first - 1], pts[first], *startBox);
            Poly out(pts.begin() + static_cast<std::ptrdiff_t>(first), pts.end());
            out.front() = hit;
            pts.swap(out);
        }
    }
    if (endBox) {
        const size_t last = trailingInsideCount(pts, *endBox);
        if (last > 0 && last < pts.size()) {
            const size_t keep = pts.size() - last;
            const Vec2 hit = aabbExit(pts[keep], pts[keep - 1], *endBox);
            pts.resize(keep);
            pts.back() = hit;
        }
    }
}

// ---------------------------------------------------------------- stroke synthesis

struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed)
        : s(seed * 6364136223846793005ULL + 1442695040888963407ULL)
    {
    }
    uint32_t next()
    {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<uint32_t>(s >> 33);
    }
    double uniform() { return static_cast<double>(next()) / 2147483648.0 - 1.0; }
};

std::vector<double> noiseProfile(int n, double amp, uint64_t seed)
{
    std::vector<double> v(static_cast<size_t>(n), 0.0);
    if (n < 2 || amp == 0.0)
        return v;
    Rng rng(seed);
    double vel = 0.0;
    double pos = 0.0;
    double m = 0.0;
    for (int i = 0; i < n; ++i) {
        vel = vel * 0.86 + rng.uniform() * 0.30;
        pos = pos * 0.985 + vel;
        v[static_cast<size_t>(i)] = pos;
        m = std::max(m, std::abs(pos));
    }
    if (m < 1e-9)
        m = 1.0;
    for (int i = 0; i < n; ++i) {
        const double u = static_cast<double>(i) / static_cast<double>(n - 1);
        v[static_cast<size_t>(i)] *= amp * std::sin(kPi * u) / m;
    }
    return v;
}

std::vector<double> speedProfile(int n, uint64_t seed)
{
    std::vector<double> ts(static_cast<size_t>(n), 0.0);
    if (n < 2)
        return ts;
    Rng rng(seed ^ 0x9E3779B97F4A7C15ULL);
    std::vector<double> w(static_cast<size_t>(n - 1));
    double acc = 0.0;
    for (int i = 0; i < n - 1; ++i) {
        double x = 1.0 + 0.30 * std::sin(2.0 * kPi * 1.7 * static_cast<double>(i)
                                         / static_cast<double>(n - 1))
            + 0.12 * rng.uniform();
        if (x < 0.2)
            x = 0.2;
        w[static_cast<size_t>(i)] = x;
        acc += x;
    }
    double c = 0.0;
    for (int i = 1; i < n; ++i) {
        c += w[static_cast<size_t>(i - 1)];
        ts[static_cast<size_t>(i)] = c / acc;
    }
    ts[static_cast<size_t>(n - 1)] = 1.0;
    return ts;
}

Poly strokeBetween(Vec2 p0, Vec2 p1, double bulge, double waveAmp, double waves,
                   double noiseAmp, uint64_t seed, int n)
{
    const Vec2 d = p1 - p0;
    const Vec2 nn = leftNormal(unit(d));
    const std::vector<double> noise = noiseProfile(n, noiseAmp, seed);
    const std::vector<double> ts = speedProfile(n, seed);
    Poly out;
    out.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const double t = ts[static_cast<size_t>(i)];
        double off = bulge * std::sin(kPi * t);
        if (waveAmp != 0.0)
            off += waveAmp * std::sin(2.0 * kPi * waves * t) * std::sin(kPi * t);
        off += noise[static_cast<size_t>(i)];
        out.push_back(p0 + d * t + nn * off);
    }
    out.front() = p0;
    out.back() = p1;
    return out;
}

Vec2 catmullRom(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    return (p1 * 2.0 + (p2 - p0) * t + (p0 * 2.0 - p1 * 5.0 + p2 * 4.0 - p3) * t2
            + (p1 * 3.0 - p0 - p2 * 3.0 + p3) * t3)
        * 0.5;
}

Poly strokeThrough(const std::vector<Vec2> &ctrl, double waveAmp, double waves,
                   double noiseAmp, uint64_t seed, int n)
{
    std::vector<Vec2> ext;
    ext.push_back(ctrl.front());
    ext.insert(ext.end(), ctrl.begin(), ctrl.end());
    ext.push_back(ctrl.back());
    const int spans = static_cast<int>(ctrl.size()) - 1;
    const std::vector<double> ts = speedProfile(n, seed);
    const std::vector<double> noise = noiseProfile(n, noiseAmp, seed);
    Poly base;
    base.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        double g = ts[static_cast<size_t>(i)] * static_cast<double>(spans);
        int seg = static_cast<int>(g);
        if (seg >= spans)
            seg = spans - 1;
        const double lt = g - static_cast<double>(seg);
        base.push_back(catmullRom(ext[static_cast<size_t>(seg)], ext[static_cast<size_t>(seg + 1)],
                                  ext[static_cast<size_t>(seg + 2)],
                                  ext[static_cast<size_t>(seg + 3)], lt));
    }
    Poly out = base;
    for (int i = 0; i < n; ++i) {
        const size_t j = static_cast<size_t>(i);
        const size_t k = (i + 1 < n) ? j + 1 : j;
        const size_t h = (i > 0) ? j - 1 : j;
        const Vec2 tang = unit(base[k] - base[h]);
        double off = noise[j];
        if (waveAmp != 0.0)
            off += waveAmp * std::sin(2.0 * kPi * waves * ts[j]) * std::sin(kPi * ts[j]);
        out[j] = base[j] + leftNormal(tang) * off;
    }
    out.front() = ctrl.front();
    out.back() = ctrl.back();
    return out;
}

// ---------------------------------------------------------------- cases

struct Case {
    std::string name;
    std::string flavour;   // "arc" | "wiggle"
    std::string scenario;
    std::string note;
    std::vector<Poly> strokes;
    Box a0{}, b0{}, a1{}, b1{};
    Anchor anchorA{}, anchorB{};
    std::vector<Box> obstacles;
};

const Box kA0{{240.0, 420.0}, 90.0, 62.0, 0.0};
const Box kB0{{760.0, 380.0}, 100.0, 70.0, 0.0};
const Anchor kAnchorA{AnchorKind::Edge, 1, 0.45};
const Anchor kAnchorB{AnchorKind::Edge, 3, 0.50};

Poly flavourStroke(const std::string &flavour, Vec2 p0, Vec2 p1, uint64_t seed)
{
    if (flavour == "arc")
        return strokeBetween(p0, p1, -68.0, 0.0, 0.0, 1.4, seed, 280);
    return strokeBetween(p0, p1, -30.0, 26.0, 2.7, 7.5, seed, 320);
}

const char *kScenarios[10] = {"translate-near", "translate-far",    "chord-flip",
                              "rotate-a",       "resize-a",         "both-move",
                              "degenerate",     "detour-third-box", "chain-3-stroke",
                              "centre-clip"};

Case makeCase(const std::string &flavour, const std::string &scenario)
{
    Case c;
    c.flavour = flavour;
    c.scenario = scenario;
    c.name = flavour + "/" + scenario;
    c.a0 = kA0;
    c.b0 = kB0;
    c.a1 = kA0;
    c.b1 = kB0;
    c.anchorA = kAnchorA;
    c.anchorB = kAnchorB;
    const uint64_t seed = (flavour == "arc") ? 0xC0FFEEULL : 0xBADCAFEULL;

    if (scenario == "centre-clip")
        c.anchorA = Anchor{AnchorKind::Centre, 0, 0.0};

    const Vec2 p0 = anchorPoint(c.a0, c.anchorA);
    const Vec2 p1 = anchorPoint(c.b0, c.anchorB);

    if (scenario == "detour-third-box") {
        c.obstacles.push_back(Box{{500.0, 415.0}, 70.0, 55.0, 0.0});
        const std::vector<Vec2> ctrl = {p0, {452.0, 322.0}, {566.0, 306.0}, p1};
        const double wave = (flavour == "arc") ? 0.0 : 16.0;
        const double noise = (flavour == "arc") ? 1.4 : 6.0;
        c.strokes.push_back(strokeThrough(ctrl, wave, 3.1, noise, seed, 340));
        c.a1 = c.a0;
        c.a1.c = c.a0.c + Vec2{-45.0, 255.0};
        c.note = "rest ink detours over a third box; the warp drags the detour with it (D12, out of scope)";
    } else if (scenario == "chain-3-stroke") {
        const Vec2 w1{455.0, 318.0};
        const Vec2 w2{575.0, 452.0};
        const Vec2 gap{2.2, 1.4};
        const double bulgeA = (flavour == "arc") ? -22.0 : -16.0;
        const double wave = (flavour == "arc") ? 0.0 : 14.0;
        const double noise = (flavour == "arc") ? 1.2 : 5.5;
        c.strokes.push_back(strokeBetween(p0, w1, bulgeA, wave, 1.6, noise, seed, 120));
        c.strokes.push_back(strokeBetween(w1 + gap, w2, -bulgeA, wave, 1.6, noise, seed + 11, 120));
        c.strokes.push_back(strokeBetween(w2 + gap, p1, bulgeA, wave, 1.6, noise, seed + 23, 120));
        c.a1 = c.a0;
        c.a1.c = c.a0.c + Vec2{-195.0, 285.0};
        c.note = "3 strokes joined end-to-end with ~2u gaps; one rest shape from all three";
    } else if (scenario == "centre-clip") {
        const double bulge = (flavour == "arc") ? -58.0 : -26.0;
        const double wave = (flavour == "arc") ? 0.0 : 22.0;
        const double noise = (flavour == "arc") ? 1.4 : 7.0;
        c.strokes.push_back(strokeBetween(p0, p1, bulge, wave, 2.5, noise, seed, 340));
        c.a1 = c.a0;
        c.a1.c = c.a0.c + Vec2{-105.0, 215.0};
        c.note = "centre-bound start (D17): facing is the ray toward the peer, clipped at A's AABB";
    } else {
        c.strokes.push_back(flavourStroke(flavour, p0, p1, seed));
        if (scenario == "translate-near") {
            c.a1.c = c.a0.c + Vec2{36.0, -28.0};
            c.note = "small move; the line should barely change character";
        } else if (scenario == "translate-far") {
            c.a1.c = c.a0.c + Vec2{-215.0, 300.0};
            c.note = "large move; chord grows and rotates";
        } else if (scenario == "chord-flip") {
            c.a1.c = Vec2{1090.0, 300.0};
            c.note = "A moves past B; both stored edge facings now oppose the chord";
        } else if (scenario == "rotate-a") {
            c.a1.rot = 52.0 * kPi / 180.0;
            c.note = "A rotates 52 deg; the anchor and its edge normal rotate with it";
        } else if (scenario == "resize-a") {
            c.a1.hw = 142.0;
            c.a1.hh = 38.0;
            c.note = "A resized wide and flat; anchor t=0.45 preserved on the right edge";
        } else if (scenario == "both-move") {
            c.a1.c = c.a0.c + Vec2{-130.0, 195.0};
            c.b1.c = c.b0.c + Vec2{155.0, -165.0};
            c.note = "both boxes move";
        } else if (scenario == "degenerate") {
            c.a1.c = Vec2{567.0, 386.2};
            c.note = "endpoints ~3 world units apart; raw similarity scale is ~0.009";
        }
    }
    return c;
}

std::vector<Case> buildCases()
{
    std::vector<Case> out;
    for (const std::string &flavour : {std::string("arc"), std::string("wiggle")}) {
        for (const char *s : kScenarios)
            out.push_back(makeCase(flavour, s));
    }
    return out;
}

// ---------------------------------------------------------------- ends and alignment

struct Ends {
    Vec2 p0{}, f0{}, p1{}, f1{};
    bool centre0 = false, centre1 = false;
    bool aligned0 = true, aligned1 = true;
    // How far the stored facing is from the direction toward the peer. This is what the
    // blend has to absorb; >= 90 deg is the reversed class.
    double obliq0 = 0.0, obliq1 = 0.0;
    Aabb box0{}, box1{};
    bool reversed() const { return !aligned0 || !aligned1; }
    double obliquity() const { return std::max(obliq0, obliq1); }
};

Ends resolveEnds(const Case &c, bool forceCentre0, bool forceCentre1)
{
    Anchor a0 = c.anchorA;
    Anchor a1 = c.anchorB;
    if (forceCentre0)
        a0 = Anchor{AnchorKind::Centre, 0, 0.0};
    if (forceCentre1)
        a1 = Anchor{AnchorKind::Centre, 0, 0.0};
    Ends e;
    e.centre0 = (a0.kind == AnchorKind::Centre);
    e.centre1 = (a1.kind == AnchorKind::Centre);
    e.p0 = anchorPoint(c.a1, a0);
    e.p1 = anchorPoint(c.b1, a1);
    e.f0 = anchorFacing(c.a1, a0, e.p1);
    e.f1 = anchorFacing(c.b1, a1, e.p0);
    e.obliq0 = angleBetweenDeg(e.f0, e.p1 - e.p0);
    e.obliq1 = angleBetweenDeg(e.f1, e.p0 - e.p1);
    e.aligned0 = e.centre0 || e.obliq0 < 90.0;
    e.aligned1 = e.centre1 || e.obliq1 < 90.0;
    e.box0 = c.a1.aabb();
    e.box1 = c.b1.aabb();
    return e;
}

// ---------------------------------------------------------------- round-2 metrics

// W5. Blend region against an absolute minimum-radius bar; middle against the round-1
// rest-relative turn bar. `sLo`/`sHi` restrict scoring to what survives boundary clipping.
struct CuspScore {
    int blend = 0;
    int middle = 0;
    int old = 0;       // round-1 reading: rest-relative bar everywhere
    int blendFull = 0; // same bar, ignoring the boundary-clip window
    double minRadiusBlend = 1e9;
    double bar = 0.0;  // effective minimum-radius bar for this connector
    int blendTight = 0;  // same count at the 8 u bar
    int blendLoose = 0;  // same count at the 16 u bar
    int total() const { return blend + middle; }
};

double blendRadiusBar(double T, double lPrime, double base)
{
    return std::min(base, kBlendRadiusSpanFactor * T * lPrime);
}

CuspScore scoreCusps(const Poly &sp, double T, double lPrime, double restMaxTurnDeg, double sLo,
                     double sHi)
{
    CuspScore cs;
    cs.bar = blendRadiusBar(T, lPrime, kBlendMinRadiusWorld);
    if (sp.size() < 3)
        return cs;
    const double barTight = blendRadiusBar(T, lPrime, kBlendMinRadiusTightWorld);
    const double barLoose = blendRadiusBar(T, lPrime, kBlendMinRadiusLooseWorld);
    const std::vector<double> cum = arcTable(sp);
    const double total = cum.back() > 1e-12 ? cum.back() : 1.0;
    const double oldThr = 1.5 * restMaxTurnDeg + 5.0;
    for (size_t i = 1; i + 1 < sp.size(); ++i) {
        const Vec2 a = sp[i] - sp[i - 1];
        const Vec2 b = sp[i + 1] - sp[i];
        const double la = vlen(a);
        const double lb = vlen(b);
        if (la < 1e-10 || lb < 1e-10)
            continue;
        const double s = cum[i] / total;
        const bool inBlend = (s < T || s > 1.0 - T);
        const double turn = angleBetweenDeg(a, b);
        const double meanSeg = 0.5 * (la + lb);
        const double radius = turn > 1e-9 ? meanSeg * 180.0 / (kPi * turn) : 1e9;
        if (inBlend && radius < cs.bar)
            ++cs.blendFull;
        if (s < sLo || s > sHi)
            continue;
        if (turn > oldThr)
            ++cs.old;
        if (inBlend) {
            cs.minRadiusBlend = std::min(cs.minRadiusBlend, radius);
            if (radius < cs.bar)
                ++cs.blend;
            if (radius < barTight)
                ++cs.blendTight;
            if (radius < barLoose)
                ++cs.blendLoose;
        } else if (turn > oldThr) {
            ++cs.middle;
        }
    }
    return cs;
}

// Failure mode of an over-long departure stub: the curve overshoots its target and comes
// back, or crosses the body. Only meaningful on aligned ends — a reversed edge bind must
// backtrack, that is the U-turn.
struct BlendHealth {
    int backtracks = 0;
    int selfIntersect = 0;
    int total() const { return backtracks + selfIntersect; }
};

BlendHealth blendHealth(const Poly &sp, double T)
{
    BlendHealth h;
    if (sp.size() < 6)
        return h;
    const std::vector<double> cum = arcTable(sp);
    const double total = cum.back();
    if (total < 1e-9)
        return h;
    size_t jT = 0;
    size_t jE = sp.size() - 1;
    for (size_t i = 0; i < sp.size(); ++i) {
        if (cum[i] / total < T)
            jT = i;
    }
    for (size_t i = sp.size(); i-- > 0;) {
        if (cum[i] / total > 1.0 - T)
            jE = i;
    }
    if (jT > 1) {
        const Vec2 dir = unit(sp[jT] - sp[0]);
        double prev = 0.0;
        for (size_t i = 1; i <= jT; ++i) {
            const double u = dot(sp[i] - sp[0], dir);
            if (u < prev - 1e-9)
                ++h.backtracks;
            prev = u;
        }
    }
    if (jE + 2 < sp.size()) {
        const Vec2 dir = unit(sp.back() - sp[jE]);
        double prev = 0.0;
        for (size_t i = jE + 1; i < sp.size(); ++i) {
            const double u = dot(sp[i] - sp[jE], dir);
            if (u < prev - 1e-9)
                ++h.backtracks;
            prev = u;
        }
    }
    for (size_t i = 0; i + 1 < sp.size(); ++i) {
        const bool inBlend = (i <= jT) || (i >= jE);
        if (!inBlend)
            continue;
        for (size_t j = i + 2; j + 1 < sp.size(); ++j) {
            if (properCross(sp[i], sp[i + 1], sp[j], sp[j + 1]))
                ++h.selfIntersect;
        }
    }
    return h;
}

// What the blend costs in drawn character. Compared against the same variant without C.
struct Fidelity {
    double midMaxSample = 0.0;  // max deviation over the fixed middle 70% of arc length
    double midMaxSpine = 0.0;   // self-check: exactly 0 while T <= 0.15
    double blendMeanSample = 0.0;
};

Fidelity fidelity(const RestShape &rs, const Poly &blended, const Poly &plain,
                  const Poly &blendedSpine, const Poly &plainSpine, double T)
{
    Fidelity f;
    if (blended.size() == plain.size()) {
        double acc = 0.0;
        int n = 0;
        for (size_t i = 0; i < blended.size(); ++i) {
            const double s = rs.sd[i].s;
            const double dev = vlen(blended[i] - plain[i]);
            if (s >= 0.15 && s <= 0.85)
                f.midMaxSample = std::max(f.midMaxSample, dev);
            if (s < T || s > 1.0 - T) {
                acc += dev;
                ++n;
            }
        }
        f.blendMeanSample = n ? acc / static_cast<double>(n) : 0.0;
    }
    if (blendedSpine.size() == plainSpine.size() && !plainSpine.empty()) {
        const std::vector<double> cum = arcTable(plainSpine);
        const double total = cum.back() > 1e-12 ? cum.back() : 1.0;
        for (size_t i = 0; i < blendedSpine.size(); ++i) {
            const double s = cum[i] / total;
            if (s >= 0.15 && s <= 0.85)
                f.midMaxSpine = std::max(f.midMaxSpine, vlen(blendedSpine[i] - plainSpine[i]));
        }
    }
    return f;
}

struct VariantEval {
    Poly spine;
    Poly samples;           // drawn geometry, clipped for centre-bound ends
    Poly samplesUnclipped;
    double facingStartDeg = 0.0;
    double facingEndDeg = 0.0;
    CuspScore cusps;
    BlendHealth health;
    Fidelity fid;
    double facingVisibleStartDeg = 0.0;  // for a clipped end: direction where the ink appears
    double visibleShare = 1.0;           // fraction of drawn samples surviving the clip
    double stubLen = 0.0;
    double tEff = 0.0;
    double endpointErr = 0.0;
    double worstFacingDeg() const { return std::max(facingStartDeg, facingEndDeg); }
};

struct CaseEval {
    std::string name;
    std::string note;
    RestShape rest;
    Ends ends;
    std::array<VariantEval, kVariantCount> v{};
    Poly restDisplay;
    double rawScale = 1.0;
    double lPrime = 0.0;
    double gapAB = 0.0;
};

Vec2 endTangentStart(const Poly &p)
{
    return p.size() < 2 ? Vec2{1.0, 0.0} : segTangentAt(p, 0);
}
Vec2 endTangentEnd(const Poly &p)
{
    return p.size() < 2 ? Vec2{1.0, 0.0} : segTangentAt(p, p.size() - 2);
}

CaseEval evaluateCase(const Case &c, const WarpParams &pm, bool forceCentre0 = false,
                      bool forceCentre1 = false)
{
    CaseEval e;
    e.name = c.name;
    e.note = c.note;
    e.rest = buildRestShape(c.strokes);
    e.ends = resolveEnds(c, forceCentre0, forceCentre1);

    // Rest display is clipped the same way the warped output is, so the comparison is fair.
    e.restDisplay = e.rest.raw;
    {
        Anchor a0 = c.anchorA;
        if (forceCentre0)
            a0 = Anchor{AnchorKind::Centre, 0, 0.0};
        Anchor a1 = c.anchorB;
        if (forceCentre1)
            a1 = Anchor{AnchorKind::Centre, 0, 0.0};
        const Aabb ra = c.a0.aabb();
        const Aabb rb = c.b0.aabb();
        clipEnds(e.restDisplay, a0.kind == AnchorKind::Centre ? &ra : nullptr,
                 a1.kind == AnchorKind::Centre ? &rb : nullptr);
    }

    for (int v = 0; v < kVariantCount; ++v) {
        const WarpResult w = warpConnector(e.rest, e.ends.p0, e.ends.f0, e.ends.p1, e.ends.f1,
                                           v, pm);
        VariantEval ve;
        ve.spine = w.spine;
        ve.samplesUnclipped = w.samples;
        ve.samples = w.samples;
        ve.stubLen = w.stubLen;
        ve.tEff = w.tEff;
        ve.endpointErr = w.endpointErr;
        e.rawScale = w.rawScale;
        e.lPrime = w.lPrime;

        double sLo = 0.0;
        double sHi = 1.0;
        if (e.ends.centre0) {
            const size_t first = leadingInsideCount(ve.samples, e.ends.box0);
            if (first > 0 && first < e.rest.sd.size())
                sLo = e.rest.sd[first].s;
        }
        if (e.ends.centre1) {
            const size_t last = trailingInsideCount(ve.samples, e.ends.box1);
            if (last > 0 && last < e.rest.sd.size())
                sHi = e.rest.sd[e.rest.sd.size() - 1 - last].s;
        }
        clipEnds(ve.samples, e.ends.centre0 ? &e.ends.box0 : nullptr,
                 e.ends.centre1 ? &e.ends.box1 : nullptr);
        ve.visibleShare = ve.samplesUnclipped.empty()
            ? 1.0
            : static_cast<double>(ve.samples.size())
                / static_cast<double>(ve.samplesUnclipped.size());

        ve.facingStartDeg = angleBetweenDeg(endTangentStart(ve.spine), e.ends.f0);
        ve.facingEndDeg = angleBetweenDeg(endTangentEnd(ve.spine) * -1.0, e.ends.f1);
        ve.facingVisibleStartDeg = ve.facingStartDeg;
        if (sLo > 0.0) {
            const std::vector<double> spCum = arcTable(ve.spine);
            ve.facingVisibleStartDeg =
                angleBetweenDeg(frameAtArc(ve.spine, spCum, sLo * spCum.back()).t, e.ends.f0);
        }
        ve.cusps = scoreCusps(ve.spine, w.tEff, w.lPrime, e.rest.restMaxTurnDeg, sLo, sHi);
        if (variantBlends(v)) {
            ve.health = blendHealth(ve.spine, w.tEff);
            ve.fid = fidelity(e.rest, w.samples, w.samplesUnblended, w.spine, w.spineUnblended,
                              w.tEff);
        }
        e.v[static_cast<size_t>(v)] = ve;
    }
    e.gapAB = maxDeviation(e.v[kA].samplesUnclipped, e.v[kB].samplesUnclipped);
    return e;
}

// ---------------------------------------------------------------- svg

struct SvgPoly {
    Poly pts;
    std::string stroke;
    double width = 1.0;
    std::string dash;
    double opacity = 1.0;
};

struct SvgDot {
    Vec2 c{};
    double r = 3.0;
    std::string fill;
};

struct SvgArrow {
    Vec2 from{};
    Vec2 dir{};
    std::string stroke;
};

struct Scene {
    std::vector<SvgPoly> polys;
    std::vector<SvgDot> dots;
    std::vector<SvgArrow> arrows;
    double minX = 1e300, minY = 1e300, maxX = -1e300, maxY = -1e300;

    void grow(Vec2 p)
    {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }
    void addPoly(const Poly &pts, const std::string &stroke, double w, const std::string &dash,
                 double opacity)
    {
        if (pts.size() < 2)
            return;
        for (const Vec2 &p : pts)
            grow(p);
        polys.push_back(SvgPoly{pts, stroke, w, dash, opacity});
    }
    void addBox(const Box &b, const std::string &stroke, double w, const std::string &dash,
                double opacity)
    {
        const auto cs = b.corners();
        Poly p(cs.begin(), cs.end());
        p.push_back(cs[0]);
        addPoly(p, stroke, w, dash, opacity);
    }
    void addDot(Vec2 c, double r, const std::string &fill)
    {
        grow(c);
        dots.push_back(SvgDot{c, r, fill});
    }
    void addArrow(Vec2 from, Vec2 dir, const std::string &stroke)
    {
        grow(from);
        arrows.push_back(SvgArrow{from, unit(dir), stroke});
    }
};

std::string num(double v, int prec = 2)
{
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", prec, v);
    return buf;
}

std::string esc(const std::string &s)
{
    std::string out;
    for (char ch : s) {
        if (ch == '&')
            out += "&amp;";
        else if (ch == '<')
            out += "&lt;";
        else if (ch == '>')
            out += "&gt;";
        else
            out += ch;
    }
    return out;
}

std::string renderSvg(const Scene &sc, const std::string &title,
                      const std::vector<std::string> &labelLines)
{
    // Kept close to square: qlmanage renders SVG thumbnails into a square and crops the
    // overhang, so a wide canvas would lose its right-hand side in the optional PNGs.
    const double W = 1100.0;
    const double headerH = 34.0 + 17.0 * static_cast<double>(labelLines.size());
    const double plotH = 880.0;
    const double legendH = 34.0;
    const double H = headerH + plotH + legendH;
    const double pad = 26.0;
    const double padRight = 88.0;
    const double plotW = W - pad - padRight;
    const double bw = std::max(1.0, sc.maxX - sc.minX);
    const double bh = std::max(1.0, sc.maxY - sc.minY);
    const double k = std::min(plotW / bw, (plotH - 2 * pad) / bh);
    const double tx = pad + (plotW - bw * k) * 0.5 - sc.minX * k;
    const double ty = headerH + pad + ((plotH - 2 * pad) - bh * k) * 0.5 - sc.minY * k;
    auto sx = [&](Vec2 p) { return p.x * k + tx; };
    auto sy = [&](Vec2 p) { return p.y * k + ty; };

    std::ostringstream o;
    o << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << num(W, 0) << "\" height=\""
      << num(H, 0) << "\" viewBox=\"0 0 " << num(W, 0) << " " << num(H, 0) << "\">\n";
    o << "<rect x=\"0\" y=\"0\" width=\"" << num(W, 0) << "\" height=\"" << num(H, 0)
      << "\" fill=\"#ffffff\"/>\n";
    o << "<text x=\"18\" y=\"26\" font-family=\"-apple-system,Helvetica,Arial\" font-size=\"18\" "
         "font-weight=\"600\" fill=\"#111\">"
      << esc(title) << "</text>\n";
    double ly = 46.0;
    for (const std::string &line : labelLines) {
        o << "<text x=\"18\" y=\"" << num(ly, 0)
          << "\" font-family=\"ui-monospace,Menlo,monospace\" font-size=\"12\" fill=\"#444\">"
          << esc(line) << "</text>\n";
        ly += 17.0;
    }

    for (const SvgPoly &p : sc.polys) {
        o << "<polyline fill=\"none\" stroke=\"" << p.stroke << "\" stroke-width=\""
          << num(p.width) << "\" stroke-linejoin=\"round\" stroke-linecap=\"round\"";
        if (!p.dash.empty())
            o << " stroke-dasharray=\"" << p.dash << "\"";
        if (p.opacity < 1.0)
            o << " opacity=\"" << num(p.opacity) << "\"";
        o << " points=\"";
        for (size_t i = 0; i < p.pts.size(); ++i) {
            if (i)
                o << ' ';
            o << num(sx(p.pts[i]), 1) << ',' << num(sy(p.pts[i]), 1);
        }
        o << "\"/>\n";
    }
    for (const SvgArrow &a : sc.arrows) {
        const double L = 34.0;
        const Vec2 tip{sx(a.from) + a.dir.x * L, sy(a.from) + a.dir.y * L};
        const Vec2 base{sx(a.from), sy(a.from)};
        const Vec2 d{a.dir.x, a.dir.y};
        const Vec2 n = leftNormal(d);
        const Vec2 h1 = tip - d * 9.0 + n * 5.0;
        const Vec2 h2 = tip - d * 9.0 - n * 5.0;
        o << "<path d=\"M " << num(base.x, 1) << ' ' << num(base.y, 1) << " L " << num(tip.x, 1)
          << ' ' << num(tip.y, 1) << " M " << num(h1.x, 1) << ' ' << num(h1.y, 1) << " L "
          << num(tip.x, 1) << ' ' << num(tip.y, 1) << " L " << num(h2.x, 1) << ' '
          << num(h2.y, 1) << "\" fill=\"none\" stroke=\"" << a.stroke
          << "\" stroke-width=\"1.6\"/>\n";
    }
    for (const SvgDot &d : sc.dots) {
        o << "<circle cx=\"" << num(sx(d.c), 1) << "\" cy=\"" << num(sy(d.c), 1) << "\" r=\""
          << num(d.r) << "\" fill=\"" << d.fill << "\"/>\n";
    }

    struct LegendItem {
        const char *label;
        const char *color;
        double width;
        const char *dash;
    };
    static const LegendItem legend[] = {
        {"rest shape", "#bbbbbb", 3.0, ""},
        {"A similarity", "#1f77b4", 1.3, ""},
        {"B anisotropic", "#2ca02c", 1.3, "6,4"},
        {"A+C blend", "#d62728", 2.8, ""},
        {"B+C blend", "#7b3fbf", 2.8, "8,5"},
    };
    double lx = 18.0;
    const double lyy = H - 12.0;
    for (const LegendItem &li : legend) {
        o << "<line x1=\"" << num(lx, 0) << "\" y1=\"" << num(lyy - 4, 0) << "\" x2=\""
          << num(lx + 34, 0) << "\" y2=\"" << num(lyy - 4, 0) << "\" stroke=\"" << li.color
          << "\" stroke-width=\"" << num(li.width) << "\"";
        if (li.dash[0])
            o << " stroke-dasharray=\"" << li.dash << "\"";
        o << "/>\n";
        o << "<text x=\"" << num(lx + 40, 0) << "\" y=\"" << num(lyy, 0)
          << "\" font-family=\"-apple-system,Helvetica,Arial\" font-size=\"12\" fill=\"#333\">"
          << li.label << "</text>\n";
        lx += 40.0 + 8.0 * static_cast<double>(std::strlen(li.label)) + 26.0;
    }
    o << "</svg>\n";
    return o.str();
}

std::string endLabel(bool centre, bool aligned)
{
    if (centre)
        return "centre(derived)";
    return aligned ? "edge:aligned" : "edge:REVERSED";
}

std::string caseSvg(const Case &c, const CaseEval &e, const WarpParams &pm,
                    const std::string &titleSuffix, const Poly *ghost)
{
    Scene sc;
    for (const Box &ob : c.obstacles)
        sc.addBox(ob, "#999999", 1.2, "3,3", 0.9);
    sc.addBox(c.a0, "#c9c9c9", 1.4, "6,4", 1.0);
    sc.addBox(c.b0, "#c9c9c9", 1.4, "6,4", 1.0);
    sc.addBox(c.a1, "#111111", 1.8, "", 1.0);
    sc.addBox(c.b1, "#111111", 1.8, "", 1.0);

    if (e.ends.centre0 || e.ends.centre1) {
        sc.addPoly(e.restDisplay, "#bbbbbb", 3.0, "", 1.0);
    } else {
        size_t begin = 0;
        for (size_t end : e.rest.strokeEnds) {
            Poly seg(e.rest.raw.begin() + static_cast<std::ptrdiff_t>(begin),
                     e.rest.raw.begin() + static_cast<std::ptrdiff_t>(end));
            sc.addPoly(seg, "#bbbbbb", 3.0, "", 1.0);
            begin = end;
        }
    }
    if (ghost)
        sc.addPoly(*ghost, "#f0a6a6", 2.0, "2,4", 1.0);

    sc.addPoly(e.v[kAC].samples, "#d62728", 2.8, "", 0.95);
    sc.addPoly(e.v[kBC].samples, "#7b3fbf", 2.8, "8,5", 0.95);
    sc.addPoly(e.v[kA].samples, "#1f77b4", 1.3, "", 1.0);
    sc.addPoly(e.v[kB].samples, "#2ca02c", 1.3, "6,4", 1.0);

    sc.addArrow(e.ends.p0, e.ends.f0, "#e07b00");
    sc.addArrow(e.ends.p1, e.ends.f1, "#e07b00");
    sc.addDot(e.ends.p0, 4.2, "#111111");
    sc.addDot(e.ends.p1, 4.2, "#111111");

    std::vector<std::string> lines;
    lines.push_back(c.note);
    {
        std::ostringstream l;
        l << "ends: A " << endLabel(e.ends.centre0, e.ends.aligned0) << "   B "
          << endLabel(e.ends.centre1, e.ends.aligned1)
          << (e.ends.reversed() ? "   -- U-turn is EXPECTED here (D26); see the W7 section"
                                : "");
        lines.push_back(l.str());
    }
    {
        std::ostringstream l;
        l << "rest " << e.rest.raw.size() << " samples, len " << num(e.rest.spineLen, 1)
          << "u, chord " << num(e.rest.chordLen, 1) << "u   |   warped L' "
          << num(e.lPrime, 1) << "u, raw scale " << num(e.rawScale, 3) << ", stub "
          << num(e.v[kAC].stubLen, 1) << "u, T " << num(pm.T, 2);
        lines.push_back(l.str());
    }
    {
        std::ostringstream l;
        l << "W4 facing dev (start/end deg):";
        for (int v = 0; v < kVariantCount; ++v)
            l << "  " << variantName(v) << " " << num(e.v[static_cast<size_t>(v)].facingStartDeg, 1)
              << "/" << num(e.v[static_cast<size_t>(v)].facingEndDeg, 1);
        lines.push_back(l.str());
    }
    {
        std::ostringstream l;
        l << "W5 new cusps blend+middle (min blend radius u):";
        for (int v = 0; v < kVariantCount; ++v) {
            const CuspScore &cs = e.v[static_cast<size_t>(v)].cusps;
            l << "  " << variantName(v) << " " << cs.blend << "+" << cs.middle << " ("
              << num(std::min(cs.minRadiusBlend, 9999.0), 1) << ")";
        }
        lines.push_back(l.str());
    }
    {
        std::ostringstream l;
        l << "fidelity A+C: mid-70% " << num(e.v[kAC].fid.midMaxSample, 2)
          << "u, blend mean " << num(e.v[kAC].fid.blendMeanSample, 2) << "u   |   overshoot "
          << e.v[kAC].health.total() << "   |   A vs B " << num(e.gapAB, 1) << "u";
        if (e.v[kAC].visibleShare < 0.999)
            l << "   |   ink kept after clipping " << num(100.0 * e.v[kAC].visibleShare, 0)
              << "%";
        lines.push_back(l.str());
    }
    return renderSvg(sc, c.name + titleSuffix, lines);
}

std::string fileNameFor(const std::string &caseName)
{
    std::string s = caseName;
    for (char &ch : s) {
        if (ch == '/')
            ch = '_';
    }
    return s;
}

// ---------------------------------------------------------------- W2 / W3 / W6

bool bitIdentical(const Poly &a, const Poly &b)
{
    if (a.size() != b.size())
        return false;
    if (a.empty())
        return true;
    return std::memcmp(a.data(), b.data(), a.size() * sizeof(Vec2)) == 0;
}

std::vector<Box> roundTripPoses(const Box &base, int steps)
{
    std::vector<Box> out;
    out.reserve(static_cast<size_t>(steps) + 1);
    const double radius = 140.0;
    for (int i = 0; i <= steps; ++i) {
        const double th = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(steps);
        Box b = base;
        b.c = base.c + Vec2{radius * (std::cos(th) - 1.0), radius * std::sin(th)};
        b.rot = base.rot + 0.6 * std::sin(th);
        b.hw = base.hw * (1.0 + 0.35 * std::sin(2.0 * th));
        b.hh = base.hh * (1.0 + 0.25 * std::sin(th));
        if (i == steps)
            b = base;  // pose[20] == pose[0] exactly
        out.push_back(b);
    }
    return out;
}

struct RoundTrip {
    double pure = 0.0;
    double rebake1 = 0.0;
    double rebake10 = 0.0;
};

RoundTrip roundTripDrift(const RestShape &rest0, const Box &aBase, const Box &bBox,
                         const Anchor &anchorA, const Anchor &anchorB, int variant,
                         const WarpParams &pm)
{
    const std::vector<Box> poses = roundTripPoses(aBase, 20);
    auto warpAt = [&](const RestShape &rs, const Box &a) {
        const Vec2 pa = anchorPoint(a, anchorA);
        const Vec2 pb = anchorPoint(bBox, anchorB);
        return warpConnector(rs, pa, anchorFacing(a, anchorA, pb), pb,
                             anchorFacing(bBox, anchorB, pa), variant, pm, false);
    };
    const Poly reference = warpAt(rest0, poses.front()).samples;

    Poly pureFinal;
    for (int loop = 0; loop < 10; ++loop) {
        for (size_t i = 1; i < poses.size(); ++i)
            pureFinal = warpAt(rest0, poses[i]).samples;
    }

    RestShape cur = rest0;
    Poly rebakeFinal;
    RoundTrip rt;
    for (int loop = 0; loop < 10; ++loop) {
        for (size_t i = 1; i < poses.size(); ++i) {
            rebakeFinal = warpAt(cur, poses[i]).samples;
            // Deliberately wrong: rebuild the rest shape from the warped output (violates D5).
            cur = buildRestShape({rebakeFinal});
        }
        if (loop == 0)
            rt.rebake1 = maxDeviation(rebakeFinal, reference);
    }
    rt.pure = maxDeviation(pureFinal, reference);
    rt.rebake10 = maxDeviation(rebakeFinal, reference);
    return rt;
}

struct Timing {
    double p50 = 0.0;
    double p95 = 0.0;
};

Timing timeWarp(const RestShape &rs, Vec2 p0, Vec2 f0, Vec2 p1, Vec2 f1, int variant,
                const WarpParams &pm, int iters)
{
    std::vector<double> us;
    us.reserve(static_cast<size_t>(iters));
    double sink = 0.0;
    for (int i = 0; i < iters; ++i) {
        const Vec2 q1 = p1 + Vec2{1e-6 * static_cast<double>(i), 0.0};
        const auto t0 = std::chrono::steady_clock::now();
        const WarpResult w = warpConnector(rs, p0, f0, q1, f1, variant, pm, false);
        const auto t1 = std::chrono::steady_clock::now();
        sink += w.samples.back().x;
        us.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
    }
    std::sort(us.begin(), us.end());
    Timing t;
    t.p50 = us[static_cast<size_t>(iters) / 2];
    t.p95 = us[static_cast<size_t>(static_cast<double>(iters) * 0.95)];
    if (sink == 12345.6789)
        std::cerr << "";  // keep the optimizer honest
    return t;
}

std::string pad(const std::string &s, size_t w)
{
    std::string out = s;
    while (out.size() < w)
        out += ' ';
    return out;
}

// ---------------------------------------------------------------- sweep aggregation

struct Agg {
    int alignedCases = 0;
    int alignedZeroCusp = 0;
    int alignedBlendCuspTotal = 0;
    double maxFacingAll = 0.0;
    int facingOverAll = 0;
    int allCases = 0;
    int overshootCases = 0;
    int overshootTotal = 0;
    double midMax = 0.0;
    double blendMeanSum = 0.0;
    int blendMeanN = 0;
    double stubMin = 1e300;
    double stubMax = 0.0;
    double minRadius = 1e300;

    void add(const CaseEval &e, int variant)
    {
        const VariantEval &ve = e.v[static_cast<size_t>(variant)];
        ++allCases;
        maxFacingAll = std::max(maxFacingAll, ve.worstFacingDeg());
        if (ve.worstFacingDeg() > 5.0)
            ++facingOverAll;
        stubMin = std::min(stubMin, ve.stubLen);
        stubMax = std::max(stubMax, ve.stubLen);
        midMax = std::max(midMax, ve.fid.midMaxSample);
        blendMeanSum += ve.fid.blendMeanSample;
        ++blendMeanN;
        if (!e.ends.reversed()) {
            ++alignedCases;
            if (ve.cusps.total() == 0)
                ++alignedZeroCusp;
            alignedBlendCuspTotal += ve.cusps.blend;
            minRadius = std::min(minRadius, ve.cusps.minRadiusBlend);
            if (ve.health.total() > 0)
                ++overshootCases;
            overshootTotal += ve.health.total();
        }
    }
    double blendMean() const { return blendMeanN ? blendMeanSum / blendMeanN : 0.0; }
};

}  // namespace

int main(int argc, char **argv)
{
    const std::string outDir = argc > 1 ? std::string(argv[1]) : std::string("out");
    const std::vector<Case> cases = buildCases();
    const WarpParams def = defaultParams();

    std::cout << "\n=== EXP-0002 round 2 — connector-ink warp probe (SPIKE, host-only) ===\n";
    std::cout << "canonical from round 1: Hermite magnitude referenced to arc length (T*L'); "
                 "far-end departure = negated incoming tangent\n";
    std::cout << "recommended defaults in this build: T=" << num(def.T, 2) << "  departure stub="
              << (def.stubAbsolute
                      ? num(def.stubWorld, 1) + "u absolute (cap " + num(def.stubCapFactor, 2)
                          + "*T*L')"
                      : "k=" + num(def.stubK, 1) + " relative (k*T*L'), outer handle only")
              << "  sample parameterization="
              << (def.localParam ? "pre-blend arc" : "blended arc")
              << "  clamp: scale-floor=" << (def.scaleFloor ? "on" : "off")
              << " d-taper=" << (def.dTaper ? "on" : "off") << "\n";
    std::cout << "W5 blend bar: minimum radius of curvature " << num(kBlendMinRadiusWorld, 1)
              << " world units; middle keeps the round-1 rest-relative turn bar\n";

    // ---- 1. alignment partition (D26) ------------------------------------
    std::cout << "\n--- 1. anchor alignment partition (D26: edge facings are never "
                 "re-selected) ---\n";
    std::cout << "obliquity = angle between the stored facing and the direction toward the "
                 "peer; >= 90 deg is reversed\n";
    std::cout << pad("case", 26) << pad("end A", 18) << pad("obliq A", 10)
              << pad("end B", 18) << pad("obliq B", 10) << "class\n";
    std::vector<size_t> alignedIdx;
    std::vector<size_t> reversedIdx;
    for (size_t i = 0; i < cases.size(); ++i) {
        const Ends en = resolveEnds(cases[i], false, false);
        std::cout << pad(cases[i].name, 26) << pad(endLabel(en.centre0, en.aligned0), 18)
                  << pad(num(en.obliq0, 1), 10) << pad(endLabel(en.centre1, en.aligned1), 18)
                  << pad(num(en.obliq1, 1), 10)
                  << (en.reversed() ? "REVERSED" : "aligned") << "\n";
        if (en.reversed())
            reversedIdx.push_back(i);
        else
            alignedIdx.push_back(i);
    }
    std::cout << "aligned " << alignedIdx.size() << "/" << cases.size() << ", reversed "
              << reversedIdx.size() << "/" << cases.size()
              << " (reversed cases are excluded from the W5 bar and answered by W7)\n";

    // ---- 2. degenerate clamp policies ------------------------------------
    std::cout << "\n--- 2. degenerate clamp policies (at the default T and stub) ---\n";
    std::cout << "cost = max |A+C sample - unclamped A+C sample| in u; endErr = |A spine end - "
                 "anchor| in u (variant A has no blend to repair a detached end)\n";
    struct Policy {
        const char *name;
        bool floor;
        bool taper;
    };
    const Policy policies[4] = {{"none", false, false},
                                {"(i) scale-floor 0.15", true, false},
                                {"(ii) d-taper", false, true},
                                {"(iii) both", true, true}};
    std::cout << pad("case", 26) << pad("rawScale", 10);
    for (int p = 1; p < 4; ++p)
        std::cout << pad(std::string(policies[static_cast<size_t>(p)].name) + " cost/endErr", 30);
    std::cout << "\n";
    std::array<double, 4> worstEndErr{};
    std::array<double, 4> worstCostNormal{};  // cost on cases that do not need a clamp
    std::array<double, 4> worstSpike{};       // reach of the ink on the cases that need one
    for (size_t i = 0; i < cases.size(); ++i) {
        WarpParams p0 = def;
        p0.scaleFloor = false;
        p0.dTaper = false;
        const CaseEval base = evaluateCase(cases[i], p0);
        const bool needsClamp = base.rawScale < 0.15;
        worstEndErr[0] = std::max(worstEndErr[0], base.v[kA].endpointErr);
        if (needsClamp) {
            double spike = 0.0;
            for (const Vec2 &q : base.v[kAC].samplesUnclipped)
                spike = std::max(spike, vlen(q - base.ends.p0));
            worstSpike[0] = std::max(worstSpike[0], spike);
        }
        std::cout << pad(cases[i].name, 26) << pad(num(base.rawScale, 3), 10);
        for (int p = 1; p < 4; ++p) {
            WarpParams pp = def;
            pp.scaleFloor = policies[static_cast<size_t>(p)].floor;
            pp.dTaper = policies[static_cast<size_t>(p)].taper;
            const CaseEval ce = evaluateCase(cases[i], pp);
            const double cost = maxDeviation(ce.v[kAC].samplesUnclipped,
                                             base.v[kAC].samplesUnclipped);
            const double endErr = ce.v[kA].endpointErr;
            worstEndErr[static_cast<size_t>(p)] =
                std::max(worstEndErr[static_cast<size_t>(p)], endErr);
            if (!needsClamp)
                worstCostNormal[static_cast<size_t>(p)] =
                    std::max(worstCostNormal[static_cast<size_t>(p)], cost);
            if (needsClamp) {
                // Spike height = how far the drawn ink strays from the tiny new anchor pair.
                double spike = 0.0;
                for (const Vec2 &q : ce.v[kAC].samplesUnclipped)
                    spike = std::max(spike, vlen(q - ce.ends.p0));
                worstSpike[static_cast<size_t>(p)] =
                    std::max(worstSpike[static_cast<size_t>(p)], spike);
            }
            std::cout << pad(num(cost, 2) + " / " + num(endErr, 2), 30);
        }
        std::cout << "\n";
    }
    std::cout << "\npolicy summary (the degenerate cases span a ~3 u chord, so any reach well "
                 "above that is the spike)\n";
    std::cout << pad("policy", 24) << pad("worst degenerate reach", 24)
              << pad("worst endpoint detach (A)", 27) << "worst cost where no clamp is needed\n";
    for (int p = 0; p < 4; ++p)
        std::cout << pad(policies[static_cast<size_t>(p)].name, 24)
                  << pad(num(worstSpike[static_cast<size_t>(p)], 1) + " u", 24)
                  << pad(num(worstEndErr[static_cast<size_t>(p)], 2) + " u", 27)
                  << num(worstCostNormal[static_cast<size_t>(p)], 2) << " u\n";

    // Round 1 attributed the ~140 u spike to "the spine collapses while d stays absolute".
    // Split it per variant and against max |d| so the attribution is checkable.
    std::cout << "\nwhere the degenerate spike actually lives (reach = max |sample - anchor A|; "
                 "the new chord is ~3 u)\n";
    std::cout << pad("case / policy", 34) << pad("max |d| u", 11);
    for (int v = 0; v < kVariantCount; ++v)
        std::cout << pad(std::string(variantName(v)) + " reach", 12);
    std::cout << "\n";
    struct DegPolicy {
        const char *name;
        bool taper;
        bool aniso;
    };
    const DegPolicy degPolicies[3] = {
        {"unclamped", false, false}, {"(ii) d-taper", true, false},
        {"(iv) d-taper + aniso taper", true, true}};
    for (const std::string &flavour : {std::string("arc"), std::string("wiggle")}) {
        const Case dc = makeCase(flavour, "degenerate");
        double maxD = 0.0;
        for (const SD &s : buildRestShape(dc.strokes).sd)
            maxD = std::max(maxD, std::abs(s.d));
        for (const DegPolicy &dp : degPolicies) {
            WarpParams pm = def;
            pm.scaleFloor = false;
            pm.dTaper = dp.taper;
            pm.anisoTaper = dp.aniso;
            const CaseEval ce = evaluateCase(dc, pm);
            std::cout << pad(flavour + " / " + dp.name, 34) << pad(num(maxD, 1), 11);
            for (int v = 0; v < kVariantCount; ++v) {
                double reach = 0.0;
                for (const Vec2 &q : ce.v[static_cast<size_t>(v)].samplesUnclipped)
                    reach = std::max(reach, vlen(q - ce.ends.p0));
                std::cout << pad(num(reach, 1), 12);
            }
            std::cout << "\n";
        }
    }

    // ---- 3. blend length T sweep ------------------------------------------
    const double tSweep[4] = {0.10, 0.15, 0.25, 0.35};
    std::cout << "\n--- 3. blend length T sweep (stub = round-1 canonical T*L'; aligned cases "
                 "only for cusps/overshoot) ---\n";
    std::cout << pad("variant", 8) << pad("T", 7) << pad("0-cusp/aligned", 16)
              << pad("blend cusps", 13) << pad("min radius u", 14) << pad("max facing", 12)
              << pad("face>5", 8) << pad("overshoot", 11) << pad("mid70 max u", 13)
              << "blend mean u\n";
    for (int v : {kAC, kBC}) {
        for (double T : tSweep) {
            WarpParams pm = def;
            pm.T = T;
            pm.stubAbsolute = false;
            pm.stubK = 1.0;
            Agg agg;
            for (const Case &c : cases)
                agg.add(evaluateCase(c, pm), v);
            std::cout << pad(variantName(v), 8) << pad(num(T, 2), 7)
                      << pad(std::to_string(agg.alignedZeroCusp) + "/"
                                 + std::to_string(agg.alignedCases),
                             16)
                      << pad(std::to_string(agg.alignedBlendCuspTotal), 13)
                      << pad(num(agg.minRadius, 1), 14) << pad(num(agg.maxFacingAll, 2), 12)
                      << pad(std::to_string(agg.facingOverAll), 8)
                      << pad(std::to_string(agg.overshootTotal), 11)
                      << pad(num(agg.midMax, 2), 13) << num(agg.blendMean(), 2) << "\n";
        }
    }

    // ---- 4. departure stub sweep -----------------------------------------
    const double kSweep[4] = {0.5, 1.0, 1.5, 2.5};
    const double stubSweep[4] = {6.0, 12.0, 24.0, 48.0};
    std::cout << "\n--- 4a. departure stub, RELATIVE (outer handle = k*T*L') ---\n";
    std::cout << pad("variant", 8) << pad("T", 7) << pad("k", 7) << pad("0-cusp/aligned", 16)
              << pad("min radius u", 14) << pad("max facing", 12) << pad("overshoot", 11)
              << pad("stub range u", 20) << pad("mid70 max u", 13) << "blend mean u\n";
    for (int v : {kAC, kBC}) {
        for (double T : tSweep) {
            for (double k : kSweep) {
                WarpParams pm = def;
                pm.T = T;
                pm.stubAbsolute = false;
                pm.stubK = k;
                Agg agg;
                for (const Case &c : cases)
                    agg.add(evaluateCase(c, pm), v);
                std::cout << pad(variantName(v), 8) << pad(num(T, 2), 7) << pad(num(k, 1), 7)
                          << pad(std::to_string(agg.alignedZeroCusp) + "/"
                                     + std::to_string(agg.alignedCases),
                                 16)
                          << pad(num(agg.minRadius, 1), 14) << pad(num(agg.maxFacingAll, 2), 12)
                          << pad(std::to_string(agg.overshootTotal), 11)
                          << pad(num(agg.stubMin, 1) + " .. " + num(agg.stubMax, 1), 20)
                          << pad(num(agg.midMax, 2), 13) << num(agg.blendMean(), 2) << "\n";
            }
        }
    }

    std::cout << "\n--- 4b. departure stub, ABSOLUTE (outer handle = min(stub, "
              << num(kStubCapFactor, 2) << "*T*L') ) ---\n";
    std::cout << pad("variant", 8) << pad("T", 7) << pad("stub u", 8) << pad("0-cusp/aligned", 16)
              << pad("min radius u", 14) << pad("max facing", 12) << pad("overshoot", 11)
              << pad("stub range u", 20) << pad("mid70 max u", 13) << "blend mean u\n";
    for (int v : {kAC, kBC}) {
        for (double T : tSweep) {
            for (double s : stubSweep) {
                WarpParams pm = def;
                pm.T = T;
                pm.stubAbsolute = true;
                pm.stubWorld = s;
                Agg agg;
                for (const Case &c : cases)
                    agg.add(evaluateCase(c, pm), v);
                std::cout << pad(variantName(v), 8) << pad(num(T, 2), 7) << pad(num(s, 0), 8)
                          << pad(std::to_string(agg.alignedZeroCusp) + "/"
                                     + std::to_string(agg.alignedCases),
                                 16)
                          << pad(num(agg.minRadius, 1), 14) << pad(num(agg.maxFacingAll, 2), 12)
                          << pad(std::to_string(agg.overshootTotal), 11)
                          << pad(num(agg.stubMin, 1) + " .. " + num(agg.stubMax, 1), 20)
                          << pad(num(agg.midMax, 2), 13) << num(agg.blendMean(), 2) << "\n";
            }
        }
    }

    // 4b showed that a world-unit stub is starved when the blend span it has to fill is a
    // fraction of arc length: on a 650 u connector the span is ~100 u and a 12 u handle
    // hairpins. Making the span absolute too is the only way the product vocabulary
    // "leaves the box straight for N units" can hold across connector lengths.
    std::cout << "\n--- 4d. fully ABSOLUTE parameterization: blend span in world units too "
                 "(T derived, clamped to [0.05, 0.35]) ---\n";
    std::cout << pad("variant", 8) << pad("blend u", 9) << pad("stub u", 8)
              << pad("0-cusp/aligned", 16) << pad("min radius u", 14) << pad("max facing", 12)
              << pad("overshoot", 11) << pad("mid70 max u", 13) << "blend mean u\n";
    const double blendSweep[4] = {40.0, 60.0, 90.0, 130.0};
    for (int v : {kAC, kBC}) {
        for (double bw : blendSweep) {
            for (double s : stubSweep) {
                WarpParams pm = def;
                pm.blendAbsolute = true;
                pm.blendWorld = bw;
                pm.stubAbsolute = true;
                pm.stubWorld = s;
                Agg agg;
                for (const Case &c : cases)
                    agg.add(evaluateCase(c, pm), v);
                std::cout << pad(variantName(v), 8) << pad(num(bw, 0), 9) << pad(num(s, 0), 8)
                          << pad(std::to_string(agg.alignedZeroCusp) + "/"
                                     + std::to_string(agg.alignedCases),
                                 16)
                          << pad(num(agg.minRadius, 1), 14) << pad(num(agg.maxFacingAll, 2), 12)
                          << pad(std::to_string(agg.overshootTotal), 11)
                          << pad(num(agg.midMax, 2), 13) << num(agg.blendMean(), 2) << "\n";
            }
        }
    }

    std::cout << "\n--- 4c. does the stub belong on the inner handle too? (A+C, T=0.15) ---\n";
    std::cout << pad("config", 34) << pad("0-cusp/aligned", 16) << pad("min radius u", 14)
              << pad("max facing", 12) << pad("overshoot", 11) << "mid70 max u\n";
    for (int inner = 0; inner < 2; ++inner) {
        for (double s : stubSweep) {
            WarpParams pm = def;
            pm.T = 0.15;
            pm.stubAbsolute = true;
            pm.stubWorld = s;
            pm.stubAppliesToInner = (inner == 1);
            Agg agg;
            for (const Case &c : cases)
                agg.add(evaluateCase(c, pm), kAC);
            std::cout << pad(std::string(inner ? "outer+inner" : "outer only") + ", stub "
                                 + num(s, 0) + "u",
                             34)
                      << pad(std::to_string(agg.alignedZeroCusp) + "/"
                                 + std::to_string(agg.alignedCases),
                             16)
                      << pad(num(agg.minRadius, 1), 14) << pad(num(agg.maxFacingAll, 2), 12)
                      << pad(std::to_string(agg.overshootTotal), 11) << num(agg.midMax, 2)
                      << "\n";
        }
    }

    std::cout << "\n--- 4e. sample parameterization: does the blend leak into the middle? "
                 "(A+C, T=0.15, relative stub) ---\n";
    std::cout << pad("parameterization", 34) << pad("0-cusp/aligned", 16) << pad("max facing", 12)
              << pad("mid70 max u", 13) << "blend mean u\n";
    for (int local = 0; local < 2; ++local) {
        for (double k : kSweep) {
            WarpParams pm = def;
            pm.T = 0.15;
            pm.stubAbsolute = false;
            pm.stubK = k;
            pm.localParam = (local == 1);
            Agg agg;
            for (const Case &c : cases)
                agg.add(evaluateCase(c, pm), kAC);
            std::cout << pad(std::string(local ? "pre-blend arc (local)" : "blended arc (round 1)")
                                 + ", k=" + num(k, 1),
                             34)
                      << pad(std::to_string(agg.alignedZeroCusp) + "/"
                                 + std::to_string(agg.alignedCases),
                             16)
                      << pad(num(agg.maxFacingAll, 2), 12) << pad(num(agg.midMax, 2), 13)
                      << num(agg.blendMean(), 2) << "\n";
        }
    }

    // The coarse grid tops out at 17/18 aligned cases clean. Refine around the optimum to
    // see whether any (T, k) reaches the W5 bar, and name what is left over if not.
    std::cout << "\n--- 4f. fine grid around the optimum (A+C, relative stub) ---\n";
    std::cout << pad("T", 7) << pad("k", 7) << pad("0-cusp/aligned", 16) << pad("max facing", 12)
              << pad("overshoot", 11) << pad("blend mean u", 14) << "cases still failing\n";
    const double tFine[4] = {0.12, 0.15, 0.18, 0.22};
    const double kFine[4] = {1.2, 1.5, 1.8, 2.1};
    for (double T : tFine) {
        for (double k : kFine) {
            WarpParams pm = def;
            pm.T = T;
            pm.stubAbsolute = false;
            pm.stubK = k;
            Agg agg;
            std::string failing;
            for (const Case &c : cases) {
                const CaseEval ce = evaluateCase(c, pm);
                agg.add(ce, kAC);
                if (!ce.ends.reversed() && ce.v[kAC].cusps.total() != 0)
                    failing += (failing.empty() ? "" : ", ") + ce.name;
            }
            std::cout << pad(num(T, 2), 7) << pad(num(k, 1), 7)
                      << pad(std::to_string(agg.alignedZeroCusp) + "/"
                                 + std::to_string(agg.alignedCases),
                             16)
                      << pad(num(agg.maxFacingAll, 2), 12)
                      << pad(std::to_string(agg.overshootTotal), 11)
                      << pad(num(agg.blendMean(), 2), 14) << (failing.empty() ? "-" : failing)
                      << "\n";
        }
    }

    // ---- 5. results at the recommended defaults ---------------------------
    std::vector<CaseEval> evals;
    evals.reserve(cases.size());
    for (const Case &c : cases)
        evals.push_back(evaluateCase(c, def));

    std::cout << "\n--- 5. W4 / W5 at the recommended defaults ---\n";
    std::cout << pad("case", 26) << pad("class", 10) << pad("obliq", 8) << pad("A+C face", 10)
              << pad("B+C face", 10) << pad("W5 old A+C", 12) << pad("W5 new A+C", 12)
              << pad("W5 new B+C", 12) << pad("minR/bar u", 13) << pad("stub u", 9)
              << pad("over", 6) << pad("mid70 u", 9) << "blend mean u\n";
    for (const CaseEval &e : evals) {
        const CuspScore &ac = e.v[kAC].cusps;
        const CuspScore &bc = e.v[kBC].cusps;
        std::cout << pad(e.name, 26) << pad(e.ends.reversed() ? "REVERSED" : "aligned", 10)
                  << pad(num(e.ends.obliquity(), 1), 8)
                  << pad(num(e.v[kAC].worstFacingDeg(), 1), 10)
                  << pad(num(e.v[kBC].worstFacingDeg(), 1), 10) << pad(std::to_string(ac.old), 12)
                  << pad(std::to_string(ac.blend) + "+" + std::to_string(ac.middle), 12)
                  << pad(std::to_string(bc.blend) + "+" + std::to_string(bc.middle), 12)
                  << pad(num(std::min(ac.minRadiusBlend, 9999.0), 1) + "/" + num(ac.bar, 1), 13)
                  << pad(num(e.v[kAC].stubLen, 1), 9)
                  << pad(std::to_string(e.v[kAC].health.total()), 6)
                  << pad(num(e.v[kAC].fid.midMaxSample, 2), 9)
                  << num(e.v[kAC].fid.blendMeanSample, 2) << "\n";
    }
    for (int v = 0; v < kVariantCount; ++v) {
        int zeroOldAll = 0, zeroNewAll = 0, zeroOldAligned = 0, zeroNewAligned = 0, nAligned = 0;
        for (const CaseEval &e : evals) {
            const CuspScore &cs = e.v[static_cast<size_t>(v)].cusps;
            if (cs.old == 0)
                ++zeroOldAll;
            if (cs.total() == 0)
                ++zeroNewAll;
            if (!e.ends.reversed()) {
                ++nAligned;
                if (cs.old == 0)
                    ++zeroOldAligned;
                if (cs.total() == 0)
                    ++zeroNewAligned;
            }
        }
        std::cout << pad(variantName(v), 6) << "zero-cusp share:  all cases old " << zeroOldAll
                  << "/" << evals.size() << " new " << zeroNewAll << "/" << evals.size()
                  << "   |   ALIGNED ONLY old " << zeroOldAligned << "/" << nAligned << " new "
                  << zeroNewAligned << "/" << nAligned << " ("
                  << num(100.0 * zeroNewAligned / std::max(1, nAligned), 0) << "%)\n";
    }
    {
        int tight = 0, loose = 0, nAligned = 0;
        for (const CaseEval &e : evals) {
            if (e.ends.reversed())
                continue;
            ++nAligned;
            if (e.v[kAC].cusps.blendTight == 0 && e.v[kAC].cusps.middle == 0)
                ++tight;
            if (e.v[kAC].cusps.blendLoose == 0 && e.v[kAC].cusps.middle == 0)
                ++loose;
        }
        std::cout << "bar sensitivity (A+C, aligned): radius 8u -> " << tight << "/" << nAligned
                  << " clean, 12u -> chosen, 16u -> " << loose << "/" << nAligned << " clean\n";
    }

    // ---- 6. W7 centre-bind escape hatch -----------------------------------
    std::cout << "\n--- 6. W7 centre-bind escape hatch for every REVERSED case ---\n";
    std::cout << "facing is measured at s=0 as briefed; 'vis' is the same angle where the ink "
                 "leaves the clip boundary, which is what a creator actually sees\n";
    std::cout << pad("case", 26) << pad("converted", 12) << pad("A+C face/vis", 15)
              << pad("B+C face/vis", 15) << pad("A+C cusps", 11) << pad("B+C cusps", 11)
              << pad("cusps unclipped", 17) << pad("ink kept", 10) << "verdict\n";
    std::vector<std::pair<std::string, std::string>> w7Svgs;
    bool w7Pass = true;
    for (size_t idx : reversedIdx) {
        const Ends orig = resolveEnds(cases[idx], false, false);
        const bool c0 = !orig.aligned0;
        const bool c1 = !orig.aligned1;
        const CaseEval ce = evaluateCase(cases[idx], def, c0, c1);
        const bool ok = ce.v[kAC].cusps.total() == 0 && ce.v[kAC].worstFacingDeg() <= 5.0
            && ce.v[kBC].cusps.total() == 0 && ce.v[kBC].worstFacingDeg() <= 5.0;
        if (!ok)
            w7Pass = false;
        std::string conv;
        if (c0 && c1)
            conv = "both ends";
        else if (c0)
            conv = "end A";
        else
            conv = "end B";
        std::cout << pad(cases[idx].name, 26) << pad(conv, 12)
                  << pad(num(ce.v[kAC].worstFacingDeg(), 1) + "/"
                             + num(ce.v[kAC].facingVisibleStartDeg, 1),
                         15)
                  << pad(num(ce.v[kBC].worstFacingDeg(), 1) + "/"
                             + num(ce.v[kBC].facingVisibleStartDeg, 1),
                         15)
                  << pad(std::to_string(ce.v[kAC].cusps.blend) + "+"
                             + std::to_string(ce.v[kAC].cusps.middle),
                         11)
                  << pad(std::to_string(ce.v[kBC].cusps.blend) + "+"
                             + std::to_string(ce.v[kBC].cusps.middle),
                         11)
                  << pad("A+C " + std::to_string(ce.v[kAC].cusps.blendFull) + "  B+C "
                             + std::to_string(ce.v[kBC].cusps.blendFull),
                         17)
                  << pad(num(100.0 * ce.v[kAC].visibleShare, 0) + "%", 10)
                  << (ok ? "PASS" : "FAIL") << "\n";
        const Poly ghost = evals[idx].v[kAC].samples;
        w7Svgs.emplace_back(cases[idx].name,
                            caseSvg(cases[idx], ce, def, "  [W7: converted to centre bind]",
                                    &ghost));
    }
    std::cout << "W7 verdict: " << (w7Pass ? "PASS" : "FAIL")
              << " — every reversed case must reach 0 new cusps and <=5 deg facing when the "
                 "reversed end is switched to a centre bind\n";

    // ---- 7. W2 / W3 / W6 at the defaults ----------------------------------
    struct RtRow {
        std::string label;
        std::array<RoundTrip, kVariantCount> rt;
    };
    std::vector<RtRow> rtRows;
    for (const std::string &flavour : {std::string("arc"), std::string("wiggle")}) {
        const Case base = makeCase(flavour, "translate-near");
        const RestShape rest = buildRestShape(base.strokes);
        RtRow row;
        row.label = flavour;
        for (int v = 0; v < kVariantCount; ++v)
            row.rt[static_cast<size_t>(v)] = roundTripDrift(rest, base.a0, base.b0, base.anchorA,
                                                            base.anchorB, v, def);
        rtRows.push_back(row);
    }
    std::cout << "\n--- 7. W2 round-trip drift at the recommended defaults ---\n";
    std::cout << pad("rest shape", 12) << pad("variant", 9) << pad("never-re-bake (D5)", 22)
              << pad("re-bake, 20 moves", 20) << "re-bake, 200 moves\n";
    double worstPure = 0.0, worstRb1 = 0.0, worstRb10 = 0.0;
    for (const RtRow &r : rtRows) {
        for (int v = 0; v < kVariantCount; ++v) {
            const RoundTrip &rt = r.rt[static_cast<size_t>(v)];
            worstPure = std::max(worstPure, rt.pure);
            worstRb1 = std::max(worstRb1, rt.rebake1);
            worstRb10 = std::max(worstRb10, rt.rebake10);
            std::cout << pad(r.label, 12) << pad(variantName(v), 9)
                      << pad(num(rt.pure, 9) + " u", 22) << pad(num(rt.rebake1, 4) + " u", 20)
                      << num(rt.rebake10, 4) << " u\n";
        }
    }
    std::cout << "W2: never-re-bake max " << num(worstPure, 9) << " u -> "
              << (worstPure <= 1.0 ? "PASS" : "FAIL") << ";  re-bake max " << num(worstRb1, 4)
              << " u / " << num(worstRb10, 4) << " u -> fails as intended\n";

    bool doubleRunOk = true;
    std::string doubleRunMsg = "-";
    for (size_t i = 0; i < cases.size() && doubleRunOk; ++i) {
        const CaseEval a = evaluateCase(cases[i], def);
        const CaseEval b = evaluateCase(cases[i], def);
        for (int v = 0; v < kVariantCount; ++v) {
            if (!bitIdentical(a.v[static_cast<size_t>(v)].samples,
                              b.v[static_cast<size_t>(v)].samples)) {
                doubleRunOk = false;
                doubleRunMsg = cases[i].name + " variant " + variantName(v);
                break;
            }
        }
    }
    bool reverseOk = true;
    std::string reverseMsg = "-";
    for (size_t i = cases.size(); i-- > 0;) {
        const CaseEval e = evaluateCase(cases[i], def);
        for (int v = 0; v < kVariantCount; ++v) {
            if (!bitIdentical(e.v[static_cast<size_t>(v)].samples,
                              evals[i].v[static_cast<size_t>(v)].samples)) {
                reverseOk = false;
                reverseMsg = e.name + " variant " + variantName(v);
                break;
            }
        }
        if (!reverseOk)
            break;
    }
    std::cout << "\n--- W3 determinism at the recommended defaults ---\n";
    std::cout << "same inputs run twice   : "
              << (doubleRunOk ? "PASS (byte-identical)" : "FAIL " + doubleRunMsg) << "\n";
    std::cout << "case set run in reverse : "
              << (reverseOk ? "PASS (byte-identical)" : "FAIL " + reverseMsg) << "\n";

    const RestShape costRest = buildRestShape(
        {strokeBetween({330.0, 413.8}, {660.0, 380.0}, -30.0, 26.0, 2.7, 7.5, 0x5EED1234ULL, 500)});
    std::array<Timing, kVariantCount> timings{};
    for (int v = 0; v < kVariantCount; ++v)
        timings[static_cast<size_t>(v)] = timeWarp(costRest, {180.0, 640.0}, {1.0, 0.0},
                                                   {880.0, 250.0}, {-1.0, 0.0}, v, def, 1000);
    std::cout << "\n--- W6 cost: 1000 re-warps of a 500-sample connector ---\n";
    std::cout << pad("variant", 9) << pad("p50 (us)", 12) << "p95 (us)\n";
    for (int v = 0; v < kVariantCount; ++v)
        std::cout << pad(variantName(v), 9) << pad(num(timings[static_cast<size_t>(v)].p50, 2), 12)
                  << num(timings[static_cast<size_t>(v)].p95, 2) << "\n";
    std::cout << "bar: p95 <= 2000 us\n";

    // ---- 8. contact sheet -------------------------------------------------
    std::vector<std::pair<std::string, std::string>> svgs;
    for (size_t i = 0; i < cases.size(); ++i) {
        const std::string svg = caseSvg(cases[i], evals[i], def, "", nullptr);
        std::ofstream f(outDir + "/" + fileNameFor(cases[i].name) + ".svg");
        if (!f) {
            std::cerr << "cannot write " << outDir << "\n";
            return 1;
        }
        f << svg;
        svgs.emplace_back(cases[i].name, svg);
    }
    for (const auto &pair : w7Svgs) {
        std::ofstream f(outDir + "/" + fileNameFor(pair.first) + "__centre.svg");
        f << pair.second;
    }

    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset=\"utf-8\">\n";
    html << "<title>EXP-0002 round 2 — connector-ink warp contact sheet</title>\n";
    html << "<style>body{font:14px/1.5 -apple-system,Helvetica,Arial;margin:32px;color:#111;"
            "background:#fafafa}h1{font-size:22px}h2{font-size:16px;margin-top:38px;"
            "border-top:1px solid #ddd;padding-top:14px}h3{font-size:15px;margin-top:26px}"
            "svg{background:#fff;border:1px solid #e3e3e3;border-radius:6px;max-width:100%;"
            "height:auto}table{border-collapse:collapse;font:12px/1.4 ui-monospace,Menlo,"
            "monospace;margin:14px 0}td,th{border:1px solid #ddd;padding:4px 9px;"
            "text-align:left}th{background:#f0f0f0}.note{color:#555;margin:6px 0 10px}"
            ".rev{background:#fff6e5}</style></head><body>\n";
    html << "<h1>EXP-0002 round 2 — connector-ink warp contact sheet</h1>\n";
    html << "<p class=\"note\">Spike output at the recommended defaults: blend length "
            "<b>T = "
         << num(def.T, 2) << "</b> of arc length, departure stub <b>"
         << (def.stubAbsolute ? num(def.stubWorld, 0) + " world units"
                              : "k = " + num(def.stubK, 1) + " &times; T &times; L'")
         << "</b> on the outer handle only, samples placed on the <b>pre-blend</b> arc "
            "parameterization, degenerate clamp <b>"
         << (def.dTaper ? "d-taper under compression" : "none")
         << "</b>. <b>W1 (naturalness) is a human verdict and is not graded here.</b> Look for: "
            "does each coloured line still read as the same hand as the grey rest shape?</p>\n";
    html << "<p class=\"note\">Why these values (full sweeps are in the probe's stdout): at "
            "T=0.15 the departure stub peaks at k=1.5 with 17/18 aligned cases free of new "
            "cusps and 2.9&deg; worst facing; a world-unit stub cannot compete because the "
            "blend span it has to fill is itself proportional to L', so a 12 u handle inside a "
            "~100 u span hairpins at the face (33&deg; measured departure). Placing samples on "
            "the pre-blend parameterization drops the blend's leakage into the middle 70% from "
            "15.3 u to 0.02 u at no cost.</p>\n";
    html << "<p class=\"note\">Per D26 an edge anchor's facing is fixed and may oppose the "
            "chord; those cases are marked <b>REVERSED</b> and their U-turn is expected "
            "behaviour, not a defect. The remedy is the creator switching that end to a centre "
            "bind — rendered in the W7 section at the bottom.</p>\n";
    html << "<table><tr><th>variant</th><th>spine warp</th><th>tangent blend</th><th>stroke</th>"
            "</tr>\n";
    html << "<tr><td>A</td><td>similarity (uniform scale + rotate)</td><td>no</td><td>thin solid "
            "blue</td></tr>\n";
    html << "<tr><td>B</td><td>anisotropic (chord-only scale)</td><td>no</td><td>thin dashed "
            "green</td></tr>\n";
    html << "<tr><td>A+C</td><td>similarity</td><td>yes</td><td>thick solid red</td></tr>\n";
    html << "<tr><td>B+C</td><td>anisotropic</td><td>yes</td><td>thick dashed purple</td></tr>\n";
    html << "</table>\n";

    html << "<h2>Summary at the recommended defaults</h2>\n<table><tr><th>case</th><th>class"
            "</th><th>A+C face</th><th>B+C face</th><th>W5 old A+C</th><th>W5 new A+C</th>"
            "<th>W5 new B+C</th><th>min blend radius u</th><th>overshoot A+C</th>"
            "<th>A vs B u</th></tr>\n";
    for (const CaseEval &e : evals) {
        html << "<tr" << (e.ends.reversed() ? " class=\"rev\"" : "") << "><td>" << esc(e.name)
             << "</td><td>" << (e.ends.reversed() ? "REVERSED" : "aligned") << "</td><td>"
             << num(e.v[kAC].worstFacingDeg(), 1) << "</td><td>"
             << num(e.v[kBC].worstFacingDeg(), 1) << "</td><td>" << e.v[kAC].cusps.old
             << "</td><td>" << e.v[kAC].cusps.blend << "+" << e.v[kAC].cusps.middle
             << "</td><td>" << e.v[kBC].cusps.blend << "+" << e.v[kBC].cusps.middle
             << "</td><td>" << num(std::min(e.v[kAC].cusps.minRadiusBlend, 9999.0), 1)
             << "</td><td>" << e.v[kAC].health.total() << "</td><td>" << num(e.gapAB, 1)
             << "</td></tr>\n";
    }
    html << "</table>\n";

    html << "<h2>W2 round-trip drift</h2><table><tr><th>rest shape</th><th>variant</th>"
            "<th>never-re-bake (D5)</th><th>re-bake, 20 moves</th><th>re-bake, 200 moves</th>"
            "</tr>\n";
    for (const RtRow &r : rtRows) {
        for (int v = 0; v < kVariantCount; ++v)
            html << "<tr><td>" << r.label << "</td><td>" << variantName(v) << "</td><td>"
                 << num(r.rt[static_cast<size_t>(v)].pure, 9) << " u</td><td>"
                 << num(r.rt[static_cast<size_t>(v)].rebake1, 4) << " u</td><td>"
                 << num(r.rt[static_cast<size_t>(v)].rebake10, 4) << " u</td></tr>\n";
    }
    html << "</table>\n";
    html << "<h2>W6 cost (1000 re-warps, 500-sample connector)</h2><table><tr><th>variant</th>"
            "<th>p50 us</th><th>p95 us</th></tr>\n";
    for (int v = 0; v < kVariantCount; ++v)
        html << "<tr><td>" << variantName(v) << "</td><td>"
             << num(timings[static_cast<size_t>(v)].p50, 2) << "</td><td>"
             << num(timings[static_cast<size_t>(v)].p95, 2) << "</td></tr>\n";
    html << "</table>\n";

    html << "<h2>Cases at the recommended defaults</h2>\n";
    for (const auto &pair : svgs)
        html << "<h3>" << esc(pair.first) << "</h3>\n" << pair.second;

    html << "<h2>W7 — the centre-bind escape hatch for every REVERSED case</h2>\n";
    html << "<p class=\"note\">Same case, same rest ink, with the reversed end(s) switched to a "
            "centre bind. The faint pink dotted line is the edge-bind A+C result from the "
            "section above, for comparison.</p>\n";
    for (const auto &pair : w7Svgs)
        html << "<h3>" << esc(pair.first) << " — centre bind</h3>\n" << pair.second;
    html << "</body></html>\n";

    std::ofstream fh(outDir + "/index.html");
    if (!fh) {
        std::cerr << "cannot write index.html\n";
        return 1;
    }
    fh << html.str();
    std::cout << "\nwrote " << svgs.size() << " case SVGs + " << w7Svgs.size()
              << " W7 SVGs + index.html to " << outDir << "\n";
    return 0;
}
