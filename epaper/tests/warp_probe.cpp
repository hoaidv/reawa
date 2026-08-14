/**
 * EXP-0002 round 5 — spine-model tournament: Local vs Always-cubic vs Morph.
 *
 * SPIKE CODE. Host-only: no Qt, no third-party libraries, no network, no device.
 * Throwaway sandbox artefact for the warp naturalness question (BS-0001 D3/D5/D8/D16/D17/D26).
 * Carries no traceability annotations on purpose: this is not a shipping path and must be
 * re-implemented docs-first if any of it is ever promoted.
 *
 * Three candidates, same rest-shape store, same (s, d) offsets (absolute, never re-baked):
 *   Local   round-4 shipped: similarity warp + G1 Hermite end blend (control; not retuned)
 *   Cubic   always a cubic Hermite through the two new endpoints with the two facings (EH1)
 *   Morph   V = mix(U, C, m) with one mix factor for the whole connector; m(0)=0 (D27)
 * Quadratic bezier is mathematically insufficient (one interior control cannot hold two
 * independent facings) and is not built.
 *
 * Writes an SVG contact sheet plus index.html, and prints the tournament to stdout.
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
#include <sstream>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------- tunables

constexpr double kPi = 3.14159265358979323846;
constexpr double kResampleWorld = 2.0;    // uniform arc-length spacing of the spine
constexpr double kSpineSigmaWorld = 6.0;  // smoothing sigma used to derive the spine

// --- the tunables of the final model (product names in CANONICAL-ALGORITHM.md) ---
// Round 4 deletes the base blend length: the blend arc is purely what the turn demands, so
// it collapses to zero when there is no turn, which is what makes the identity exact.
constexpr double kStubRatio = 1.5;      // departure stub ratio, multiples of the blend arc
constexpr double kTurnFactor = 5.0;     // turn room, re-derived in round 4 section 2
constexpr double kBlendCap = 0.35;      // blend cap per end, re-decided in round 4 section 6
constexpr double kMinInkRadius = 12.0;  // minimum ink radius the warp may bend to

// Round-3 base blend length. Deleted from the model in round 4; not used in round 5.

constexpr double kMinInkRadiusTight = 8.0;   // reported for bar sensitivity only
constexpr double kMinInkRadiusLoose = 16.0;  // reported for bar sensitivity only
// Below this the whole connector is shorter than two minimum radii, so no radius bar can be
// stated for it at all and the cusp count is reported rather than graded. Round 4 uses a
// scoreability floor instead of round 3's span-relative bar relaxation (PM decision).
constexpr double kScoreableArcWorld = 2.0 * kMinInkRadius;

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
    double d = 0.0;  // signed perpendicular offset, world units, always absolute (D8)
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

// D27 (round 4): an edge anchor's facing is the drawn departure direction, carried rigidly in
// the edge's local frame. `EdgeNormal` is the round-3 rule, kept only as the control.
enum class FacingRule { DrawnDeparture, EdgeNormal };

// Where the turn each end must absorb is read off the warped spine. `AtAnchor` is the
// round-4 rule and is the only basis under which the identity can be exact, because the
// stored facing *is* the spine's own departure direction at that same point.
enum class TurnBasis { AtAnchor, AtFloorArc };

// Cubic Hermite handle (derivative magnitude over the unit interval).
// Chord     — Hermite speed = |c'|; Bezier interior controls sit chord/3 along each facing.
//             This is the unique speed that makes the cubic a straight line when both
//             facings already align with the chord.
// ChordThird — Hermite speed = |c'|/3; a short handle, tried because "chord/3" is also
//             readable as the speed itself.
// RestSpeed  — Hermite speed = L' of the unblended spine, i.e. match the rest shape's own
//             parametric speed when s is normalized arc.
enum class HandleMode { Chord, ChordThird, RestSpeed };

enum class MixMode {
    WorldLerp,    // V = (1-m) U + m C. Equals C at m=1. Linear, so basis-independent.
    TurningAngle  // mix unwrapped headings, reconstruct, then similarity-fit to the ends
};

struct WarpParams {
    double stubRatio = kStubRatio;
    double turnFactor = kTurnFactor;
    double blendCap = kBlendCap;
    bool adaptive = true;
    bool applyCap = true;
    FacingRule facing = FacingRule::DrawnDeparture;
    TurnBasis basis = TurnBasis::AtAnchor;
    // Round-3 base blend, as a fraction of arc per end. 0 in the round-4 model; set to
    // kBlendLengthR3 to reproduce round 3.
    double floorFrac = 0.0;
    // Zero-turn short circuit / deadband, degrees. 0 in the round-4 model.
    double deadbandDeg = 0.0;
    // D29: a centre anchor keeps the drawn departure, clamped to a 60° cone about the
    // peer ray. Negative = round-3 behaviour (the pure ray).
    double centreConeDeg = 60.0;
    // Sweep overrides: when >= 0 they replace the demanded blend arc at that end, in world
    // units. Absolute, because round 3 established the demand itself is absolute.
    double forceArc0 = -1.0;
    double forceArc1 = -1.0;
    // Round 5: cubic handle and morph mix. forceMix >= 0 replaces m(turn) for characterization.
    HandleMode handle = HandleMode::RestSpeed;
    MixMode mix = MixMode::WorldLerp;
    double forceMix = -1.0;
    // m(turn) saturation angle, degrees. m(0)=0; m(satDeg)=1 for the linear and versine
    // forms. Chosen from the characterization, not from taste.
    double morphSatDeg = 90.0;
    // 0 = linear in turn/sat, 1 = versine (zero derivative at 0), 2 = 1-exp.
    int morphForm = 1;
};


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

Vec2 hermite(Vec2 p0, Vec2 m0, Vec2 p1, Vec2 m1, double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    return p0 * (2 * t3 - 3 * t2 + 1) + m0 * (t3 - 2 * t2 + t) + p1 * (-2 * t3 + 3 * t2)
        + m1 * (t3 - t2);
}

struct BlendSpec {
    double t0 = 0.0, t1 = 0.0;          // blend length per end, fraction of arc
    double outer0 = 0.0, outer1 = 0.0;  // departure stub per end, world units
    double inner0 = 0.0, inner1 = 0.0;  // inner handle, matched to the blend arc
    double turn0 = 0.0, turn1 = 0.0;    // turn each end must absorb, degrees
    double chord0 = 0.0, chord1 = 0.0;  // angle from the facing to the blend chord, degrees
    bool capped0 = false, capped1 = false;
    bool grown0 = false, grown1 = false;  // the turn demand exceeded the base blend
};

// Variant C. Replaces the first t0 and last t1 of arc length with cubic Hermites that
// leave p0 along f0 and arrive at p1 along -f1.
Poly tangentBlend(const Poly &sp, Vec2 p0, Vec2 f0, Vec2 p1, Vec2 f1, const BlendSpec &b)
{
    if (sp.size() < 6)
        return sp;
    const std::vector<double> cum = arcTable(sp);
    const double total = cum.back();
    if (total < 1e-9)
        return sp;
    const Frame fT = frameAtArc(sp, cum, b.t0 * total);
    const Frame fE = frameAtArc(sp, cum, (1.0 - b.t1) * total);
    const Vec2 m0 = f0 * b.outer0;
    const Vec2 m1 = fT.t * b.inner0;
    const Vec2 n0 = fE.t * b.inner1;
    const Vec2 n1 = f1 * -b.outer1;
    Poly out = sp;
    for (size_t i = 0; i < sp.size(); ++i) {
        const double s = cum[i] / total;
        if (s < b.t0)
            out[i] = hermite(p0, m0, fT.p, m1, s / b.t0);
        else if (s > 1.0 - b.t1)
            out[i] = hermite(fE.p, n0, p1, n1, (s - (1.0 - b.t1)) / b.t1);
    }
    out.front() = p0;
    out.back() = p1;
    return out;
}

// `paramSpine` supplies the arc-length parameterization, `geomSpine` the geometry. They
// share indices, so parameterizing on the pre-blend spine keeps the blend strictly local.
Poly placeSamples(const Poly &paramSpine, const Poly &geomSpine, const std::vector<SD> &sd)
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
        out[i] = p + leftNormal(t) * sd[i].d;
    }
    return out;
}

enum Variant { kA = 0, kAC = 1, kCubic = 2, kMorph = 3 };
constexpr int kVariantCount = 4;
const char *variantName(int v)
{
    switch (v) {
    case kA:
        return "A";
    case kAC:
        return "Local";
    case kCubic:
        return "Cubic";
    case kMorph:
        return "Morph";
    default:
        return "?";
    }
}

double cubicHandleSpeed(const Poly &U, Vec2 p0, Vec2 p1, HandleMode mode)
{
    const double chord = vlen(p1 - p0);
    if (mode == HandleMode::ChordThird)
        return chord / 3.0;
    if (mode == HandleMode::RestSpeed) {
        const std::vector<double> cum = arcTable(U);
        return cum.empty() ? chord : cum.back();
    }
    return chord;  // Chord: Bezier offset = chord/3
}

// Sample a cubic Hermite at the same normalized-arc parameter as U. P'(0) = f0 * h0,
// P'(1) = -f1 * h1, so the far-end spine tangent points into the box (same convention as
// the local blend). Quadratic is not implemented: one interior control cannot represent
// two independent facings.
Poly cubicSpineAtU(const Poly &U, Vec2 p0, Vec2 f0, Vec2 p1, Vec2 f1, double h0, double h1)
{
    if (U.size() < 2)
        return U;
    const std::vector<double> cum = arcTable(U);
    const double total = cum.back();
    const Vec2 m0 = f0 * h0;
    const Vec2 m1 = f1 * -h1;
    Poly C(U.size());
    for (size_t i = 0; i < U.size(); ++i) {
        const double s = total > 1e-12 ? cum[i] / total : 0.0;
        C[i] = hermite(p0, m0, p1, m1, s);
    }
    C.front() = p0;
    C.back() = p1;
    return C;
}

double mixFromTurn(double turnDeg, const WarpParams &pm)
{
    if (pm.forceMix >= 0.0)
        return pm.forceMix;
    if (!(turnDeg > 0.0))
        return 0.0;
    const double sat = pm.morphSatDeg > 1e-9 ? pm.morphSatDeg : 90.0;
    if (pm.morphForm == 0) {
        return std::min(1.0, turnDeg / sat);
    }
    if (pm.morphForm == 2) {
        // 1-exp, scaled so m(sat)=1-exp(-k) with k=3 → m(sat)≈0.95
        return 1.0 - std::exp(-3.0 * turnDeg / sat);
    }
    // versine: m'(0)=0 so a 2° drag cannot pop; m(sat)=1
    const double u = std::min(1.0, turnDeg / sat);
    return 0.5 * (1.0 - std::cos(kPi * u));
}

Poly mixWorld(const Poly &U, const Poly &C, double m)
{
    if (!(m > 0.0) || U.size() != C.size())
        return U;
    if (m >= 1.0)
        return C;
    Poly V(U.size());
    const double om = 1.0 - m;
    for (size_t i = 0; i < U.size(); ++i)
        V[i] = U[i] * om + C[i] * m;
    V.front() = C.front();  // endpoints are shared; pin to the cubic's (equals U's)
    V.back() = C.back();
    return V;
}

// Unwrap a polyline's heading so consecutive samples differ by less than 180°.
std::vector<double> unwrappedHeading(const Poly &p)
{
    std::vector<double> th(p.size(), 0.0);
    if (p.size() < 2)
        return th;
    th[0] = std::atan2(p[1].y - p[0].y, p[1].x - p[0].x);
    for (size_t i = 1; i + 1 < p.size(); ++i) {
        const Vec2 d = p[i + 1] - p[i];
        double a = (dot(d, d) > 1e-24) ? std::atan2(d.y, d.x) : th[i - 1];
        while (a - th[i - 1] > kPi)
            a -= 2.0 * kPi;
        while (a - th[i - 1] < -kPi)
            a += 2.0 * kPi;
        th[i] = a;
    }
    th.back() = th[p.size() - 2];
    return th;
}

// Mix headings, walk with U's segment lengths, then similarity-fit onto (p0, p1) so the
// result still meets the new endpoints. At m=1 the headings are C's, so after the fit this
// is C up to sampling. At m=0 the walk reconstructs U and the fit is the identity (U
// already ends on (p0, p1)).
Poly mixTurning(const Poly &U, const Poly &C, double m, Vec2 p0, Vec2 p1)
{
    if (!(m > 0.0) || U.size() != C.size() || U.size() < 2)
        return U;
    if (m >= 1.0)
        return C;
    const std::vector<double> hu = unwrappedHeading(U);
    const std::vector<double> hc = unwrappedHeading(C);
    Poly raw(U.size());
    raw[0] = p0;
    for (size_t i = 0; i + 1 < U.size(); ++i) {
        const double len = vlen(U[i + 1] - U[i]);
        const double th = hu[i] * (1.0 - m) + hc[i] * m;
        raw[i + 1] = raw[i] + Vec2{std::cos(th), std::sin(th)} * len;
    }
    // Similarity fit of raw onto (p0, p1): translate is already p0; rotate+scale the chord.
    const Vec2 rawC = raw.back() - raw.front();
    const Vec2 wantC = p1 - p0;
    const double rawL = vlen(rawC);
    const double wantL = vlen(wantC);
    const double sc = (rawL > 1e-12) ? wantL / rawL : 1.0;
    const double th = (rawL > 1e-12 && wantL > 1e-12)
        ? std::atan2(wantC.y, wantC.x) - std::atan2(rawC.y, rawC.x)
        : 0.0;
    Poly V(U.size());
    for (size_t i = 0; i < raw.size(); ++i)
        V[i] = p0 + rot2((raw[i] - raw.front()) * sc, th);
    V.front() = p0;
    V.back() = p1;
    return V;
}

Poly mixSpines(const Poly &U, const Poly &C, double m, Vec2 p0, Vec2 p1, MixMode mode)
{
    if (mode == MixMode::TurningAngle)
        return mixTurning(U, C, m, p0, p1);
    return mixWorld(U, C, m);
}

struct WarpResult {
    Poly spine;
    Poly spineUnblended;
    Poly spineCubic;
    Poly samples;
    Poly samplesUnblended;
    double scale = 1.0;
    double lPrime = 0.0;
    BlendSpec blend;
    double mixM = 0.0;
    double handle0 = 0.0;
    double handle1 = 0.0;
};

// Single pass: the turn each end must absorb is read off the unblended spine at the anchor,
// then the blend is given the arc that turn needs. No iteration, so the result stays a pure
// function of the rest shape plus the endpoints.
//
// Round 4: there is no base blend floor. The identity is a consequence of the sizing rule.
// Round 5 still uses this for Local, and also to measure the turn that drives Morph's m.
BlendSpec planBlend(const Poly &spine, Vec2 f0, Vec2 f1, const WarpParams &pm, double lPrime)
{
    BlendSpec b;
    const std::vector<double> cum = arcTable(spine);
    const double floorArc = pm.floorFrac * lPrime;
    const double measureArc = (pm.basis == TurnBasis::AtFloorArc) ? floorArc : 0.0;
    const Frame q0 = frameAtArc(spine, cum, measureArc);
    const Frame q1 = frameAtArc(spine, cum, lPrime - measureArc);
    b.turn0 = angleBetweenDeg(f0, q0.t);
    b.turn1 = angleBetweenDeg(f1 * -1.0, q1.t);

    double arc0 = 0.0;
    double arc1 = 0.0;
    if (pm.adaptive) {
        arc0 = pm.turnFactor * kMinInkRadius * b.turn0 * kPi / 180.0;
        arc1 = pm.turnFactor * kMinInkRadius * b.turn1 * kPi / 180.0;
    }
    b.grown0 = arc0 > floorArc;
    b.grown1 = arc1 > floorArc;
    arc0 = (b.turn0 < pm.deadbandDeg) ? 0.0 : std::max(arc0, floorArc);
    arc1 = (b.turn1 < pm.deadbandDeg) ? 0.0 : std::max(arc1, floorArc);
    if (pm.forceArc0 >= 0.0)
        arc0 = pm.forceArc0;
    if (pm.forceArc1 >= 0.0)
        arc1 = pm.forceArc1;

    b.t0 = lPrime > 1e-9 ? arc0 / lPrime : 0.0;
    b.t1 = lPrime > 1e-9 ? arc1 / lPrime : 0.0;
    if (pm.applyCap) {
        b.capped0 = b.t0 > pm.blendCap;
        b.capped1 = b.t1 > pm.blendCap;
        b.t0 = std::min(b.t0, pm.blendCap);
        b.t1 = std::min(b.t1, pm.blendCap);
    }
    b.inner0 = b.t0 * lPrime;
    b.inner1 = b.t1 * lPrime;
    b.outer0 = pm.stubRatio * b.inner0;
    b.outer1 = pm.stubRatio * b.inner1;

    const Frame h0 = frameAtArc(spine, cum, b.inner0);
    const Frame h1 = frameAtArc(spine, cum, lPrime - b.inner1);
    b.chord0 = b.inner0 > 1e-6 ? angleBetweenDeg(f0, h0.p - spine.front()) : b.turn0;
    b.chord1 = b.inner1 > 1e-6 ? angleBetweenDeg(f1 * -1.0, spine.back() - h1.p) : b.turn1;
    return b;
}

WarpResult warpConnector(const RestShape &rs, Vec2 p0, Vec2 f0, Vec2 p1, Vec2 f1, int v,
                         const WarpParams &pm, bool wantUnblended = true)
{
    WarpResult out;
    out.scale = rs.chordLen > 1e-9 ? vlen(p1 - p0) / rs.chordLen : 1.0;
    out.spineUnblended = warpSpineSimilarity(rs, p0, p1, out.scale);
    out.lPrime = arcTable(out.spineUnblended).back();
    out.blend = planBlend(out.spineUnblended, f0, f1, pm, out.lPrime);
    out.spine = out.spineUnblended;
    if (v == kAC) {
        out.spine = tangentBlend(out.spineUnblended, p0, f0, p1, f1, out.blend);
    } else if (v == kCubic || v == kMorph) {
        const double h = cubicHandleSpeed(out.spineUnblended, p0, p1, pm.handle);
        out.handle0 = h;
        out.handle1 = h;
        out.spineCubic = cubicSpineAtU(out.spineUnblended, p0, f0, p1, f1, out.handle0, out.handle1);
        const double turn = std::max(out.blend.turn0, out.blend.turn1);
        out.mixM = mixFromTurn(turn, pm);
        if (v == kCubic) {
            out.spine = out.spineCubic;
        } else if (out.mixM > 0.0) {
            out.spine = mixSpines(out.spineUnblended, out.spineCubic, out.mixM, p0, p1, pm.mix);
        }
    }
    // Local / Morph / Cubic all re-place (s, d) using U's arc as the parameter so s is the
    // same correspondence the morph mixes on. d stays absolute.
    out.samples = placeSamples(out.spineUnblended, out.spine, rs.sd);
    if (wantUnblended)
        out.samplesUnblended = placeSamples(out.spineUnblended, out.spineUnblended, rs.sd);
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
    // D27: the drawn departure direction, captured once at recognition. Stored in the edge's
    // own frame (x = along the outward normal, y = along the edge) so it is invariant under a
    // move and a resize and rotates rigidly with the box. `boxLocal` is the same direction in
    // the box frame, used if this end is later switched to a centre bind.
    Vec2 drawnEdgeLocal{1.0, 0.0};
    Vec2 drawnBoxLocal{1.0, 0.0};
    bool hasDrawn = false;
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

const Vec2 kEdgeNormalLocal[4] = {{0.0, -1.0}, {1.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0}};
const Vec2 kEdgeAlongLocal[4] = {{1.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0}, {0.0, -1.0}};

// The edge's own orthonormal frame in world space: outward normal, then along-edge. Both are
// the box rotation applied to a constant, so a direction stored in this frame survives a move
// and a resize untouched and rotates with the box. Storing in the edge frame rather than the
// box frame is a relabelling, but it makes the stored value readable as "how far off
// perpendicular the creator drew", which is the quantity D26/D27 is about.
void edgeFrame(const Box &b, int edge, Vec2 &n, Vec2 &along)
{
    n = rot2(kEdgeNormalLocal[edge], b.rot);
    along = rot2(kEdgeAlongLocal[edge], b.rot);
}

Vec2 toEdgeLocal(const Box &b, int edge, Vec2 world)
{
    Vec2 n{}, al{};
    edgeFrame(b, edge, n, al);
    return {dot(world, n), dot(world, al)};
}

Vec2 fromEdgeLocal(const Box &b, int edge, Vec2 local)
{
    Vec2 n{}, al{};
    edgeFrame(b, edge, n, al);
    return unit(n * local.x + al * local.y);
}

// Signed angle from `a` to `b`, degrees, positive counter-clockwise in a y-down world.
double signedAngleDeg(Vec2 a, Vec2 b)
{
    return std::atan2(cross(a, b), dot(a, b)) * 180.0 / kPi;
}

// D26 + D27. An edge anchor's facing is the creator's own drawn departure, carried rigidly
// with the edge; the edge is never re-selected, so the facing may still end up opposing the
// chord and the connector U-turns. A centre anchor derives its facing from the ray toward the
// peer, so it can never oppose the chord. `centreConeDeg >= 0` is the round-4 proposal:
// a centre anchor keeps the drawn departure too, but limited to a cone about that ray.
Vec2 anchorFacing(const Box &b, const Anchor &a, Vec2 towardOther, const WarpParams &pm)
{
    if (a.kind == AnchorKind::Centre) {
        const Vec2 ray = unit(towardOther - b.c);
        if (pm.centreConeDeg < 0.0 || !a.hasDrawn || pm.facing == FacingRule::EdgeNormal)
            return ray;
        const Vec2 drawn = unit(rot2(a.drawnBoxLocal, b.rot));
        const double off = signedAngleDeg(ray, drawn);
        if (std::abs(off) <= pm.centreConeDeg)
            return drawn;
        const double lim = (off > 0.0 ? pm.centreConeDeg : -pm.centreConeDeg) * kPi / 180.0;
        return unit(rot2(ray, lim));
    }
    if (pm.facing == FacingRule::EdgeNormal || !a.hasDrawn)
        return unit(rot2(kEdgeNormalLocal[a.edge], b.rot));
    return fromEdgeLocal(b, a.edge, a.drawnEdgeLocal);
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

// D17: a centre-bound end clips the ink at the box boundary. The full rest shape is kept
// so a later move or resize re-clips correctly (human decision, round 3).
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
    std::string flavour;
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

uint64_t flavourSeed(const std::string &flavour)
{
    return flavour == "arc" ? 0xC0FFEEULL : 0xBADCAFEULL;
}

const char *kScenarios[11] = {"identity",   "translate-near",   "translate-far",
                              "chord-flip", "rotate-a",         "resize-a",
                              "both-move",  "degenerate",       "detour-third-box",
                              "chain-3-stroke", "centre-clip"};

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
    const uint64_t seed = flavourSeed(flavour);

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
        c.note = "rest ink detours over a third box; the warp drags the detour with it "
                 "(D12 interior pins are out of scope)";
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
        c.note = "centre-bound start (D17): facing is the ray toward the peer, clipped at A's "
                 "AABB; the hidden ink is kept so a later move re-clips correctly";
    } else {
        c.strokes.push_back(flavourStroke(flavour, p0, p1, seed));
        if (scenario == "identity") {
            c.note = "nothing moved -- I6 requires the output to be the drawn ink, bit for bit";
        } else if (scenario == "translate-near") {
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
            c.note = "endpoints ~3 world units apart; similarity scale is ~0.009";
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

// Rotation of box A in isolation, for the obliquity characterization.
Case makeRotationCase(const std::string &flavour, double deg)
{
    Case c = makeCase(flavour, "rotate-a");
    c.a1 = c.a0;
    c.a1.rot = deg * kPi / 180.0;
    c.scenario = "rot";
    c.name = flavour + "/rot-" + std::to_string(static_cast<int>(std::llround(deg)));
    return c;
}

// Peer distance sweep, for the case where the adaptive rule runs out of arc.
Case makeShortCase(const std::string &flavour, double centreGap, double deg)
{
    Case c;
    c.flavour = flavour;
    c.scenario = "short";
    c.a0 = kA0;
    c.b0 = kA0;
    c.b0.c = kA0.c + Vec2{centreGap, 0.0};
    c.b0.hw = 40.0;
    c.b0.hh = 34.0;
    c.a0.hw = 40.0;
    c.a0.hh = 34.0;
    c.anchorA = kAnchorA;
    c.anchorB = kAnchorB;
    const Vec2 p0 = anchorPoint(c.a0, c.anchorA);
    const Vec2 p1 = anchorPoint(c.b0, c.anchorB);
    const double span = vlen(p1 - p0);
    const bool wig = (flavour != "arc");
    c.strokes.push_back(strokeBetween(p0, p1, wig ? -0.10 * span : -0.18 * span,
                                      wig ? 0.06 * span : 0.0, 2.2, (wig ? 0.04 : 0.02) * span,
                                      flavourSeed(flavour), 220));
    c.a1 = c.a0;
    c.b1 = c.b0;
    c.a1.rot = deg * kPi / 180.0;
    c.name = flavour + "/short-" + std::to_string(static_cast<int>(std::llround(span))) + "u-"
        + std::to_string(static_cast<int>(std::llround(deg))) + "deg";
    return c;
}

// ---------------------------------------------------------------- recognition

// Recognition, once per connector. The drawn departure at each end is read off the rest
// spine, not the raw ink: the spine is what the blend acts on, so this is the direction the
// identity has to reproduce, and a smoothed centreline is far more stable than the tangent
// between the first two raw samples.
struct Prepared {
    RestShape rest;
    Anchor a0{}, a1{};
    Vec2 drawn0{}, drawn1{};  // outward drawn departure at recognition, world space
    double offNormal0 = 0.0;  // stored departure as an angle off the edge normal, degrees
    double offNormal1 = 0.0;
};

Prepared prepare(const Case &c)
{
    Prepared p;
    p.rest = buildRestShape(c.strokes);
    p.a0 = c.anchorA;
    p.a1 = c.anchorB;
    const Poly &S = p.rest.spine;
    if (S.size() >= 2) {
        p.drawn0 = segTangentAt(S, 0);
        p.drawn1 = segTangentAt(S, S.size() - 2) * -1.0;  // outward at the far end
        p.a0.drawnEdgeLocal = toEdgeLocal(c.a0, p.a0.edge, p.drawn0);
        p.a1.drawnEdgeLocal = toEdgeLocal(c.b0, p.a1.edge, p.drawn1);
        p.a0.drawnBoxLocal = rot2(p.drawn0, -c.a0.rot);
        p.a1.drawnBoxLocal = rot2(p.drawn1, -c.b0.rot);
        p.a0.hasDrawn = true;
        p.a1.hasDrawn = true;
        p.offNormal0 = signedAngleDeg(rot2(kEdgeNormalLocal[p.a0.edge], c.a0.rot), p.drawn0);
        p.offNormal1 = signedAngleDeg(rot2(kEdgeNormalLocal[p.a1.edge], c.b0.rot), p.drawn1);
    }
    return p;
}

// ---------------------------------------------------------------- ends and alignment

struct Ends {
    Vec2 p0{}, f0{}, p1{}, f1{};
    bool centre0 = false, centre1 = false;
    bool aligned0 = true, aligned1 = true;
    double obliq0 = 0.0, obliq1 = 0.0;
    Aabb box0{}, box1{};
    bool reversed() const { return !aligned0 || !aligned1; }
    double obliquity() const { return std::max(obliq0, obliq1); }
};

Ends resolveEnds(const Case &c, const Prepared &pp, const WarpParams &pm, bool forceCentre0,
                 bool forceCentre1)
{
    Anchor a0 = pp.a0;
    Anchor a1 = pp.a1;
    if (forceCentre0)
        a0.kind = AnchorKind::Centre;
    if (forceCentre1)
        a1.kind = AnchorKind::Centre;
    Ends e;
    e.centre0 = (a0.kind == AnchorKind::Centre);
    e.centre1 = (a1.kind == AnchorKind::Centre);
    e.p0 = anchorPoint(c.a1, a0);
    e.p1 = anchorPoint(c.b1, a1);
    e.f0 = anchorFacing(c.a1, a0, e.p1, pm);
    e.f1 = anchorFacing(c.b1, a1, e.p0, pm);
    e.obliq0 = angleBetweenDeg(e.f0, e.p1 - e.p0);
    e.obliq1 = angleBetweenDeg(e.f1, e.p0 - e.p1);
    e.aligned0 = e.centre0 || e.obliq0 < 90.0;
    e.aligned1 = e.centre1 || e.obliq1 < 90.0;
    e.box0 = c.a1.aabb();
    e.box1 = c.b1.aabb();
    return e;
}

// ---------------------------------------------------------------- metrics

struct CuspScore {
    int blend = 0;
    int middle = 0;
    int old = 0;        // round-1 reading: rest-relative bar everywhere
    int blendFull = 0;  // same bar, ignoring the boundary-clip window
    double minRadiusBlend = 1e9;
    double bar0 = 0.0, bar1 = 0.0;
    int blendTight = 0;
    int blendLoose = 0;
    bool hasBlend = false;
    // Vertices inside a blend region that are tighter than the bar but were already tighter
    // than the bar before any blending. These are the creator's own curves, not new cusps.
    int waived = 0;
    int total() const { return blend + middle; }
};

// Round 4: a flat bar. Round 3 relaxed it in proportion to the blend span, which let a capped
// blend hide its own shortfall; the PM replaced that with the L' >= 24 u scoreability floor.
double radiusBar(double, double, double base) { return base; }

double vertexRadius(const Poly &p, size_t i)
{
    if (i == 0 || i + 1 >= p.size())
        return 1e9;
    const Vec2 a = p[i] - p[i - 1];
    const Vec2 c = p[i + 1] - p[i];
    const double la = vlen(a);
    const double lb = vlen(c);
    if (la < 1e-10 || lb < 1e-10)
        return 1e9;
    const double turn = angleBetweenDeg(a, c);
    return turn > 1e-9 ? 0.5 * (la + lb) * 180.0 / (kPi * turn) : 1e9;
}

// `ref` is the same spine before blending, vertex for vertex. A cusp counts as NEW only if the
// blended vertex is tighter than the bar and the unblended one was not: an absolute bar applied
// blind grades the creator's own curves once the blend region grows large, which is round 1's
// lesson in the opposite direction.
CuspScore scoreCusps(const Poly &sp, const Poly *ref, const BlendSpec &b, double lPrime,
                     double restMaxTurnDeg, double sLo, double sHi)
{
    CuspScore cs;
    cs.bar0 = radiusBar(b.t0, lPrime, kMinInkRadius);
    cs.bar1 = radiusBar(b.t1, lPrime, kMinInkRadius);
    cs.hasBlend = (b.t0 > 0.0 || b.t1 > 0.0);
    if (sp.size() < 3)
        return cs;
    const std::vector<double> cum = arcTable(sp);
    const double total = cum.back() > 1e-12 ? cum.back() : 1.0;
    const double oldThr = 1.5 * restMaxTurnDeg + 5.0;
    const bool haveRef = ref && ref->size() == sp.size();
    for (size_t i = 1; i + 1 < sp.size(); ++i) {
        const Vec2 a = sp[i] - sp[i - 1];
        const Vec2 c = sp[i + 1] - sp[i];
        if (vlen(a) < 1e-10 || vlen(c) < 1e-10)
            continue;
        const double s = cum[i] / total;
        const bool inStart = s < b.t0;
        const bool inEnd = s > 1.0 - b.t1;
        const double turn = angleBetweenDeg(a, c);
        const double radius = vertexRadius(sp, i);
        const double bar = inStart ? cs.bar0 : cs.bar1;
        const double refRadius = haveRef ? vertexRadius(*ref, i) : 1e9;
        if ((inStart || inEnd) && radius < bar && refRadius >= bar)
            ++cs.blendFull;
        if (s < sLo || s > sHi)
            continue;
        if (turn > oldThr)
            ++cs.old;
        if (inStart || inEnd) {
            cs.minRadiusBlend = std::min(cs.minRadiusBlend, radius);
            if (radius < bar) {
                if (refRadius >= bar)
                    ++cs.blend;
                else
                    ++cs.waived;
            }
            if (radius < kMinInkRadiusTight && refRadius >= kMinInkRadiusTight)
                ++cs.blendTight;
            if (radius < kMinInkRadiusLoose && refRadius >= kMinInkRadiusLoose)
                ++cs.blendLoose;
        } else if (turn > oldThr) {
            ++cs.middle;
        }
    }
    return cs;
}

// Overshoot: the curve passes its target and comes back, or crosses the body. Only
// meaningful on aligned ends — a reversed edge bind must backtrack, that is the U-turn.
struct BlendHealth {
    int backtracks = 0;
    int selfIntersect = 0;
    int total() const { return backtracks + selfIntersect; }
};

BlendHealth blendHealth(const Poly &sp, const BlendSpec &b)
{
    BlendHealth h;
    if (sp.size() < 6)
        return h;
    // With no blend at either end there is no blend region to be unhealthy. Scoring the whole
    // curve here would charge the blend for self-crossings the creator drew.
    if (b.t0 <= 0.0 && b.t1 <= 0.0)
        return h;
    const std::vector<double> cum = arcTable(sp);
    const double total = cum.back();
    if (total < 1e-9)
        return h;
    size_t jT = 0;
    size_t jE = sp.size() - 1;
    for (size_t i = 0; i < sp.size(); ++i) {
        if (cum[i] / total < b.t0)
            jT = i;
    }
    for (size_t i = sp.size(); i-- > 0;) {
        if (cum[i] / total > 1.0 - b.t1)
            jE = i;
    }
    if (b.t0 <= 0.0)
        jT = 0;
    if (b.t1 <= 0.0)
        jE = sp.size() - 1;
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
        const bool inStartBlend = b.t0 > 0.0 && i <= jT;
        const bool inEndBlend = b.t1 > 0.0 && i >= jE;
        if (!inStartBlend && !inEndBlend)
            continue;
        for (size_t j = i + 2; j + 1 < sp.size(); ++j) {
            if (properCross(sp[i], sp[i + 1], sp[j], sp[j + 1]))
                ++h.selfIntersect;
        }
    }
    return h;
}

// What the blend costs in drawn character, compared against the unblended warp.
struct Fidelity {
    double midMaxSample = 0.0;   // over the untouched middle, s in [t0, 1-t1]
    double blendMeanSample = 0.0;
    double blendFrac = 0.0;      // share of arc length spent on the transition
};

Fidelity fidelity(const RestShape &rs, const Poly &blended, const Poly &plain,
                  const BlendSpec &b)
{
    Fidelity f;
    f.blendFrac = b.t0 + b.t1;
    if (blended.size() != plain.size())
        return f;
    double acc = 0.0;
    int n = 0;
    for (size_t i = 0; i < blended.size(); ++i) {
        const double s = rs.sd[i].s;
        const double dev = vlen(blended[i] - plain[i]);
        if (s >= b.t0 && s <= 1.0 - b.t1)
            f.midMaxSample = std::max(f.midMaxSample, dev);
        else {
            acc += dev;
            ++n;
        }
    }
    f.blendMeanSample = n ? acc / static_cast<double>(n) : 0.0;
    return f;
}

// Whole-line spend vs the unblended warp. The right unit for a global morph: blend share
// is meaningless when there is no untouched middle.
struct Spend {
    double maxDev = 0.0;
    double meanDev = 0.0;
    double fracMoved1u = 0.0;
    int n = 0;
};

Spend spendVs(const Poly &a, const Poly &b)
{
    Spend s;
    if (a.size() != b.size() || a.empty())
        return s;
    double acc = 0.0;
    int moved = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        const double d = vlen(a[i] - b[i]);
        s.maxDev = std::max(s.maxDev, d);
        acc += d;
        if (d > 1.0)
            ++moved;
    }
    s.n = static_cast<int>(a.size());
    s.meanDev = acc / static_cast<double>(s.n);
    s.fracMoved1u = static_cast<double>(moved) / static_cast<double>(s.n);
    return s;
}

double meanSideOfChord(const Poly &p)
{
    if (p.size() < 2)
        return 0.0;
    const Vec2 n = leftNormal(unit(p.back() - p.front()));
    double acc = 0.0;
    for (const Vec2 &q : p)
        acc += dot(q - p.front(), n);
    return acc / static_cast<double>(p.size());
}

int inflectionCount(const Poly &p)
{
    if (p.size() < 4)
        return 0;
    int n = 0;
    double prev = 0.0;
    bool have = false;
    for (size_t i = 1; i + 1 < p.size(); ++i) {
        const double cr = cross(p[i] - p[i - 1], p[i + 1] - p[i]);
        if (std::abs(cr) < 1e-12)
            continue;
        if (have && ((prev > 0.0) != (cr > 0.0)))
            ++n;
        prev = cr;
        have = true;
    }
    return n;
}

CuspScore scoreCuspsWhole(const Poly &sp, const Poly *ref)
{
    BlendSpec all;
    all.t0 = 1.0;
    all.t1 = 0.0;
    return scoreCusps(sp, ref, all, arcTable(sp).empty() ? 0.0 : arcTable(sp).back(), 0.0, 0.0,
                      1.0);
}

BlendHealth wholeHealth(const Poly &sp)
{
    BlendHealth h;
    if (sp.size() < 4)
        return h;
    const Vec2 dir = unit(sp.back() - sp.front());
    double prev = 0.0;
    for (size_t i = 1; i < sp.size(); ++i) {
        const double u = dot(sp[i] - sp[0], dir);
        if (u < prev - 1e-9)
            ++h.backtracks;
        prev = u;
    }
    for (size_t i = 0; i + 1 < sp.size(); ++i) {
        for (size_t j = i + 2; j + 1 < sp.size(); ++j) {
            if (properCross(sp[i], sp[i + 1], sp[j], sp[j + 1]))
                ++h.selfIntersect;
        }
    }
    return h;
}

struct VariantEval {
    Poly spine;
    Poly samples;
    Poly samplesUnclipped;
    double facingStartDeg = 0.0;
    double facingEndDeg = 0.0;
    double facingVisibleStartDeg = 0.0;
    double visibleShare = 1.0;
    CuspScore cusps;
    BlendHealth health;
    Fidelity fid;
    Spend spend;
    BlendSpec blend;
    double mixM = 0.0;
    double handle0 = 0.0;
    int inflections = 0;
    double analyticFacingDeg = 0.0;  // constructed derivative vs facing; 0 by construction for Cubic
    // W4 applies to edge-bound ends only; a centre bind is measured at the boundary
    // crossing and carries no facing bar (round-3 restatement).
    double facingBarDeg = 0.0;
    double facingBaseline = 0.0;
    double facingBoundDeg = 0.0;
    double devFromInk = 0.0;
    double restMinRadiusInBlend = 1e9;
    bool identicalToA = false;
};

struct CaseEval {
    std::string name;
    std::string note;
    RestShape rest;
    Ends ends;
    std::array<VariantEval, kVariantCount> v{};
    Poly restDisplay;
    double scale = 1.0;
    double lPrime = 0.0;
    double offNormal0 = 0.0, offNormal1 = 0.0;
    bool blendIsIdentity = false;
    double blendMoved = 0.0;
};

Vec2 endTangentStart(const Poly &p)
{
    return p.size() < 2 ? Vec2{1.0, 0.0} : segTangentAt(p, 0);
}
Vec2 endTangentEnd(const Poly &p)
{
    return p.size() < 2 ? Vec2{1.0, 0.0} : segTangentAt(p, p.size() - 2);
}

bool bitIdenticalPoly(const Poly &a, const Poly &b)
{
    if (a.size() != b.size())
        return false;
    if (a.empty())
        return true;
    return std::memcmp(a.data(), b.data(), a.size() * sizeof(Vec2)) == 0;
}

CaseEval evaluatePrepared(const Case &c, const Prepared &pp, const WarpParams &pm,
                          bool forceCentre0 = false, bool forceCentre1 = false)
{
    CaseEval e;
    e.name = c.name;
    e.note = c.note;
    e.rest = pp.rest;
    e.offNormal0 = pp.offNormal0;
    e.offNormal1 = pp.offNormal1;
    e.ends = resolveEnds(c, pp, pm, forceCentre0, forceCentre1);

    e.restDisplay = e.rest.raw;
    {
        const Aabb ra = c.a0.aabb();
        const Aabb rb = c.b0.aabb();
        clipEnds(e.restDisplay, e.ends.centre0 ? &ra : nullptr,
                 e.ends.centre1 ? &rb : nullptr);
    }

    for (int v = 0; v < kVariantCount; ++v) {
        const WarpResult w = warpConnector(e.rest, e.ends.p0, e.ends.f0, e.ends.p1, e.ends.f1,
                                           v, pm);
        VariantEval ve;
        ve.spine = w.spine;
        ve.samplesUnclipped = w.samples;
        ve.samples = w.samples;
        ve.blend = w.blend;
        e.scale = w.scale;
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
        ve.facingBarDeg = 0.0;
        if (!e.ends.centre0)
            ve.facingBarDeg = std::max(ve.facingBarDeg, ve.facingStartDeg);
        if (!e.ends.centre1)
            ve.facingBarDeg = std::max(ve.facingBarDeg, ve.facingEndDeg);

        ve.cusps = (v == kCubic || v == kMorph)
            ? scoreCuspsWhole(ve.spine, &w.spineUnblended)
            : scoreCusps(ve.spine, &w.spineUnblended, w.blend, w.lPrime, e.rest.restMaxTurnDeg,
                         sLo, sHi);
        if (ve.spine.size() >= 2) {
            const double seg0 = vlen(ve.spine[1] - ve.spine[0]);
            const double segN = vlen(ve.spine[ve.spine.size() - 1] - ve.spine[ve.spine.size() - 2]);
            ve.facingBaseline = std::max(seg0, segN);
            const double r = std::min(ve.cusps.minRadiusBlend, 9999.0);
            ve.facingBoundDeg = r > 1e-6 ? 0.5 * ve.facingBaseline / r * 180.0 / kPi : 0.0;
        }
        ve.devFromInk = maxDeviation(w.samples, e.rest.raw);
        ve.spend = spendVs(w.samples, w.samplesUnblended);
        ve.identicalToA = bitIdenticalPoly(w.samples, w.samplesUnblended);
        ve.mixM = w.mixM;
        ve.handle0 = w.handle0;
        ve.inflections = inflectionCount(ve.spine);
        ve.fid = fidelity(e.rest, w.samples, w.samplesUnblended, w.blend);

        if (v == kCubic) {
            // Constructed derivative is f0 * h0 / -f1 * h1. Analytic deviation is 0.
            ve.analyticFacingDeg = 0.0;
            ve.health = wholeHealth(ve.spine);
        } else if (v == kMorph) {
            // Position lerp: V'(0) = (1-m) U'(0) + m C'(0). Equals facing only at m=1.
            const Vec2 uT = endTangentStart(w.spineUnblended);
            const Vec2 cT = e.ends.f0;
            const Vec2 vT = uT * (1.0 - w.mixM) + cT * w.mixM;
            const Vec2 uE = endTangentEnd(w.spineUnblended) * -1.0;
            const Vec2 cE = e.ends.f1;
            const Vec2 vE = uE * (1.0 - w.mixM) + cE * w.mixM;
            ve.analyticFacingDeg = std::max(angleBetweenDeg(vT, e.ends.f0),
                                            angleBetweenDeg(vE, e.ends.f1));
            ve.health = wholeHealth(ve.spine);
        } else if (v == kAC) {
            ve.analyticFacingDeg = (w.blend.t0 > 0.0 || w.blend.t1 > 0.0) ? 0.0
                                                                          : ve.facingBarDeg;
            ve.health = blendHealth(ve.spine, w.blend);
            e.blendIsIdentity = ve.identicalToA;
            e.blendMoved = ve.spend.maxDev;
            ve.restMinRadiusInBlend = scoreCusps(w.spineUnblended, nullptr, w.blend, w.lPrime,
                                                 e.rest.restMaxTurnDeg, sLo, sHi)
                                          .minRadiusBlend;
        }
        e.v[static_cast<size_t>(v)] = ve;
    }
    return e;
}

CaseEval evaluateCase(const Case &c, const WarpParams &pm, bool forceCentre0 = false,
                      bool forceCentre1 = false)
{
    return evaluatePrepared(c, prepare(c), pm, forceCentre0, forceCentre1);
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
    // qlmanage rasterises an SVG by fitting its HEIGHT into the square thumbnail and cropping the
    // width, so a canvas wider than it is tall loses its right-hand side in the optional PNGs.
    // The height therefore follows the content's aspect ratio but is never allowed below the
    // width. Ink size is set by the width either way; this only controls dead space.
    const double W = 1100.0;
    const double headerH = 34.0 + 17.0 * static_cast<double>(labelLines.size());
    const double legendH = 34.0;
    const double pad = 26.0;
    const double padRight = 88.0;
    const double plotW = W - pad - padRight;
    const double bw = std::max(1.0, sc.maxX - sc.minX);
    const double bh = std::max(1.0, sc.maxY - sc.minY);
    const double plotH = std::min(1000.0, std::max(W - headerH - legendH,
                                                   plotW * bh / bw + 2.0 * pad));
    const double H = headerH + plotH + legendH;
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
        {"as drawn", "#bbbbbb", 3.0, ""},
        {"carried rigidly (U)", "#8fbde0", 6.0, ""},
        {"Local", "#d62728", 2.2, ""},
        {"Always-cubic", "#2ca02c", 2.2, ""},
        {"Morph", "#ff7f0e", 2.2, ""},
        {"Local blend region", "#f3c98b", 9.0, ""},
    };
    double lx = 18.0;
    const double lyy = H - 12.0;
    for (const LegendItem &li : legend) {
        o << "<line x1=\"" << num(lx, 0) << "\" y1=\"" << num(lyy - 4, 0) << "\" x2=\""
          << num(lx + 30, 0) << "\" y2=\"" << num(lyy - 4, 0) << "\" stroke=\"" << li.color
          << "\" stroke-width=\"" << num(li.width) << "\"";
        if (li.dash[0])
            o << " stroke-dasharray=\"" << li.dash << "\"";
        o << "/>\n";
        o << "<text x=\"" << num(lx + 36, 0) << "\" y=\"" << num(lyy, 0)
          << "\" font-family=\"-apple-system,Helvetica,Arial\" font-size=\"12\" fill=\"#333\">"
          << li.label << "</text>\n";
        lx += 36.0 + 6.6 * static_cast<double>(std::strlen(li.label)) + 22.0;
    }
    o << "</svg>\n";
    return o.str();
}

// The caption is monospace 12px on an 1100px canvas, so a line past ~140 characters would run off
// the artifact. Wrap on a space and indent the continuation.
void pushWrapped(std::vector<std::string> &lines, const std::string &line)
{
    const size_t limit = 140;
    std::string rest = line;
    bool first = true;
    while (rest.size() > limit) {
        size_t cut = rest.rfind(' ', limit);
        if (cut == std::string::npos || cut < limit / 2)
            cut = limit;
        lines.push_back((first ? "" : "    ") + rest.substr(0, cut));
        rest = rest.substr(cut + (rest[cut] == ' ' ? 1 : 0));
        first = false;
    }
    if (!rest.empty())
        lines.push_back((first ? "" : "    ") + rest);
}

std::string endLabel(bool centre, bool aligned)
{
    if (centre)
        return "centre(derived)";
    return aligned ? "edge:aligned" : "edge:REVERSED";
}

// The stretch of output the blend is allowed to alter, laid under the line as a broad band so
// the untouched middle is visible at a glance. This is the claim the W1 verdict rests on.
void addBlendBand(Scene &sc, const Poly &pts, double frac, bool fromStart)
{
    if (pts.size() < 3 || frac <= 0.0)
        return;
    const std::vector<double> cum = arcTable(pts);
    const double total = cum.back();
    if (total <= 0.0)
        return;
    const double want = frac * total;
    Poly band;
    if (fromStart) {
        for (size_t i = 0; i < pts.size(); ++i) {
            band.push_back(pts[i]);
            if (cum[i] >= want)
                break;
        }
    } else {
        for (size_t i = pts.size(); i-- > 0;) {
            band.push_back(pts[i]);
            if (total - cum[i] >= want)
                break;
        }
    }
    sc.addPoly(band, "#f3c98b", 9.0, "", 0.85);
}

// Short tick across the spine marking where the blend region ends.
void addBlendTick(Scene &sc, const Poly &spine, double s)
{
    if (spine.size() < 2 || s <= 0.0 || s >= 1.0)
        return;
    const std::vector<double> cum = arcTable(spine);
    const Frame f = frameAtArc(spine, cum, s * cum.back());
    const Vec2 n = leftNormal(f.t);
    Poly tick{f.p - n * 14.0, f.p + n * 14.0};
    sc.addPoly(tick, "#e8a33d", 2.0, "2,3", 0.95);
}

std::string caseSvg(const Case &c, const CaseEval &e, const std::string &titleSuffix,
                    const Poly *ghost)
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

    // Departure stubs from Local's outer handle, drawn to the Bezier control (handle/3).
    const BlendSpec &b = e.v[kAC].blend;
    Poly stub0{e.ends.p0, e.ends.p0 + e.ends.f0 * (b.outer0 / 3.0)};
    Poly stub1{e.ends.p1, e.ends.p1 + e.ends.f1 * (b.outer1 / 3.0)};
    sc.addPoly(stub0, "#e8a33d", 1.0, "1,4", 0.8);
    sc.addPoly(stub1, "#e8a33d", 1.0, "1,4", 0.8);

    // Local only: sand band for the blend region. Morph/Cubic have no untouched middle.
    addBlendBand(sc, e.v[kAC].samples, b.t0, true);
    addBlendBand(sc, e.v[kAC].samples, b.t1, false);
    sc.addPoly(e.v[kA].samples, "#8fbde0", 6.0, "", 1.0);
    sc.addPoly(e.v[kAC].samples, "#d62728", 2.2, "", 1.0);
    sc.addPoly(e.v[kCubic].samples, "#2ca02c", 2.0, "", 1.0);
    sc.addPoly(e.v[kMorph].samples, "#ff7f0e", 2.0, "", 1.0);
    addBlendTick(sc, e.v[kAC].spine, b.t0);
    addBlendTick(sc, e.v[kAC].spine, 1.0 - b.t1);

    sc.addArrow(e.ends.p0, e.ends.f0, "#e07b00");
    sc.addArrow(e.ends.p1, e.ends.f1, "#e07b00");
    sc.addDot(e.ends.p0, 4.2, "#111111");
    sc.addDot(e.ends.p1, 4.2, "#111111");

    std::vector<std::string> lines;
    pushWrapped(lines, c.note);
    {
        std::ostringstream l;
        l << "ends: A " << endLabel(e.ends.centre0, e.ends.aligned0) << " obliq "
          << num(e.ends.obliq0, 1) << "deg   B " << endLabel(e.ends.centre1, e.ends.aligned1)
          << " obliq " << num(e.ends.obliq1, 1) << "deg"
          << (e.ends.reversed() ? "   -- U-turn is EXPECTED here (D26); see the W7 section"
                                : "");
        pushWrapped(lines, l.str());
    }
    {
        std::ostringstream l;
        l << "rest " << e.rest.raw.size() << " samples, len " << num(e.rest.spineLen, 1)
          << "u, chord " << num(e.rest.chordLen, 1) << "u   |   warped L' " << num(e.lPrime, 1)
          << "u, scale " << num(e.scale, 3);
        pushWrapped(lines, l.str());
    }
    {
        std::ostringstream l;
        l << "turn " << num(b.turn0, 1) << "/" << num(b.turn1, 1) << "deg   Local blend "
          << num(b.t0, 3) << "/" << num(b.t1, 3) << " of arc" << ((b.capped0 || b.capped1) ? " [CAPPED]" : "")
          << "   Morph m=" << num(e.v[kMorph].mixM, 3) << "   Cubic handle " << num(e.v[kCubic].handle0, 1)
          << "u";
        pushWrapped(lines, l.str());
    }
    {
        std::ostringstream l;
        l << "secant facing deg  Local " << num(e.v[kAC].facingBarDeg, 1) << "  Cubic "
          << num(e.v[kCubic].facingBarDeg, 1) << "  Morph " << num(e.v[kMorph].facingBarDeg, 1)
          << "   analytic  Local " << num(e.v[kAC].analyticFacingDeg, 1) << "  Cubic "
          << num(e.v[kCubic].analyticFacingDeg, 2) << "  Morph " << num(e.v[kMorph].analyticFacingDeg, 1);
        if (e.ends.centre0)
            l << "   (A centre: boundary " << num(e.v[kAC].facingVisibleStartDeg, 1) << "deg)";
        pushWrapped(lines, l.str());
    }
    {
        std::ostringstream l;
        auto cuspLine = [](const VariantEval &ve) {
            return std::to_string(ve.cusps.blend) + "+" + std::to_string(ve.cusps.middle) + " r"
                + (ve.cusps.minRadiusBlend > 9998.0 ? std::string("n/a")
                                                    : num(ve.cusps.minRadiusBlend, 1));
        };
        l << "W5 cusps/minR  Local " << cuspLine(e.v[kAC]) << "  Cubic " << cuspLine(e.v[kCubic])
          << "  Morph " << cuspLine(e.v[kMorph]) << "   backtrack/x " << e.v[kAC].health.total()
          << "/" << e.v[kCubic].health.total() << "/" << e.v[kMorph].health.total()
          << "   inflections Cubic/Morph " << e.v[kCubic].inflections << "/"
          << e.v[kMorph].inflections;
        pushWrapped(lines, l.str());
    }
    {
        std::ostringstream l;
        l << "ink spent vs U: mean/frac>1u  Local " << num(e.v[kAC].spend.meanDev, 2) << "u/"
          << num(100.0 * e.v[kAC].spend.fracMoved1u, 0) << "%  Cubic "
          << num(e.v[kCubic].spend.meanDev, 2) << "u/" << num(100.0 * e.v[kCubic].spend.fracMoved1u, 0)
          << "%  Morph " << num(e.v[kMorph].spend.meanDev, 2) << "u/"
          << num(100.0 * e.v[kMorph].spend.fracMoved1u, 0) << "%";
        if (e.v[kAC].identicalToA && e.v[kMorph].identicalToA)
            l << "   |   Local+Morph bitwise U (I6)";
        else if (e.v[kAC].identicalToA)
            l << "   |   Local bitwise U";
        if (e.v[kAC].visibleShare < 0.999)
            l << "   |   ink kept after clipping " << num(100.0 * e.v[kAC].visibleShare, 0)
              << "%";
        pushWrapped(lines, l.str());
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
        return warpConnector(rs, pa, anchorFacing(a, anchorA, pb, pm), pb,
                             anchorFacing(bBox, anchorB, pa, pm), variant, pm, false);
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

}  // namespace

int main(int argc, char **argv)
{
    const std::string outDir = argc > 1 ? std::string(argv[1]) : std::string("out");
    const std::vector<Case> cases = buildCases();
    const WarpParams def;

    std::vector<Prepared> prep;
    prep.reserve(cases.size());
    for (const Case &c : cases)
        prep.push_back(prepare(c));

    std::cout << "\n=== EXP-0002 round 5 — Local vs Always-cubic vs Morph (SPIKE, host-only) ===\n";
    std::cout << "Local is the round-4 shipped model, not retuned: stub " << num(kStubRatio, 2)
              << ", turn room " << num(kTurnFactor, 1) << ", blend cap " << num(kBlendCap, 2)
              << ", min radius " << num(kMinInkRadius, 1) << " u. No base blend. d absolute, "
                 "never re-bake, pre-blend parameterization, no clamp, variant B gone.\n";
    std::cout << "Always-cubic (EH1): warped spine is a cubic Hermite through the new endpoints "
                 "with the two facings. (s,d) re-placed on that cubic. Not an identity at rest "
                 "unless the rest spine was already that cubic.\n";
    std::cout << "Morph: V = mix(U, C, m) with ONE mix factor for the whole connector. "
                 "m = f(max(turn0, turn1)), turn_i = angle(facing_i, unblended spine tangent at "
                 "that end). Measured at the end itself (no base blend length). m(0)=0 is a true "
                 "skip, bitwise U (D27). Quadratic is mathematically insufficient (one interior "
                 "control cannot hold two independent facings) and is not built.\n";
    std::cout << "Handle default: RestSpeed (Hermite speed = L' of the unblended spine). "
                 "Mix default: world-space lerp. m(turn) form: versine saturating at 90 deg — "
                 "a proposal, not a win; section 4 says whether any form can keep both goals. "
                 "The case set and contact sheet use these.\n";
    std::cout << "Centre facing is D29 (drawn departure, 60 deg cone about the peer ray).\n";

    auto minR = [](const VariantEval &ve) {
        return ve.cusps.minRadiusBlend > 9998.0 ? 9999.0 : ve.cusps.minRadiusBlend;
    };

    // ---- 1. identity -------------------------------------------------------
    std::cout << "\n--- 1. I6 identity at rest (bitwise vs rest-shape reconstruction) ---\n";
    std::cout << "Local and Morph must pass. Always-cubic is expected to fail; the residual is "
                 "the price of EH1.\n";
    std::cout << pad("shape", 8) << pad("rep residual u", 16) << pad("Local vs U", 18)
              << pad("Morph vs U", 18) << pad("Cubic vs U max/mean", 22) << "Cubic frac>1u\n";
    for (size_t i = 0; i < cases.size(); ++i) {
        if (cases[i].scenario != "identity")
            continue;
        const CaseEval e = evaluatePrepared(cases[i], prep[i], def);
        std::cout << pad(cases[i].flavour, 8) << pad(num(e.v[kA].devFromInk, 4), 16)
                  << pad(e.v[kAC].identicalToA ? "bitwise" : num(e.v[kAC].spend.maxDev, 6), 18)
                  << pad(e.v[kMorph].identicalToA ? "bitwise" : num(e.v[kMorph].spend.maxDev, 6),
                         18)
                  << pad(num(e.v[kCubic].spend.maxDev, 3) + "/" + num(e.v[kCubic].spend.meanDev, 3),
                         22)
                  << num(100.0 * e.v[kCubic].spend.fracMoved1u, 0) << "%\n";
    }

    std::cout << "\n--- 1b. identity continuity: 0-8 deg rotation of box A (arc) ---\n";
    std::cout << "A threshold/deadband pops; a continuous mix must not. Cubic has no identity "
                 "to be continuous with.\n";
    std::cout << pad("rot", 7) << pad("turn", 8) << pad("m", 8) << pad("Local mean/max", 16)
              << pad("Morph mean/max", 16) << pad("Cubic mean/max", 16) << "Local/Morph bitwise?\n";
    for (double deg : {0.0, 0.5, 1.0, 2.0, 4.0, 8.0, 15.0}) {
        const Case rc = makeRotationCase("arc", deg);
        const CaseEval e = evaluateCase(rc, def);
        const double turn = std::max(e.v[kAC].blend.turn0, e.v[kAC].blend.turn1);
        std::cout << pad(num(deg, 1), 7) << pad(num(turn, 2), 8) << pad(num(e.v[kMorph].mixM, 3), 8)
                  << pad(num(e.v[kAC].spend.meanDev, 3) + "/" + num(e.v[kAC].spend.maxDev, 3), 16)
                  << pad(num(e.v[kMorph].spend.meanDev, 3) + "/" + num(e.v[kMorph].spend.maxDev, 3),
                         16)
                  << pad(num(e.v[kCubic].spend.meanDev, 3) + "/" + num(e.v[kCubic].spend.maxDev, 3),
                         16)
                  << (e.v[kAC].identicalToA ? "L=U " : "L~ ")
                  << (e.v[kMorph].identicalToA ? "M=U" : "M~") << "\n";
    }

    // ---- 2. handle length --------------------------------------------------
    std::cout << "\n--- 2. cubic handle length: Chord vs ChordThird vs RestSpeed ---\n";
    std::cout << "Chord = Hermite speed |c'| (Bezier controls at chord/3). ChordThird = speed "
                 "|c'|/3. RestSpeed = L' of the unblended spine. Fit at rest is the EH1 identity "
                 "price; the rotated rows ask whether the handle loops, cusps, or S-curves.\n";
    const HandleMode handleModes[3] = {HandleMode::Chord, HandleMode::ChordThird,
                                       HandleMode::RestSpeed};
    const char *handleNames[3] = {"Chord |c'|", "Chord/3", "RestSpeed L'"};
    std::cout << pad("handle", 14) << pad("shape", 8) << pad("rot", 6) << pad("h u", 9)
              << pad("fit max/mean", 14) << pad("minR", 8) << pad("cusps", 7) << pad("back/x", 8)
              << pad("inflect", 9) << "face analytic\n";
    HandleMode pickedHandle = HandleMode::Chord;
    for (int hi = 0; hi < 3; ++hi) {
        WarpParams pm = def;
        pm.handle = handleModes[hi];
        for (const std::string &fl : {std::string("arc"), std::string("wiggle")}) {
            for (int deg : {0, 15, 45, 75, 120}) {
                const Case rc = (deg == 0) ? makeCase(fl, "identity")
                                           : makeRotationCase(fl, static_cast<double>(deg));
                const CaseEval e = evaluateCase(rc, pm);
                std::cout << pad(handleNames[hi], 14) << pad(fl, 8) << pad(std::to_string(deg), 6)
                          << pad(num(e.v[kCubic].handle0, 1), 9)
                          << pad(num(e.v[kCubic].spend.maxDev, 1) + "/"
                                     + num(e.v[kCubic].spend.meanDev, 1),
                                 14)
                          << pad(num(minR(e.v[kCubic]), 1), 8)
                          << pad(std::to_string(e.v[kCubic].cusps.total()), 7)
                          << pad(std::to_string(e.v[kCubic].health.backtracks) + "/"
                                     + std::to_string(e.v[kCubic].health.selfIntersect),
                                 8)
                          << pad(std::to_string(e.v[kCubic].inflections), 9)
                          << num(e.v[kCubic].analyticFacingDeg, 2) << "\n";
            }
        }
    }
    std::cout << "pick (on this table): RestSpeed. It has the lowest rest-fit error on both "
                 "shapes (the EH1 identity price we came to measure): arc 19.2/10.4 vs Chord "
                 "23.7/13.4 vs ChordThird 53.9/37.0. High-turn failure modes are the same as "
                 "Chord (S-curve, then backtrack past ~120 deg of rotation); ChordThird "
                 "under-steers and cusps from 45 deg. Chord's theoretical virtue — it is the "
                 "unique speed that makes a cubic a straight line when both facings already "
                 "align with the chord — does not apply here: the stored facings are 16-39 deg "
                 "off the chord, so that identity is never in play. The case set uses RestSpeed.\n";
    (void)pickedHandle;

    // ---- 3. mix correspondence --------------------------------------------
    std::cout << "\n--- 3. U/C correspondence artefacts at m=0.5 (arc, box A rotated) ---\n";
    std::cout << "World lerp is V=(1-m)U+mC. Turning-angle mixes unwrapped headings then "
                 "similarity-fits onto the endpoints. Opposite-side: sign(mean offset of U from "
                 "the chord) != sign of C. Shortening: len(V) vs (1-m)len(U)+m len(C).\n";
    std::cout << pad("rot", 6) << pad("mix", 12) << pad("sideU", 9) << pad("sideC", 9)
              << pad("opp?", 6) << pad("lenU", 8) << pad("lenC", 8) << pad("lenV", 8)
              << pad("shorten", 9) << pad("minR", 8) << pad("back/x", 8) << "inflect\n";
    for (int deg : {0, 15, 45, 75, 120}) {
        const Case rc = makeRotationCase("arc", static_cast<double>(deg));
        const Prepared pr = prepare(rc);
        for (MixMode mm : {MixMode::WorldLerp, MixMode::TurningAngle}) {
            WarpParams pm = def;
            pm.mix = mm;
            pm.forceMix = (deg == 0) ? 0.0 : 0.5;
            const CaseEval e = evaluatePrepared(rc, pr, pm);
            const Ends ends = e.ends;
            const WarpResult w = warpConnector(e.rest, ends.p0, ends.f0, ends.p1, ends.f1, kMorph,
                                               pm);
            const double sideU = meanSideOfChord(w.spineUnblended);
            const double sideC = meanSideOfChord(w.spineCubic);
            const double lenU = arcTable(w.spineUnblended).back();
            const double lenC = arcTable(w.spineCubic).back();
            const double lenV = arcTable(w.spine).back();
            const double expect = 0.5 * (lenU + lenC);
            const bool opp = (sideU * sideC) < 0.0;
            std::cout << pad(std::to_string(deg), 6)
                      << pad(mm == MixMode::WorldLerp ? "world" : "turning", 12)
                      << pad(num(sideU, 1), 9) << pad(num(sideC, 1), 9)
                      << pad(opp ? "YES" : "no", 6) << pad(num(lenU, 1), 8) << pad(num(lenC, 1), 8)
                      << pad(num(lenV, 1), 8)
                      << pad(expect > 1e-6 ? num(lenV / expect, 3) : std::string("-"), 9)
                      << pad(num(minR(e.v[kMorph]), 1), 8)
                      << pad(std::to_string(e.v[kMorph].health.backtracks) + "/"
                                 + std::to_string(e.v[kMorph].health.selfIntersect),
                             8)
                      << e.v[kMorph].inflections << "\n";
        }
    }
    std::cout << "pick: world lerp. Opposite-side (U and C on different sides of the chord) "
                 "starts around 75 deg of rotation, where versine m is already ~1 so Morph has "
                 "become Cubic and the mix method is idle. At the intermediate m that the mix "
                 "actually runs (15-45 deg) the two spines are on the same side, shortening is "
                 "<3%, and neither mix self-intersects. Turning-angle keeps more radius once "
                 "they go opposite, but it does not equal C at m=1 (similarity-fit of a "
                 "reconstructed polyline) and it is not bitwise U at m=0 without the skip we "
                 "already have. World lerp equals C at m=1 exactly. The case set uses it.\n";

    // ---- 4. m(turn) characterization --------------------------------------
    std::cout << "\n--- 4. m(turn) characterization: Morph at forced m, arc rest shape ---\n";
    std::cout << "turn = max(turn0, turn1), measured at the unblended spine ends (no base blend). "
                 "Each cell is minR / new-cusps / backtrack / mean-dev-from-U / frac>1u.\n";
    const double mGrid[7] = {0.0, 0.1, 0.25, 0.35, 0.5, 0.75, 1.0};
    std::cout << pad("rot", 6) << pad("turn", 8);
    for (double m : mGrid)
        std::cout << pad("m=" + num(m, 2), 28);
    std::cout << "\n";
    for (int deg : {0, 2, 8, 15, 30, 45, 60, 75, 90, 120, 150}) {
        const Case rc = makeRotationCase("arc", static_cast<double>(deg));
        const Prepared pr = prepare(rc);
        const CaseEval probe = evaluatePrepared(rc, pr, def);
        const double turn = std::max(probe.v[kAC].blend.turn0, probe.v[kAC].blend.turn1);
        std::cout << pad(std::to_string(deg), 6) << pad(num(turn, 1), 8);
        for (double m : mGrid) {
            WarpParams pm = def;
            pm.forceMix = m;
            const CaseEval e = evaluatePrepared(rc, pr, pm);
            const VariantEval &ve = e.v[kMorph];
            std::ostringstream cell;
            cell << num(minR(ve), 1) << "/" << ve.cusps.total() << "/" << ve.health.total() << "/"
                 << num(ve.spend.meanDev, 1) << "/" << num(100.0 * ve.spend.fracMoved1u, 0) << "%";
            std::cout << pad(cell.str(), 28);
        }
        std::cout << "\n";
    }
    std::cout << "same grid, wiggle rest shape, selected rotations:\n";
    std::cout << pad("rot", 6) << pad("turn", 8);
    for (double m : mGrid)
        std::cout << pad("m=" + num(m, 2), 28);
    std::cout << "\n";
    for (int deg : {0, 15, 45, 75, 120}) {
        const Case rc = makeRotationCase("wiggle", static_cast<double>(deg));
        const Prepared pr = prepare(rc);
        const CaseEval probe = evaluatePrepared(rc, pr, def);
        const double turn = std::max(probe.v[kAC].blend.turn0, probe.v[kAC].blend.turn1);
        std::cout << pad(std::to_string(deg), 6) << pad(num(turn, 1), 8);
        for (double m : mGrid) {
            WarpParams pm = def;
            pm.forceMix = m;
            const CaseEval e = evaluatePrepared(rc, pr, pm);
            const VariantEval &ve = e.v[kMorph];
            std::ostringstream cell;
            cell << num(minR(ve), 1) << "/" << ve.cusps.total() << "/" << ve.health.total() << "/"
                 << num(ve.spend.meanDev, 1) << "/" << num(100.0 * ve.spend.fracMoved1u, 0) << "%";
            std::cout << pad(cell.str(), 28);
        }
        std::cout << "\n";
    }

    std::cout << "\n--- 4b. candidate m(turn) forms on arc, compared at 15/45/75 deg ---\n";
    std::cout << "linear: m=turn/90. versine: 0.5*(1-cos(pi*turn/90)), m'(0)=0. exp: "
                 "1-exp(-3*turn/90), m(90)~0.95.\n";
    std::cout << pad("form", 10) << pad("rot", 6) << pad("turn", 8) << pad("m", 8)
              << pad("minR", 8) << pad("cusps", 7) << pad("back/x", 8) << pad("mean u", 9)
              << pad("frac>1u", 9) << "analytic face\n";
    for (int form : {0, 1, 2}) {
        const char *fn = form == 0 ? "linear" : (form == 1 ? "versine" : "exp");
        WarpParams pm = def;
        pm.morphForm = form;
        pm.morphSatDeg = 90.0;
        for (int deg : {0, 2, 15, 45, 75, 90}) {
            const Case rc = makeRotationCase("arc", static_cast<double>(deg));
            const CaseEval e = evaluateCase(rc, pm);
            const double turn = std::max(e.v[kAC].blend.turn0, e.v[kAC].blend.turn1);
            std::cout << pad(fn, 10) << pad(std::to_string(deg), 6) << pad(num(turn, 1), 8)
                      << pad(num(e.v[kMorph].mixM, 3), 8) << pad(num(minR(e.v[kMorph]), 1), 8)
                      << pad(std::to_string(e.v[kMorph].cusps.total()), 7)
                      << pad(std::to_string(e.v[kMorph].health.total()), 8)
                      << pad(num(e.v[kMorph].spend.meanDev, 2), 9)
                      << pad(num(100.0 * e.v[kMorph].spend.fracMoved1u, 0) + "%", 9)
                      << num(e.v[kMorph].analyticFacingDeg, 1) << "\n";
        }
    }
    std::cout << "shipped form for the sheet: versine, sat 90 deg. It is continuous at 0 "
                 "(section 1b: 0.002 u at 0.5 deg, no pop). It does NOT satisfy both product "
                 "goals: see section 7. No other form in this table does either. Linear at 15 deg "
                 "already spends most of the line; exp spends it faster; versine is the slowest "
                 "start and still moves 71% of samples >1 u at 15 deg of rotation because C is "
                 "~19 u away from U at that pose. A form slow enough to keep ink at 15 deg "
                 "(m ~ 0.002) is still ~0.04 at 75 deg and is not a cubic. That is the conflict "
                 "this round exists to measure, not a constant to retune.\n";

    // ---- 5. case set ------------------------------------------------------
    std::vector<CaseEval> evals;
    evals.reserve(cases.size());
    for (size_t i = 0; i < cases.size(); ++i)
        evals.push_back(evaluatePrepared(cases[i], prep[i], def));

    std::cout << "\n--- 5. the " << cases.size()
              << "-case set, Local / Cubic / Morph side by side ---\n";
    std::cout << pad("case", 26) << pad("class", 10) << pad("turn", 8) << pad("m", 7)
              << pad("L cusp/r", 12) << pad("C cusp/r", 12) << pad("M cusp/r", 12)
              << pad("L/C/M back", 12) << pad("L mean", 8) << pad("C mean", 8) << pad("M mean", 8)
              << "L/C/M face-an\n";
    for (const CaseEval &e : evals) {
        const double turn = std::max(e.v[kAC].blend.turn0, e.v[kAC].blend.turn1);
        auto cr = [&](const VariantEval &ve) {
            return std::to_string(ve.cusps.total()) + "/" + num(minR(ve), 1);
        };
        std::cout << pad(e.name, 26) << pad(e.ends.reversed() ? "REVERSED" : "aligned", 10)
                  << pad(num(turn, 1), 8) << pad(num(e.v[kMorph].mixM, 2), 7)
                  << pad(cr(e.v[kAC]), 12) << pad(cr(e.v[kCubic]), 12) << pad(cr(e.v[kMorph]), 12)
                  << pad(std::to_string(e.v[kAC].health.total()) + "/"
                             + std::to_string(e.v[kCubic].health.total()) + "/"
                             + std::to_string(e.v[kMorph].health.total()),
                         12)
                  << pad(num(e.v[kAC].spend.meanDev, 2), 8) << pad(num(e.v[kCubic].spend.meanDev, 2), 8)
                  << pad(num(e.v[kMorph].spend.meanDev, 2), 8)
                  << num(e.v[kAC].analyticFacingDeg, 1) << "/"
                  << num(e.v[kCubic].analyticFacingDeg, 1) << "/"
                  << num(e.v[kMorph].analyticFacingDeg, 1) << "\n";
    }

    // ---- 6. restated bars -------------------------------------------------
    std::cout << "\n--- 6. restated bars, all three variants ---\n";
    auto scoreBars = [&](int v, const char *name) {
        int idN = 0, idFail = 0;
        double worstRep = 0.0, cubicResid = 0.0;
        for (size_t i = 0; i < cases.size(); ++i) {
            if (cases[i].scenario != "identity")
                continue;
            ++idN;
            worstRep = std::max(worstRep, evals[i].v[kA].devFromInk);
            cubicResid = std::max(cubicResid, evals[i].v[kCubic].spend.maxDev);
            if (v != kCubic && !evals[i].v[static_cast<size_t>(v)].identicalToA)
                ++idFail;
        }
        int cuspN = 0, cuspFail = 0, overN = 0, overFail = 0;
        int unscoreable = 0, forcedBT = 0;
        double maxTurn = 0.0;
        std::string cuspNames, overNames;
        for (const CaseEval &e : evals) {
            if (e.ends.reversed())
                continue;
            if (e.lPrime < kScoreableArcWorld) {
                ++unscoreable;
                continue;
            }
            ++cuspN;
            maxTurn = std::max(maxTurn, std::max(e.v[kAC].blend.turn0, e.v[kAC].blend.turn1));
            if (e.v[static_cast<size_t>(v)].cusps.total() != 0) {
                ++cuspFail;
                cuspNames += (cuspNames.empty() ? "" : ", ") + e.name;
            }
            const double chordMax = std::max(e.v[kAC].blend.chord0, e.v[kAC].blend.chord1);
            const int ht = e.v[static_cast<size_t>(v)].health.total();
            if (chordMax >= 90.0) {
                if (ht != 0)
                    ++forcedBT;
            } else {
                ++overN;
                if (ht != 0) {
                    ++overFail;
                    overNames += (overNames.empty() ? "" : ", ") + e.name;
                }
            }
        }
        std::cout << name << "\n";
        if (v == kCubic)
            std::cout << "  I6  identity at rest: EXPECTED FAIL, residual vs U max "
                      << num(cubicResid, 3) << " u (the EH1 price). rep residual vs raw ink "
                      << num(worstRep, 4) << " u\n";
        else
            std::cout << "  I6  identity at rest, bitwise vs U: " << idFail << " failing of "
                      << idN << " -> " << (idFail == 0 ? "PASS" : "FAIL") << "\n";
        std::cout << "  W5  zero new cusps, aligned, L'>=24u: " << cuspFail << " failing of "
                  << cuspN << (cuspFail ? " (" + cuspNames + ")" : "") << " -> "
                  << (cuspFail == 0 ? "PASS" : "FAIL") << "  (turn to " << num(maxTurn, 1)
                  << " deg, " << unscoreable << " unscoreable)\n";
        std::cout << "  W5b zero backtrack where facing faces the chord: " << overFail
                  << " failing of " << overN << (overFail ? " (" + overNames + ")" : "") << " -> "
                  << (overFail == 0 ? "PASS" : "FAIL") << "  forced-BT-region still unhealthy: "
                  << forcedBT << "\n";
    };
    scoreBars(kAC, "Local");
    scoreBars(kCubic, "Always-cubic");
    scoreBars(kMorph, "Morph");

    std::cout << "D30 analytic facing: Cubic is 0 by construction. Morph analytic is the angle "
                 "between (1-m)U' + m C' and the facing — it inherits Local's mismatch at low m "
                 "and buys facing by spending ink. Local is 0 when a blend fires, else the "
                 "unblended mismatch (which is 0 at rest).\n";

    // ---- 7. ink spent at 15/45/75 -----------------------------------------
    std::cout << "\n--- 7. adaptation / straighten cost at 15 / 45 / 75 deg box rotation ---\n";
    std::cout << "unit is whole-line mean deviation from U, and fraction of samples moved >1 u. "
                 "Blend share is the wrong unit for a global morph.\n";
    std::cout << pad("shape", 8) << pad("rot", 6) << pad("turn", 8) << pad("m", 7)
              << pad("L mean/%", 14) << pad("C mean/%", 14) << pad("M mean/%", 14)
              << pad("L minR", 8) << pad("C minR", 8) << pad("M minR", 8)
              << "L/C/M cusp\n";
    for (const std::string &fl : {std::string("arc"), std::string("wiggle")}) {
        for (int deg : {0, 15, 45, 75}) {
            const Case rc = makeRotationCase(fl, static_cast<double>(deg));
            const CaseEval e = evaluateCase(rc, def);
            const double turn = std::max(e.v[kAC].blend.turn0, e.v[kAC].blend.turn1);
            auto sp = [](const VariantEval &ve) {
                return num(ve.spend.meanDev, 2) + "/" + num(100.0 * ve.spend.fracMoved1u, 0) + "%";
            };
            std::cout << pad(fl, 8) << pad(std::to_string(deg), 6) << pad(num(turn, 1), 8)
                      << pad(num(e.v[kMorph].mixM, 2), 7) << pad(sp(e.v[kAC]), 14)
                      << pad(sp(e.v[kCubic]), 14) << pad(sp(e.v[kMorph]), 14)
                      << pad(num(minR(e.v[kAC]), 1), 8) << pad(num(minR(e.v[kCubic]), 1), 8)
                      << pad(num(minR(e.v[kMorph]), 1), 8) << e.v[kAC].cusps.total() << "/"
                      << e.v[kCubic].cusps.total() << "/" << e.v[kMorph].cusps.total() << "\n";
        }
    }

    // ---- 8. forced backtrack ----------------------------------------------
    std::cout << "\n--- 8. forced backtrack >~90 deg: can a cubic hold opposing facings? ---\n";
    std::cout << "Local cannot (round 4: geometry, not tuning). A cubic can loop or S-curve. "
                 "chord-flip is the case; also the high-rotation rows.\n";
    std::cout << pad("case", 26) << pad("obliq", 8) << pad("L back/x/r", 14)
              << pad("C back/x/r/inf", 18) << pad("M back/x/r/inf", 18) << "C mean vs U\n";
    for (size_t i = 0; i < cases.size(); ++i) {
        if (cases[i].scenario != "chord-flip" && cases[i].scenario != "rotate-a")
            continue;
        const CaseEval &e = evals[i];
        auto hx = [&](const VariantEval &ve) {
            return std::to_string(ve.health.backtracks) + "/"
                + std::to_string(ve.health.selfIntersect) + "/" + num(minR(ve), 1);
        };
        std::cout << pad(e.name, 26) << pad(num(e.ends.obliquity(), 1), 8)
                  << pad(hx(e.v[kAC]), 14)
                  << pad(hx(e.v[kCubic]) + "/" + std::to_string(e.v[kCubic].inflections), 18)
                  << pad(hx(e.v[kMorph]) + "/" + std::to_string(e.v[kMorph].inflections), 18)
                  << num(e.v[kCubic].spend.meanDev, 1) << "\n";
    }
    for (int deg : {90, 105, 120, 150, 180}) {
        for (const std::string &fl : {std::string("arc")}) {
            const Case rc = makeRotationCase(fl, static_cast<double>(deg));
            const CaseEval e = evaluateCase(rc, def);
            auto hx = [&](const VariantEval &ve) {
                return std::to_string(ve.health.backtracks) + "/"
                    + std::to_string(ve.health.selfIntersect) + "/" + num(minR(ve), 1);
            };
            std::cout << pad(e.name, 26) << pad(num(e.ends.obliquity(), 1), 8)
                      << pad(hx(e.v[kAC]), 14)
                      << pad(hx(e.v[kCubic]) + "/" + std::to_string(e.v[kCubic].inflections), 18)
                      << pad(hx(e.v[kMorph]) + "/" + std::to_string(e.v[kMorph].inflections), 18)
                      << num(e.v[kCubic].spend.meanDev, 1) << "\n";
        }
    }

    // ---- 9. short connectors ----------------------------------------------
    std::cout << "\n--- 9. short connectors: does a global cubic remove the ~50 u floor? ---\n";
    std::cout << pad("case", 26) << pad("L' u", 8) << pad("turn", 8) << pad("L minR/cusp", 14)
              << pad("C minR/cusp", 14) << pad("M minR/cusp", 14) << "scoreable?\n";
    const double gapSweep[6] = {100.0, 110.0, 125.0, 150.0, 190.0, 280.0};
    for (double gap : gapSweep) {
        for (int deg : {0, 15, 45}) {
            const Case sc = makeShortCase("arc", gap, static_cast<double>(deg));
            const CaseEval e = evaluateCase(sc, def);
            const double turn = std::max(e.v[kAC].blend.turn0, e.v[kAC].blend.turn1);
            auto rc = [&](const VariantEval &ve) {
                return num(minR(ve), 1) + "/" + std::to_string(ve.cusps.total());
            };
            std::cout << pad(e.name, 26) << pad(num(e.lPrime, 1), 8) << pad(num(turn, 1), 8)
                      << pad(rc(e.v[kAC]), 14) << pad(rc(e.v[kCubic]), 14) << pad(rc(e.v[kMorph]), 14)
                      << (e.lPrime < kScoreableArcWorld ? "no" : "yes") << "\n";
        }
    }

    // ---- 10. W7 ------------------------------------------------------------
    std::cout << "\n--- 10. W7 centre-bind escape hatch (D29 60 deg cone in force) ---\n";
    std::cout << pad("case", 26) << pad("var", 8) << pad("cusps", 8) << pad("overshoot", 11)
              << pad("ink kept", 10) << "verdict\n";
    std::vector<std::pair<std::string, std::string>> w7Svgs;
    bool w7Pass = true;
    std::vector<size_t> reversedIdx;
    for (size_t i = 0; i < cases.size(); ++i) {
        if (resolveEnds(cases[i], prep[i], def, false, false).reversed())
            reversedIdx.push_back(i);
    }
    for (size_t idx : reversedIdx) {
        const Ends orig = resolveEnds(cases[idx], prep[idx], def, false, false);
        const CaseEval ce = evaluatePrepared(cases[idx], prep[idx], def, !orig.aligned0,
                                             !orig.aligned1);
        for (int v : {kAC, kCubic, kMorph}) {
            const VariantEval &ve = ce.v[static_cast<size_t>(v)];
            const bool ok = ve.cusps.total() == 0 && ve.health.total() == 0;
            if (v == kAC && !ok)
                w7Pass = false;
            std::cout << pad(cases[idx].name, 26) << pad(variantName(v), 8)
                      << pad(std::to_string(ve.cusps.total()), 8)
                      << pad(std::to_string(ve.health.total()), 11)
                      << pad(num(100.0 * ve.visibleShare, 0) + "%", 10) << (ok ? "PASS" : "FAIL")
                      << "\n";
        }
        const Poly ghost = evals[idx].v[kAC].samples;
        w7Svgs.emplace_back(cases[idx].name,
                            caseSvg(cases[idx], ce, "  [W7: converted to centre bind]", &ghost));
    }
    std::cout << "W7 Local (the remedy the policy needs): " << (w7Pass ? "PASS" : "FAIL") << "\n";

    // ---- 11. W2 / W3 / W6 -------------------------------------------------
    struct RtRow {
        std::string label;
        std::array<RoundTrip, kVariantCount> rt;
    };
    std::vector<RtRow> rtRows;
    for (const std::string &flavour : {std::string("arc"), std::string("wiggle")}) {
        const Case base = makeCase(flavour, "translate-near");
        const Prepared bp = prepare(base);
        RtRow row;
        row.label = flavour;
        for (int v = 0; v < kVariantCount; ++v)
            row.rt[static_cast<size_t>(v)] = roundTripDrift(bp.rest, base.a0, base.b0, bp.a0,
                                                            bp.a1, v, def);
        rtRows.push_back(row);
    }
    std::cout << "\n--- 11. W2 round-trip drift ---\n";
    std::cout << pad("rest shape", 12) << pad("variant", 9) << pad("never-re-bake (D5)", 22)
              << pad("re-bake, 20 moves", 20) << "re-bake, 200 moves\n";
    double worstPure = 0.0;
    for (const RtRow &r : rtRows) {
        for (int v = 0; v < kVariantCount; ++v) {
            const RoundTrip &rt = r.rt[static_cast<size_t>(v)];
            worstPure = std::max(worstPure, rt.pure);
            std::cout << pad(r.label, 12) << pad(variantName(v), 9)
                      << pad(num(rt.pure, 9) + " u", 22) << pad(num(rt.rebake1, 4) + " u", 20)
                      << num(rt.rebake10, 4) << " u\n";
        }
    }
    std::cout << "W2 never-re-bake max " << num(worstPure, 9) << " u -> "
              << (worstPure <= 1.0 ? "PASS" : "FAIL") << "\n";

    bool doubleRunOk = true;
    std::string doubleRunMsg = "-";
    for (size_t i = 0; i < cases.size() && doubleRunOk; ++i) {
        const CaseEval a = evaluateCase(cases[i], def);
        const CaseEval b = evaluateCase(cases[i], def);
        for (int v = 0; v < kVariantCount; ++v) {
            if (!bitIdenticalPoly(a.v[static_cast<size_t>(v)].samples,
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
            if (!bitIdenticalPoly(e.v[static_cast<size_t>(v)].samples,
                                  evals[i].v[static_cast<size_t>(v)].samples)) {
                reverseOk = false;
                reverseMsg = e.name + " variant " + variantName(v);
                break;
            }
        }
        if (!reverseOk)
            break;
    }
    std::cout << "\n--- W3 determinism ---\n";
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

    // ---- contact sheet ----------------------------------------------------
    std::vector<std::pair<std::string, std::string>> identitySvgs;
    std::vector<std::pair<std::string, std::string>> sweepSvgs;
    std::vector<std::pair<std::string, std::string>> svgs;
    for (size_t i = 0; i < cases.size(); ++i) {
        const std::string svg = caseSvg(cases[i], evals[i], "", nullptr);
        std::ofstream f(outDir + "/" + fileNameFor(cases[i].name) + ".svg");
        if (!f) {
            std::cerr << "cannot write " << outDir << "\n";
            return 1;
        }
        f << svg;
        if (cases[i].scenario == "identity")
            identitySvgs.emplace_back(cases[i].name, svg);
        else
            svgs.emplace_back(cases[i].name, svg);
    }
    for (const std::string &fl : {std::string("arc")}) {
        for (int deg : {0, 15, 45, 75}) {
            const Case rc = makeRotationCase(fl, static_cast<double>(deg));
            const CaseEval e = evaluateCase(rc, def);
            const std::string svg = caseSvg(rc, e, "", nullptr);
            const std::string fn = "sweep_" + fl + "_" + std::to_string(deg);
            std::ofstream f(outDir + "/" + fn + ".svg");
            f << svg;
            sweepSvgs.emplace_back(rc.name, svg);
        }
    }
    for (const auto &pair : w7Svgs) {
        std::ofstream f(outDir + "/" + fileNameFor(pair.first) + "__centre.svg");
        f << pair.second;
    }

    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset=\"utf-8\">\n";
    html << "<title>EXP-0002 round 5 — Local vs Cubic vs Morph</title>\n";
    html << "<style>body{font:14px/1.5 -apple-system,Helvetica,Arial;margin:32px;color:#111;"
            "background:#fafafa}h1{font-size:22px}h2{font-size:16px;margin-top:38px;"
            "border-top:1px solid #ddd;padding-top:14px}h3{font-size:15px;margin-top:26px}"
            "svg{background:#fff;border:1px solid #e3e3e3;border-radius:6px;max-width:100%;"
            "height:auto}table{border-collapse:collapse;font:12px/1.4 ui-monospace,Menlo,"
            "monospace;margin:14px 0}td,th{border:1px solid #ddd;padding:4px 9px;"
            "text-align:left}th{background:#f0f0f0}.note{color:#555;margin:6px 0 10px}"
            ".rev{background:#fff6e5}</style></head><body>\n";
    html << "<h1>EXP-0002 round 5 — Local vs Always-cubic vs Morph</h1>\n";
    html << "<p class=\"note\"><b>W1 is a human verdict.</b> Grey = ink as drawn. Pale blue halo "
            "= that ink carried rigidly (unblended A). <b style=\"color:#d62728\">Red = Local</b> "
            "(round-4 end blend). <b style=\"color:#2ca02c\">Green = Always-cubic</b>. "
            "<b style=\"color:#ff7f0e\">Orange = Morph</b> at the proposed m(turn). Sand band is "
            "Local's blend region only — Morph and Always-cubic have no untouched middle, so "
            "there is no band to draw for them.</p>\n";
    html << "<p class=\"note\">Read the two <code>identity</code> panels first: Local and Morph "
            "must sit on the halo (I6). Always-cubic will not — that residual is the price of "
            "replacing the spine with a cubic when nothing has turned. Then the rotation sweep: "
            "the question is how each spends ink as the turn grows.</p>\n";
    html << "<p class=\"note\">Local constants unchanged from round 4. Cubic handle = rest-spine "
            "end speed L' (picked over chord/3 on rest-fit). Morph m = versine(turn / 90°), one "
            "factor for the whole connector, driven by max(turn0, turn1) at the unblended ends. "
            "That mix cannot keep both “small nudge looks like ink” and “hard turn looks like a "
            "cubic” — read the 15° vs 75° sweep. Sweeps in "
            "<a href=\"report.txt\">report.txt</a>.</p>\n";

    html << "<h2>Identity at rest</h2>\n";
    for (const auto &pair : identitySvgs)
        html << "<h3>" << esc(pair.first) << "</h3>\n" << pair.second;

    html << "<h2>Rotation sweep (arc) — 0° / 15° / 45° / 75°</h2>\n";
    html << "<p class=\"note\">Same rest ink, box A rotated. All three overlays. This is the "
            "picture the m(turn) question is about.</p>\n";
    for (const auto &pair : sweepSvgs)
        html << "<h3>" << esc(pair.first) << "</h3>\n" << pair.second;

    html << "<h2>Summary at the proposed defaults</h2>\n<table><tr><th>case</th><th>class</th>"
            "<th>turn</th><th>m</th><th>Local cusp/r</th><th>Cubic cusp/r</th>"
            "<th>Morph cusp/r</th><th>L/C/M mean vs U</th></tr>\n";
    for (const CaseEval &e : evals) {
        const double turn = std::max(e.v[kAC].blend.turn0, e.v[kAC].blend.turn1);
        auto cr = [&](const VariantEval &ve) {
            return std::to_string(ve.cusps.total()) + " / " + num(minR(ve), 1);
        };
        html << "<tr" << (e.ends.reversed() ? " class=\"rev\"" : "") << "><td>" << esc(e.name)
             << "</td><td>" << (e.ends.reversed() ? "REVERSED" : "aligned") << "</td><td>"
             << num(turn, 1) << "</td><td>" << num(e.v[kMorph].mixM, 2) << "</td><td>"
             << cr(e.v[kAC]) << "</td><td>" << cr(e.v[kCubic]) << "</td><td>" << cr(e.v[kMorph])
             << "</td><td>" << num(e.v[kAC].spend.meanDev, 2) << " / "
             << num(e.v[kCubic].spend.meanDev, 2) << " / " << num(e.v[kMorph].spend.meanDev, 2)
             << "</td></tr>\n";
    }
    html << "</table>\n";

    html << "<h2>Cases (identity already shown above)</h2>\n";
    for (const auto &pair : svgs)
        html << "<h3>" << esc(pair.first) << "</h3>\n" << pair.second;

    html << "<h2>W7 — centre-bind escape hatch</h2>\n";
    html << "<p class=\"note\">Reversed end(s) switched to centre. Pink dotted = the edge-bind "
            "Local result. D29 60° cone is in force.</p>\n";
    for (const auto &pair : w7Svgs)
        html << "<h3>" << esc(pair.first) << " — centre bind</h3>\n" << pair.second;

    html << "<h2>W2 / W6</h2><table><tr><th>rest</th><th>variant</th><th>never-re-bake</th>"
            "<th>re-bake 20</th><th>re-bake 200</th></tr>\n";
    for (const RtRow &r : rtRows) {
        for (int v = 0; v < kVariantCount; ++v)
            html << "<tr><td>" << r.label << "</td><td>" << variantName(v) << "</td><td>"
                 << num(r.rt[static_cast<size_t>(v)].pure, 9) << " u</td><td>"
                 << num(r.rt[static_cast<size_t>(v)].rebake1, 4) << " u</td><td>"
                 << num(r.rt[static_cast<size_t>(v)].rebake10, 4) << " u</td></tr>\n";
    }
    html << "</table>\n<table><tr><th>variant</th><th>p50 us</th><th>p95 us</th></tr>\n";
    for (int v = 0; v < kVariantCount; ++v)
        html << "<tr><td>" << variantName(v) << "</td><td>"
             << num(timings[static_cast<size_t>(v)].p50, 2) << "</td><td>"
             << num(timings[static_cast<size_t>(v)].p95, 2) << "</td></tr>\n";
    html << "</table>\n</body></html>\n";

    std::ofstream fh(outDir + "/index.html");
    if (!fh) {
        std::cerr << "cannot write index.html\n";
        return 1;
    }
    fh << html.str();
    std::cout << "\nwrote " << (identitySvgs.size() + svgs.size()) << " case SVGs + "
              << sweepSvgs.size() << " sweep SVGs + " << w7Svgs.size()
              << " W7 SVGs + index.html to " << outDir << "\n";
    return 0;
}
