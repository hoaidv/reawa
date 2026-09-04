#pragma once
/**
 * Path B endpoint-ink: steal, bind, tick erase.
 * @implements [SRS-EP-35] endpoint-ink membership
 * @implements [SRS-EP-74] Path B product
 * @implements [ADR-0038] face frame, length steal, tick erase
 */

#include "connector_warp.hpp"
#include "erase_clip.hpp"
#include "operations/remove_node_edit.hpp"
#include "operations/set_endpoint_ink_edit.hpp"
#include "surround_create.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace epaper {
namespace document {

constexpr double kEndpointInkRadiusMm = 5.0;
constexpr double kEndpointInkLengthFrac = 0.8;

struct EndpointInkResult {
    bool bound = false;
    std::string connectorId;
    std::string end;
    std::string reason;
};

inline void collectConnectors(const std::vector<DocNode> &nodes,
                              std::vector<const DocNode *> *out)
{
    for (const auto &n : nodes) {
        if (n.kind == NodeKind::Connector)
            out->push_back(&n);
        collectConnectors(n.children, out);
    }
}

inline void collectConnectorsMut(std::vector<DocNode> &nodes, std::vector<DocNode *> *out)
{
    for (auto &n : nodes) {
        if (n.kind == NodeKind::Connector)
            out->push_back(&n);
        collectConnectorsMut(n.children, out);
    }
}

inline double segmentLengthInDisk(RestVec a, RestVec b, RestVec c, double R)
{
    const double ax = a.x - c.x;
    const double ay = a.y - c.y;
    const double bx = b.x - c.x;
    const double by = b.y - c.y;
    const double dx = bx - ax;
    const double dy = by - ay;
    const double A = dx * dx + dy * dy;
    const double len = std::sqrt(A);
    if (len < 1e-12)
        return 0;
    const double B = 2 * (ax * dx + ay * dy);
    const double C = ax * ax + ay * ay - R * R;
    std::vector<double> ts;
    ts.push_back(0);
    ts.push_back(1);
    const double disc = B * B - 4 * A * C;
    if (disc >= 0) {
        const double s = std::sqrt(disc);
        const double t1 = (-B - s) / (2 * A);
        const double t2 = (-B + s) / (2 * A);
        if (t1 > 0 && t1 < 1)
            ts.push_back(t1);
        if (t2 > 0 && t2 < 1)
            ts.push_back(t2);
    }
    std::sort(ts.begin(), ts.end());
    auto inside = [&](double t) {
        const double x = ax + t * dx;
        const double y = ay + t * dy;
        return x * x + y * y <= R * R + 1e-9;
    };
    double acc = 0;
    for (size_t i = 1; i < ts.size(); ++i) {
        const double mid = 0.5 * (ts[i - 1] + ts[i]);
        if (inside(mid))
            acc += (ts[i] - ts[i - 1]) * len;
    }
    return acc;
}

inline double polylineLengthInDisk(const std::vector<InkSample> &s, RestVec c, double R)
{
    double acc = 0;
    for (size_t i = 1; i < s.size(); ++i)
        acc += segmentLengthInDisk({s[i - 1].x, s[i - 1].y}, {s[i].x, s[i].y}, c, R);
    return acc;
}

struct EndpointStealHit {
    std::string connectorId;
    bool fromEnd = true;
};

inline std::vector<EndpointStealHit> collectEndpointSteals(const DeviceDocument &doc,
                                                           const std::vector<InkSample> &samples)
{
    std::vector<EndpointStealHit> hits;
    const double R = eraseMmToWorld(kEndpointInkRadiusMm);
    const double total = polylineArcLength(samples);
    if (total <= 1e-12)
        return hits;
    const double need = kEndpointInkLengthFrac * total;
    std::vector<const DocNode *> conns;
    collectConnectors(doc.rootChildren, &conns);
    for (const DocNode *c : conns) {
        if (!c || c->kind != NodeKind::Connector)
            continue;
        WarpEnd e0, e1;
        if (!resolveConnectorEnds(doc, *c, &e0, &e1))
            continue;
        const bool fromOk = polylineLengthInDisk(samples, e0.p, R) + 1e-9 >= need;
        const bool toOk = polylineLengthInDisk(samples, e1.p, R) + 1e-9 >= need;
        if (fromOk && toOk)
            continue;
        if (fromOk)
            hits.push_back({c->id, true});
        else if (toOk)
            hits.push_back({c->id, false});
    }
    return hits;
}

inline EndpointInkResult tryBindEndpointInk(DeviceDocument &doc, const std::string &inkId)
{
    EndpointInkResult out;
    const DocNode *ink = doc.find(inkId);
    if (!ink || ink->kind != NodeKind::Ink || ink->samples.size() < 2) {
        out.reason = "not_ink";
        return out;
    }
    const auto hits = collectEndpointSteals(doc, ink->samples);
    if (hits.size() != 1) {
        out.reason = hits.empty() ? "no_end" : "mixed";
        return out;
    }
    const EndpointStealHit &h = hits[0];
    const DocNode *conn = doc.find(h.connectorId);
    if (!conn || conn->kind != NodeKind::Connector) {
        out.reason = "missing_connector";
        return out;
    }
    const FaceFrame f = styleInkPaintFrame(doc, *conn, h.fromEnd);
    if (!f.ok) {
        out.reason = "no_frame";
        return out;
    }
    std::vector<EndpointInkStroke> next =
        h.fromEnd ? conn->fromAnchor.styleInk : conn->toAnchor.styleInk;
    next.push_back(styleInkStrokeFromWorld(f, ink->samples));
    const std::string opId = std::string("set_endpoint_ink:") + inkId;
    std::vector<std::unique_ptr<DocEdit>> parts;
    auto set = std::make_unique<SetEndpointInkEdit>(h.connectorId, h.fromEnd, std::move(next));
    set->setId(opId);
    parts.push_back(std::move(set));
    parts.push_back(makeRemoveEdit(opId, inkId));
    const ApplyResult r = doc.commitGesture(opId, std::move(parts));
    if (!r.applied) {
        out.reason = r.reason.empty() ? "commit_failed" : r.reason;
        return out;
    }
    refreshAllConnectorWarps(doc);
    out.bound = true;
    out.connectorId = h.connectorId;
    out.end = h.fromEnd ? "from" : "to";
    out.reason = "none";
    return out;
}

inline std::vector<std::unique_ptr<DocEdit>> planStyleInkEraseEdits(DeviceDocument &doc,
                                                                   const std::string &opId,
                                                                   const ClipRegion &region)
{
    std::vector<std::unique_ptr<DocEdit>> parts;
    std::vector<DocNode *> conns;
    collectConnectorsMut(doc.rootChildren, &conns);
    const EraseAabb regionBox = clipRegionAabb(region);
    for (DocNode *c : conns) {
        if (!c)
            continue;
        for (int end = 0; end < 2; ++end) {
            const bool fromEnd = end == 0;
            const auto &cur = fromEnd ? c->fromAnchor.styleInk : c->toAnchor.styleInk;
            if (cur.empty())
                continue;
            const FaceFrame f = styleInkPaintFrame(doc, *c, fromEnd);
            if (!f.ok)
                continue;
            std::vector<EndpointInkStroke> next;
            bool hit = false;
            for (const auto &st : cur) {
                const std::vector<InkSample> world = styleInkStrokeWorld(f, st);
                if (!samplesOverlapAabb(world, regionBox)) {
                    next.push_back(st);
                    continue;
                }
                const ClipResult clipped = clipInkPolyline(world, region);
                if (!clipped.hit) {
                    next.push_back(st);
                    continue;
                }
                hit = true;
                for (const auto &rem : clipped.remnants)
                    next.push_back(styleInkStrokeFromWorld(f, rem));
            }
            if (!hit)
                continue;
            auto e = std::make_unique<SetEndpointInkEdit>(c->id, fromEnd, std::move(next));
            e->setId(opId);
            parts.push_back(std::move(e));
        }
    }
    return parts;
}

inline double styleInkFractionInLasso(const std::vector<InkSample> &samples,
                                      const std::vector<InkSample> &lasso)
{
    if (samples.size() < 2 || lasso.size() < 3)
        return 0;
    double total = 0;
    for (size_t i = 1; i < samples.size(); ++i)
        total += std::hypot(samples[i].x - samples[i - 1].x, samples[i].y - samples[i - 1].y);
    if (total <= 1e-12)
        return 0;
    double inside = 0;
    for (size_t i = 1; i < samples.size(); ++i) {
        const double len =
            std::hypot(samples[i].x - samples[i - 1].x, samples[i].y - samples[i - 1].y);
        const double mx = 0.5 * (samples[i].x + samples[i - 1].x);
        const double my = 0.5 * (samples[i].y + samples[i - 1].y);
        if (pointInPolygonEvenOdd(mx, my, lasso))
            inside += len;
    }
    return inside / total;
}

inline std::vector<std::unique_ptr<DocEdit>> planStyleInkObjectErase(
    DeviceDocument &doc, const std::string &opId, const std::vector<InkSample> &lasso,
    const std::unordered_set<std::string> &removedConnectors)
{
    std::vector<std::unique_ptr<DocEdit>> parts;
    if (lasso.size() < 3)
        return parts;
    std::vector<DocNode *> conns;
    collectConnectorsMut(doc.rootChildren, &conns);
    for (DocNode *c : conns) {
        if (!c || removedConnectors.count(c->id))
            continue;
        for (int end = 0; end < 2; ++end) {
            const bool fromEnd = end == 0;
            const auto &cur = fromEnd ? c->fromAnchor.styleInk : c->toAnchor.styleInk;
            if (cur.empty())
                continue;
            const FaceFrame f = styleInkPaintFrame(doc, *c, fromEnd);
            if (!f.ok)
                continue;
            std::vector<EndpointInkStroke> next;
            bool hit = false;
            for (const auto &st : cur) {
                const std::vector<InkSample> world = styleInkStrokeWorld(f, st);
                if (styleInkFractionInLasso(world, lasso) + 1e-9 >= 0.8) {
                    hit = true;
                    continue;
                }
                next.push_back(st);
            }
            if (!hit)
                continue;
            auto e = std::make_unique<SetEndpointInkEdit>(c->id, fromEnd, std::move(next));
            e->setId(opId);
            parts.push_back(std::move(e));
        }
    }
    return parts;
}

} // namespace document
} // namespace epaper
