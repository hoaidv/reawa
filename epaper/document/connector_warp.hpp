#pragma once
/**
 * Morph (Ink) and Always-cubic (Curve) warp — pure function of rest + ends.
 * @implements [SRS-EP-18] similarity U, Hermite C, Morph skip at m=0, centre clip
 * @implements [ADR-0020] I1 never re-bake; I3 Morph identity at rest
 */

#include "connector_warp_params.hpp"
#include "device_document.hpp"
#include "recognize_enclose.hpp"
#include "rest_shape.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace epaper {
namespace document {

constexpr double kWarpPi = 3.14159265358979323846;

struct WarpEnd {
    RestVec p{};
    RestVec f{1, 0};
    bool centre = false;
    Aabb clip{};
    bool hasClip = false;
};

struct WarpResult {
    std::vector<RestVec> spine;
    std::vector<RestVec> samples;
    double mixM = 0;
    double lPrime = 0;
};

inline bool resolveConnectorEnds(const DeviceDocument &doc, const DocNode &conn, WarpEnd *from,
                                 WarpEnd *to);

inline RestVec warpAdd(RestVec a, RestVec b) { return {a.x + b.x, a.y + b.y}; }
inline RestVec warpSub(RestVec a, RestVec b) { return {a.x - b.x, a.y - b.y}; }
inline RestVec warpMul(RestVec a, double k) { return {a.x * k, a.y * k}; }
inline double warpDot(RestVec a, RestVec b) { return a.x * b.x + a.y * b.y; }
inline double warpLen(RestVec a) { return std::hypot(a.x, a.y); }
inline RestVec warpUnit(RestVec a)
{
    const double l = warpLen(a);
    return l > 1e-12 ? RestVec{a.x / l, a.y / l} : RestVec{1, 0};
}
inline RestVec warpLeft(RestVec t) { return {-t.y, t.x}; }
inline RestVec warpRot(RestVec v, double th)
{
    const double c = std::cos(th);
    const double s = std::sin(th);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}
inline double warpAngleDeg(RestVec a, RestVec b)
{
    const RestVec ua = warpUnit(a);
    const RestVec ub = warpUnit(b);
    double c = warpDot(ua, ub);
    c = std::max(-1.0, std::min(1.0, c));
    return std::acos(c) * 180.0 / kWarpPi;
}
inline double warpSignedDeg(RestVec a, RestVec b)
{
    const RestVec ua = warpUnit(a);
    const RestVec ub = warpUnit(b);
    return std::atan2(ua.x * ub.y - ua.y * ub.x, warpDot(ua, ub)) * 180.0 / kWarpPi;
}

inline std::vector<double> warpArcTable(const std::vector<RestVec> &p)
{
    std::vector<double> cum(p.size(), 0);
    for (size_t i = 1; i < p.size(); ++i)
        cum[i] = cum[i - 1] + warpLen(warpSub(p[i], p[i - 1]));
    return cum;
}

inline RestVec warpSegTangent(const std::vector<RestVec> &p, size_t i)
{
    const size_t n = p.size();
    if (n < 2)
        return {1, 0};
    for (size_t j = i + 1; j < n; ++j) {
        const RestVec d = warpSub(p[j], p[i]);
        if (warpDot(d, d) > 1e-24)
            return warpUnit(d);
    }
    for (size_t j = i; j-- > 0;) {
        const RestVec d = warpSub(p[i], p[j]);
        if (warpDot(d, d) > 1e-24)
            return warpUnit(d);
    }
    return {1, 0};
}

inline RestVec warpHermite(RestVec p0, RestVec m0, RestVec p1, RestVec m1, double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    return warpAdd(warpAdd(warpMul(p0, 2 * t3 - 3 * t2 + 1), warpMul(m0, t3 - 2 * t2 + t)),
                   warpAdd(warpMul(p1, -2 * t3 + 3 * t2), warpMul(m1, t3 - t2)));
}

inline std::vector<RestVec> warpSimilarity(const std::vector<RestVec> &spine, RestVec p0, RestVec p1)
{
    std::vector<RestVec> out(spine.size());
    if (spine.empty())
        return out;
    const RestVec chord = warpSub(spine.back(), spine.front());
    const RestVec cNew = warpSub(p1, p0);
    const double lOld = warpLen(chord);
    const double lNew = warpLen(cNew);
    const double scale = lOld > 1e-9 ? lNew / lOld : 1.0;
    const double th = (lNew > 1e-9 && lOld > 1e-9)
        ? std::atan2(cNew.y, cNew.x) - std::atan2(chord.y, chord.x)
        : 0.0;
    const RestVec o = spine.front();
    for (size_t i = 0; i < spine.size(); ++i)
        out[i] = warpAdd(p0, warpRot(warpMul(warpSub(spine[i], o), scale), th));
    return out;
}

inline std::vector<RestVec> warpCubicAtU(const std::vector<RestVec> &U, RestVec p0, RestVec f0,
                                         RestVec p1, RestVec f1, double h0, double h1)
{
    if (U.size() < 2)
        return U;
    const std::vector<double> cum = warpArcTable(U);
    const double total = cum.back();
    const RestVec m0 = warpMul(f0, h0);
    const RestVec m1 = warpMul(f1, -h1);
    std::vector<RestVec> C(U.size());
    for (size_t i = 0; i < U.size(); ++i) {
        const double s = total > 1e-12 ? cum[i] / total : 0;
        C[i] = warpHermite(p0, m0, p1, m1, s);
    }
    C.front() = p0;
    C.back() = p1;
    return C;
}

inline std::vector<RestVec> placeOnSpine(const std::vector<RestVec> &param,
                                         const std::vector<RestVec> &geom,
                                         const std::vector<RestOffset> &sd)
{
    const std::vector<double> cum = warpArcTable(param);
    const double total = cum.empty() ? 0.0 : cum.back();
    std::vector<RestVec> out(sd.size());
    for (size_t i = 0; i < sd.size(); ++i) {
        const double arc = sd[i].s * total;
        size_t lo = 0;
        double u = 0;
        if (param.size() >= 2 && total > 1e-12) {
            if (arc >= total) {
                lo = param.size() - 2;
                u = 1;
            } else if (arc > 0) {
                size_t hi = param.size() - 1;
                while (lo + 1 < hi) {
                    const size_t mid = (lo + hi) / 2;
                    if (cum[mid] <= arc)
                        lo = mid;
                    else
                        hi = mid;
                }
                const double segLen = cum[lo + 1] - cum[lo];
                u = segLen > 1e-12 ? (arc - cum[lo]) / segLen : 0;
            }
        }
        RestVec p = geom.empty() ? RestVec{} : geom[0];
        RestVec t{1, 0};
        if (geom.size() >= 2) {
            p = warpAdd(geom[lo], warpMul(warpSub(geom[lo + 1], geom[lo]), u));
            t = warpSegTangent(geom, lo);
        }
        out[i] = warpAdd(p, warpMul(warpLeft(t), sd[i].d));
    }
    return out;
}

inline bool aabbContains(const Aabb &b, RestVec p)
{
    return p.x >= b.minX && p.x <= b.maxX && p.y >= b.minY && p.y <= b.maxY;
}

inline RestVec aabbExit(RestVec inside, RestVec outside, const Aabb &b)
{
    RestVec lo = inside;
    RestVec hi = outside;
    for (int i = 0; i < 48; ++i) {
        const RestVec mid = warpMul(warpAdd(lo, hi), 0.5);
        if (aabbContains(b, mid))
            lo = mid;
        else
            hi = mid;
    }
    return hi;
}

inline void clipCentreEnds(std::vector<RestVec> &pts, const WarpEnd &e0, const WarpEnd &e1)
{
    if (e0.centre && e0.hasClip) {
        size_t n = 0;
        while (n < pts.size() && aabbContains(e0.clip, pts[n]))
            ++n;
        if (n > 0 && n < pts.size()) {
            const RestVec hit = aabbExit(pts[n - 1], pts[n], e0.clip);
            pts.erase(pts.begin(), pts.begin() + static_cast<std::ptrdiff_t>(n));
            pts.front() = hit;
        }
    }
    if (e1.centre && e1.hasClip) {
        size_t n = 0;
        while (n < pts.size() && aabbContains(e1.clip, pts[pts.size() - 1 - n]))
            ++n;
        if (n > 0 && n < pts.size()) {
            const size_t keep = pts.size() - n;
            const RestVec hit = aabbExit(pts[keep], pts[keep - 1], e1.clip);
            pts.resize(keep);
            pts.back() = hit;
        }
    }
}

inline double morphMixFromTurn(double turnDeg)
{
    if (!(turnDeg > 0.0))
        return 0.0;
    const double sat = kMorphSatDeg > 1e-9 ? kMorphSatDeg : 90.0;
    const double u = std::min(1.0, turnDeg / sat);
    return 0.5 * (1.0 - std::cos(kWarpPi * u));
}

/**
 * Reconstruct rest samples from S + (s,d). Morph at m=0 must match this bitwise.
 */
inline std::vector<RestVec> restShapeReconstruction(const RestShape &rs)
{
    return placeOnSpine(rs.spine, rs.spine, rs.offsets);
}

/** @implements [SRS-EP-18] Morph skip when m=0; Cubic V=C; d never scaled */
inline WarpResult warpConnector(const RestShape &rs, WarpEnd e0, WarpEnd e1, const std::string &style)
{
    WarpResult out;
    if (rs.spine.size() < 2)
        return out;
    const std::vector<RestVec> U = warpSimilarity(rs.spine, e0.p, e1.p);
    out.lPrime = warpArcTable(U).back();
    const double chord = warpLen(warpSub(e1.p, e0.p));
    const double h = kHandleModeRestSpeed ? (out.lPrime > 1e-12 ? out.lPrime : chord) : chord / 3.0;
    const std::vector<RestVec> C = warpCubicAtU(U, e0.p, e0.f, e1.p, e1.f, h, h);
    const RestVec t0 = warpSegTangent(U, 0);
    const RestVec t1 = warpSegTangent(U, U.size() >= 2 ? U.size() - 2 : 0);
    const double turn = std::max(warpAngleDeg(e0.f, t0), warpAngleDeg(warpMul(e1.f, -1.0), t1));
    out.mixM = morphMixFromTurn(turn);
    const bool cubic = style == "cubic";
    if (cubic) {
        out.spine = C;
    } else if (!(out.mixM > 0.0)) {
        out.spine = U;
    } else if (out.mixM >= 1.0) {
        out.spine = C;
    } else {
        out.spine.resize(U.size());
        const double om = 1.0 - out.mixM;
        for (size_t i = 0; i < U.size(); ++i)
            out.spine[i] = warpAdd(warpMul(U[i], om), warpMul(C[i], out.mixM));
        if (!out.spine.empty()) {
            out.spine.front() = e0.p;
            out.spine.back() = e1.p;
        }
    }
    out.samples = placeOnSpine(U, out.spine, rs.offsets);
    clipCentreEnds(out.samples, e0, e1);
    return out;
}

inline RestShape restShapeFromNode(const DocNode &n)
{
    RestShape r;
    r.warpStyle = n.warpStyle.empty() ? "morph" : n.warpStyle;
    r.spine.reserve(n.restSpine.size());
    for (const auto &p : n.restSpine)
        r.spine.push_back({p.x, p.y});
    r.offsets.reserve(n.restOffsets.size());
    for (const auto &o : n.restOffsets)
        r.offsets.push_back({o.s, o.d});
    return r;
}

struct WarpBox {
    std::array<RestVec, 4> corners{};
    RestVec c{};
    Aabb aabb{};
    bool ok = false;
};

inline WarpBox warpBoxFromSmart(const DocNode &sg)
{
    WarpBox b;
    if (sg.kind != NodeKind::SmartGroup)
        return b;
    const SmartBounds &sb = sg.smartBounds;
    const RestVec local[4] = {
        {sb.x, sb.y},
        {sb.x + sb.width, sb.y},
        {sb.x + sb.width, sb.y + sb.height},
        {sb.x, sb.y + sb.height},
    };
    b.aabb.minX = 1e300;
    b.aabb.minY = 1e300;
    b.aabb.maxX = -1e300;
    b.aabb.maxY = -1e300;
    RestVec acc{0, 0};
    for (int i = 0; i < 4; ++i) {
        const Vec2 w = smartLocalToWorld(local[i].x, local[i].y, sg, "boundary", std::nullopt, nullptr);
        b.corners[size_t(i)] = {w.x, w.y};
        acc.x += w.x;
        acc.y += w.y;
        b.aabb.minX = std::min(b.aabb.minX, w.x);
        b.aabb.minY = std::min(b.aabb.minY, w.y);
        b.aabb.maxX = std::max(b.aabb.maxX, w.x);
        b.aabb.maxY = std::max(b.aabb.maxY, w.y);
    }
    b.c = {acc.x * 0.25, acc.y * 0.25};
    b.ok = true;
    return b;
}

inline void edgeFrameFromBox(const WarpBox &b, int edge, RestVec *n, RestVec *along)
{
    const int e = ((edge % 4) + 4) % 4;
    const RestVec p = b.corners[size_t(e)];
    const RestVec q = b.corners[size_t((e + 1) % 4)];
    *along = warpUnit(warpSub(q, p));
    *n = RestVec{along->y, -along->x};
}

inline RestVec anchorPointOnBox(const WarpBox &b, const ConnectorAnchor &a)
{
    if (a.kind == "centre")
        return b.c;
    const int e = ((a.edge % 4) + 4) % 4;
    const RestVec p = b.corners[size_t(e)];
    const RestVec q = b.corners[size_t((e + 1) % 4)];
    const double t = std::max(0.0, std::min(1.0, a.t));
    return warpAdd(p, warpMul(warpSub(q, p), t));
}

inline RestVec facingOnBox(const WarpBox &b, const ConnectorAnchor &a, RestVec peerCentre)
{
    if (a.kind == "centre") {
        const RestVec ray = warpUnit(warpSub(peerCentre, b.c));
        if (kCentreConeDeg <= 0.0)
            return ray;
        const RestVec ex = warpUnit(warpSub(b.corners[1], b.corners[0]));
        const RestVec ey = warpUnit(warpSub(b.corners[3], b.corners[0]));
        const RestVec drawnWorld =
            warpUnit(warpAdd(warpMul(ex, a.drawnBoxX), warpMul(ey, a.drawnBoxY)));
        const double off = warpSignedDeg(ray, drawnWorld);
        if (std::abs(off) <= kCentreConeDeg)
            return drawnWorld;
        const double lim = (off > 0.0 ? kCentreConeDeg : -kCentreConeDeg) * kWarpPi / 180.0;
        return warpUnit(warpRot(ray, lim));
    }
    RestVec n{}, along{};
    edgeFrameFromBox(b, a.edge, &n, &along);
    if (kEdgeFacingPerpendicular)
        return n;
    return warpUnit(warpAdd(warpMul(n, a.drawnN), warpMul(along, a.drawnE)));
}

inline RestVec attachWorld(const DocNode &sg, const ConnectorAnchor &a, const WarpBox &b)
{
    if (a.hasLocal) {
        const Vec2 w = smartLocalToWorld(a.localX, a.localY, sg, "boundary", std::nullopt, nullptr);
        return {w.x, w.y};
    }
    return anchorPointOnBox(b, a);
}

inline RestVec facingAtAttach(const WarpBox &b, const ConnectorAnchor &a, RestVec thisP, RestVec peerP)
{
    if (a.kind == "centre")
        return warpUnit(warpSub(peerP, thisP));
    RestVec n{}, along{};
    edgeFrameFromBox(b, a.edge, &n, &along);
    if (kEdgeFacingPerpendicular)
        return n;
    const RestVec ex = warpUnit(warpSub(b.corners[1], b.corners[0]));
    const RestVec ey = warpUnit(warpSub(b.corners[3], b.corners[0]));
    return warpUnit(warpAdd(warpMul(ex, a.drawnBoxX), warpMul(ey, a.drawnBoxY)));
}

inline ConnectorEndPose poseFromWarpEnd(const WarpEnd &e)
{
    ConnectorEndPose p;
    p.x = e.p.x;
    p.y = e.p.y;
    p.fx = e.f.x;
    p.fy = e.f.y;
    p.valid = true;
    return p;
}

/** Live face frame for Path B styleInk ([ADR-0038]). */
struct FaceFrame {
    RestVec origin{};
    RestVec n{1, 0};
    RestVec e{0, 1};
    bool ok = false;
};

inline FaceFrame liveFaceFrame(const WarpEnd &end, const ConnectorAnchor &a, const DocNode *sg)
{
    FaceFrame f;
    f.origin = end.p;
    if (sg && a.kind != "centre") {
        const WarpBox b = warpBoxFromSmart(*sg);
        if (b.ok) {
            RestVec along{};
            edgeFrameFromBox(b, a.edge, &f.n, &along);
            f.e = along;
            f.ok = true;
            return f;
        }
    }
    f.n = warpUnit(end.f);
    f.e = warpLeft(f.n);
    f.ok = true;
    return f;
}

inline FaceFrame liveFaceFrame(const DeviceDocument &doc, const DocNode &conn, bool fromEnd)
{
    WarpEnd e0, e1;
    if (!resolveConnectorEnds(doc, conn, &e0, &e1))
        return {};
    const ConnectorAnchor &a = fromEnd ? conn.fromAnchor : conn.toAnchor;
    const std::string &nid = fromEnd ? conn.fromNodeId : conn.toNodeId;
    return liveFaceFrame(fromEnd ? e0 : e1, a, doc.find(nid));
}

/** Leaving tangent of the painted spine: into the connector from that end. */
inline RestVec warpedLeaveFromSamples(const std::vector<RestVec> &samples, bool fromEnd)
{
    if (samples.size() < 2)
        return {0, 0};
    if (fromEnd)
        return warpSegTangent(samples, 0);
    return warpUnit(warpMul(warpSegTangent(samples, samples.size() - 1), -1.0));
}

inline std::vector<RestVec> restVecsFromWarpedSamples(const std::vector<ConnectorRestPt> &pts)
{
    std::vector<RestVec> o;
    o.reserve(pts.size());
    for (const auto &p : pts)
        o.push_back({p.x, p.y});
    return o;
}

/**
 * Paint/bind/erase frame for Path B styleInk ([SRS-EP-35]).
 * Storage stays in the live face frame. World paint rotates that frame by
 * α = signed angle from stored leave (drawnN/drawnE / drawnBoxX/drawnBoxY,
 * reconstructed as WarpEnd.f) to the re-warped sample tangent. Drawn leave
 * is not rewritten.
 */
inline FaceFrame styleInkPaintFrame(const FaceFrame &face, RestVec storedLeave, RestVec warpedLeave)
{
    if (!face.ok)
        return face;
    if (warpLen(storedLeave) < 1e-9 || warpLen(warpedLeave) < 1e-9)
        return face;
    const double rad = warpSignedDeg(storedLeave, warpedLeave) * kWarpPi / 180.0;
    FaceFrame p = face;
    p.n = warpRot(face.n, rad);
    p.e = warpRot(face.e, rad);
    return p;
}

inline FaceFrame styleInkPaintFrame(const DeviceDocument &doc, const DocNode &conn, bool fromEnd)
{
    WarpEnd e0, e1;
    if (!resolveConnectorEnds(doc, conn, &e0, &e1))
        return {};
    const ConnectorAnchor &a = fromEnd ? conn.fromAnchor : conn.toAnchor;
    const std::string &nid = fromEnd ? conn.fromNodeId : conn.toNodeId;
    const FaceFrame face = liveFaceFrame(fromEnd ? e0 : e1, a, doc.find(nid));
    const RestVec stored = fromEnd ? e0.f : e1.f;
    const RestVec warped = warpedLeaveFromSamples(restVecsFromWarpedSamples(conn.warpedSamples),
                                                  fromEnd);
    return styleInkPaintFrame(face, stored, warped);
}

inline RestVec faceToWorld(const FaceFrame &f, EndpointInkPt p)
{
    return warpAdd(f.origin, warpAdd(warpMul(f.n, p.n), warpMul(f.e, p.e)));
}

inline EndpointInkPt worldToFace(const FaceFrame &f, RestVec w)
{
    const RestVec d = warpSub(w, f.origin);
    return {warpDot(d, f.n), warpDot(d, f.e)};
}

inline std::vector<InkSample> styleInkStrokeWorld(const FaceFrame &f, const EndpointInkStroke &st)
{
    std::vector<InkSample> out;
    out.reserve(st.pts.size());
    for (const auto &p : st.pts) {
        const RestVec w = faceToWorld(f, p);
        InkSample s;
        s.x = w.x;
        s.y = w.y;
        out.push_back(s);
    }
    return out;
}

inline EndpointInkStroke styleInkStrokeFromWorld(const FaceFrame &f,
                                                 const std::vector<InkSample> &world)
{
    EndpointInkStroke st;
    st.pts.reserve(world.size());
    for (const auto &s : world)
        st.pts.push_back(worldToFace(f, RestVec{s.x, s.y}));
    return st;
}

inline void writeWarpedStyleInk(DocNode &conn, const FaceFrame &fromF, const FaceFrame &toF)
{
    conn.warpedStyleInk.clear();
    auto appendWarped = [&](const FaceFrame &f, const std::vector<EndpointInkStroke> &strokes) {
        for (const auto &st : strokes) {
            std::vector<ConnectorRestPt> poly;
            poly.reserve(st.pts.size());
            for (const auto &p : st.pts) {
                const RestVec w = faceToWorld(f, p);
                poly.push_back({w.x, w.y});
            }
            if (poly.size() >= 2)
                conn.warpedStyleInk.push_back(std::move(poly));
        }
    };
    appendWarped(fromF, conn.fromAnchor.styleInk);
    appendWarped(toF, conn.toAnchor.styleInk);
}

inline WarpEnd endFromPose(const ConnectorEndPose &p)
{
    WarpEnd e;
    e.p = {p.x, p.y};
    e.f = warpUnit({p.fx, p.fy});
    return e;
}

inline bool resolveConnectorEnds(const DeviceDocument &doc, const DocNode &conn, WarpEnd *from,
                                 WarpEnd *to)
{
    if (!from || !to)
        return false;
    const DocNode *sg0 = doc.find(conn.fromNodeId);
    const DocNode *sg1 = doc.find(conn.toNodeId);
    const WarpBox b0 = sg0 ? warpBoxFromSmart(*sg0) : WarpBox{};
    const WarpBox b1 = sg1 ? warpBoxFromSmart(*sg1) : WarpBox{};
    WarpEnd e0;
    WarpEnd e1;
    bool ok0 = false;
    bool ok1 = false;
    if (b0.ok && b1.ok) {
        e0.p = attachWorld(*sg0, conn.fromAnchor, b0);
        e1.p = attachWorld(*sg1, conn.toAnchor, b1);
        e0.f = facingAtAttach(b0, conn.fromAnchor, e0.p, e1.p);
        e1.f = facingAtAttach(b1, conn.toAnchor, e1.p, e0.p);
        e0.centre = conn.fromAnchor.kind == "centre";
        e1.centre = conn.toAnchor.kind == "centre";
        e0.hasClip = false;
        e1.hasClip = false;
        ok0 = ok1 = true;
    } else if (b0.ok && conn.toPose.valid) {
        e0.p = attachWorld(*sg0, conn.fromAnchor, b0);
        e1 = endFromPose(conn.toPose);
        e0.f = facingAtAttach(b0, conn.fromAnchor, e0.p, e1.p);
        e0.centre = conn.fromAnchor.kind == "centre";
        e0.hasClip = false;
        ok0 = ok1 = true;
    } else if (b1.ok && conn.fromPose.valid) {
        e1.p = attachWorld(*sg1, conn.toAnchor, b1);
        e0 = endFromPose(conn.fromPose);
        e1.f = facingAtAttach(b1, conn.toAnchor, e1.p, e0.p);
        e1.centre = conn.toAnchor.kind == "centre";
        e1.hasClip = false;
        ok0 = ok1 = true;
    } else if (conn.fromPose.valid && conn.toPose.valid) {
        e0 = endFromPose(conn.fromPose);
        e1 = endFromPose(conn.toPose);
        ok0 = ok1 = true;
    }
    if (!ok0 || !ok1)
        return false;
    *from = e0;
    *to = e1;
    return true;
}

inline void writeWarpedSamples(DocNode *n, const std::vector<RestVec> &s)
{
    if (!n)
        return;
    n->warpedSamples.clear();
    n->warpedSamples.reserve(s.size());
    for (const auto &p : s)
        n->warpedSamples.push_back({p.x, p.y});
}

inline bool refreshConnectorWarp(DeviceDocument &doc, DocNode &conn)
{
    if (conn.kind != NodeKind::Connector)
        return false;
    WarpEnd e0, e1;
    if (!resolveConnectorEnds(doc, conn, &e0, &e1))
        return false;
    const RestShape rs = restShapeFromNode(conn);
    const WarpResult w = warpConnector(rs, e0, e1, conn.warpStyle);
    writeWarpedSamples(&conn, w.samples);
    const DocNode *sg0 = doc.find(conn.fromNodeId);
    const DocNode *sg1 = doc.find(conn.toNodeId);
    const FaceFrame fromFace = liveFaceFrame(e0, conn.fromAnchor, sg0);
    const FaceFrame toFace = liveFaceFrame(e1, conn.toAnchor, sg1);
    writeWarpedStyleInk(conn,
                        styleInkPaintFrame(fromFace, e0.f, warpedLeaveFromSamples(w.samples, true)),
                        styleInkPaintFrame(toFace, e1.f, warpedLeaveFromSamples(w.samples, false)));
    conn.fromPose = poseFromWarpEnd(e0);
    conn.toPose = poseFromWarpEnd(e1);
    conn.connectorInvalid = false;
    return true;
}

inline void refreshAllConnectorWarps(DeviceDocument &doc)
{
    std::vector<DocNode *> stack;
    for (auto &n : doc.rootChildren)
        stack.push_back(&n);
    while (!stack.empty()) {
        DocNode *n = stack.back();
        stack.pop_back();
        if (n->kind == NodeKind::Connector)
            refreshConnectorWarp(doc, *n);
        for (auto &c : n->children)
            stack.push_back(&c);
    }
}

inline void refreshConnectorsBoundTo(DeviceDocument &doc, const std::string &nodeId)
{
    std::vector<DocNode *> stack;
    for (auto &n : doc.rootChildren)
        stack.push_back(&n);
    while (!stack.empty()) {
        DocNode *n = stack.back();
        stack.pop_back();
        if (n->kind == NodeKind::Connector
            && (n->fromNodeId == nodeId || n->toNodeId == nodeId))
            refreshConnectorWarp(doc, *n);
        for (auto &c : n->children)
            stack.push_back(&c);
    }
}

inline Aabb warpedSamplesAabb(const DocNode &conn)
{
    Aabb a;
    bool any = false;
    auto acc = [&](double x, double y) {
        if (!any) {
            a.minX = a.maxX = x;
            a.minY = a.maxY = y;
            any = true;
        } else {
            a.minX = std::min(a.minX, x);
            a.minY = std::min(a.minY, y);
            a.maxX = std::max(a.maxX, x);
            a.maxY = std::max(a.maxY, y);
        }
    };
    for (const auto &p : conn.warpedSamples)
        acc(p.x, p.y);
    for (const auto &poly : conn.warpedStyleInk) {
        for (const auto &p : poly)
            acc(p.x, p.y);
    }
    return a;
}

} // namespace document
} // namespace epaper
