#pragma once

/**
 * Object erase: 80% table, whole-node remove, 0 remnants.
 * @implements [SRS-EP-58] object erase
 * @implements [STORY-EP-066] Object erase 80 percent table
 */

#include "erase_area.hpp"
#include "surround_create.hpp"

#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace epaper {
namespace document {

inline std::vector<ErasePt> downsampleErasePoly(const std::vector<ErasePt> &p, size_t maxPts)
{
    if (maxPts < 2 || p.size() <= maxPts)
        return p;
    std::vector<ErasePt> o;
    o.reserve(maxPts);
    const size_t last = p.size() - 1;
    for (size_t i = 0; i < maxPts; ++i) {
        const size_t j = i * last / (maxPts - 1);
        o.push_back(p[j]);
    }
    return o;
}

inline std::vector<InkSample> erasePolyToSamples(const std::vector<ErasePt> &poly)
{
    std::vector<InkSample> s;
    s.reserve(poly.size());
    for (const auto &p : poly) {
        InkSample i;
        i.x = p.x;
        i.y = p.y;
        s.push_back(i);
    }
    return s;
}

inline double fractionArcLengthInside(const std::vector<InkSample> &samples,
                                      const std::vector<InkSample> &lasso)
{
    if (samples.size() < 2 || lasso.size() < 3)
        return 0;
    double total = 0;
    for (size_t i = 1; i < samples.size(); ++i)
        total += std::hypot(samples[i].x - samples[i - 1].x, samples[i].y - samples[i - 1].y);
    if (total <= 1e-12)
        return 0;
    const double need = 0.8 * total;
    double inside = 0;
    double remaining = total;
    for (size_t i = 1; i < samples.size(); ++i) {
        const double dx = samples[i].x - samples[i - 1].x;
        const double dy = samples[i].y - samples[i - 1].y;
        const double len = std::hypot(dx, dy);
        remaining -= len;
        if (len < 1e-12)
            continue;
        const double mx = 0.5 * (samples[i].x + samples[i - 1].x);
        const double my = 0.5 * (samples[i].y + samples[i - 1].y);
        if (pointInPolygonEvenOdd(mx, my, lasso))
            inside += len;
        if (inside >= need)
            return inside / total;
        if (inside + remaining < need)
            return inside / total;
    }
    return inside / total;
}

inline std::vector<InkSample> smartGroupBoundaryWorld(const DocNode &sg)
{
    std::vector<InkSample> world = sg.boundaryPolyline;
    for (auto &s : world) {
        const Vec2 w = smartLocalToWorld(s.x, s.y, sg, "boundary", std::nullopt, nullptr);
        s.x = w.x;
        s.y = w.y;
    }
    return world;
}

inline double fractionBoundaryAreaInside(const DocNode &sg, const std::vector<InkSample> &lasso)
{
    const std::vector<InkSample> poly = smartGroupBoundaryWorld(sg);
    if (poly.size() < 3)
        return 0;
    SmartBounds b = samplesAabb(poly);
    int inPoly = 0;
    int inBoth = 0;
    for (int gy = 0; gy < 8; ++gy) {
        for (int gx = 0; gx < 8; ++gx) {
            const double x = b.x + b.width * (gx + 0.5) / 8.0;
            const double y = b.y + b.height * (gy + 0.5) / 8.0;
            if (!pointInPolygonEvenOdd(x, y, poly))
                continue;
            ++inPoly;
            if (pointInPolygonEvenOdd(x, y, lasso))
                ++inBoth;
        }
    }
    return inPoly == 0 ? 0 : static_cast<double>(inBoth) / static_cast<double>(inPoly);
}

inline bool aabbIntersectsInclusive(const SmartBounds &a, const SmartBounds &b)
{
    return a.x <= b.x + b.width && b.x <= a.x + a.width && a.y <= b.y + b.height
        && b.y <= a.y + a.height;
}

inline bool objectEraseCullAabb(const DocNode &n, SmartBounds *out)
{
    if (n.kind == NodeKind::Connector) {
        const auto path = connectorWorldPath(n);
        if (path.empty())
            return false;
        *out = samplesAabb(path);
        return true;
    }
    return nodeWorldAabb(n, *out);
}

enum class ObjectErasePass { Commit, Overlay };

constexpr size_t kObjectEraseOverlayLassoMax = 48;
constexpr int kObjectEraseOverlayBudgetUs = 8000;

inline bool objectEraseHits(const DocNode &n, const std::vector<InkSample> &lasso,
                            ObjectErasePass pass = ObjectErasePass::Commit)
{
    if (n.kind == NodeKind::Frame)
        return false;
    if (n.kind == NodeKind::Ink) {
        if (n.samples.empty())
            return false;
        if (n.samples.size() < 2)
            return pointInPolygonEvenOdd(n.samples[0].x, n.samples[0].y, lasso);
        return fractionArcLengthInside(n.samples, lasso) >= 0.8;
    }
    if (n.kind == NodeKind::SmartGroup) {
        if (pass == ObjectErasePass::Overlay) {
            SmartBounds wb;
            if (!nodeWorldAabb(n, wb))
                return false;
            return fractionAabbInsidePolygon(wb, lasso) >= 0.8;
        }
        return fractionBoundaryAreaInside(n, lasso) >= 0.8;
    }
    if (n.kind == NodeKind::Connector)
        return fractionArcLengthInside(connectorWorldPath(n), lasso) >= 0.8;
    if (n.kind == NodeKind::Primitive || n.kind == NodeKind::Text) {
        SmartBounds wb;
        if (!boundsOf(n, wb))
            return false;
        return fractionAabbInsidePolygon(wb, lasso) >= 0.8;
    }
    return false;
}

inline void collectObjectHits(const std::vector<DocNode> &nodes, const std::vector<InkSample> &lasso,
                              const SmartBounds &lassoAabb, ObjectErasePass pass,
                              std::vector<std::string> *hitIds,
                              const std::chrono::steady_clock::time_point *deadline = nullptr)
{
    for (const auto &n : nodes) {
        if (deadline && std::chrono::steady_clock::now() > *deadline)
            return;
        if (n.kind == NodeKind::Frame) {
            collectObjectHits(n.children, lasso, lassoAabb, pass, hitIds, deadline);
            continue;
        }
        if (n.kind == NodeKind::Group) {
            collectObjectHits(n.children, lasso, lassoAabb, pass, hitIds, deadline);
            continue;
        }
        if (pass == ObjectErasePass::Overlay
            && (n.kind == NodeKind::Ink || n.kind == NodeKind::Connector))
            continue;
        SmartBounds nb;
        if (objectEraseCullAabb(n, &nb) && !aabbIntersectsInclusive(nb, lassoAabb))
            continue;
        if (objectEraseHits(n, lasso, pass))
            hitIds->push_back(n.id);
    }
}

inline std::vector<std::string> objectEraseCandidateIds(const DeviceDocument &doc,
                                                        const std::vector<ErasePt> &poly,
                                                        ObjectErasePass pass = ObjectErasePass::Commit)
{
    auto closed = autoCloseErasePoly(poly);
    if (pass == ObjectErasePass::Overlay)
        closed = downsampleErasePoly(closed, kObjectEraseOverlayLassoMax);
    const auto lasso = erasePolyToSamples(closed);
    if (lasso.size() < 3)
        return {};
    std::vector<std::string> ids;
    const std::chrono::steady_clock::time_point *deadline = nullptr;
    std::chrono::steady_clock::time_point until;
    if (pass == ObjectErasePass::Overlay) {
        until = std::chrono::steady_clock::now()
            + std::chrono::microseconds(kObjectEraseOverlayBudgetUs);
        deadline = &until;
    }
    collectObjectHits(doc.rootChildren, lasso, samplesAabb(lasso), pass, &ids, deadline);
    return ids;
}

inline ApplyResult commitObjectErase(DeviceDocument &doc, const std::string &opId,
                                     std::vector<ErasePt> poly)
{
    poly = autoCloseErasePoly(std::move(poly));
    if (poly.size() < 3)
        return {true, "noop"};
    const auto lasso = erasePolyToSamples(poly);
    std::vector<std::string> hitIds;
    collectObjectHits(doc.rootChildren, lasso, samplesAabb(lasso), ObjectErasePass::Commit, &hitIds);
    if (hitIds.empty())
        return {true, "noop"};
    std::vector<std::unique_ptr<DocEdit>> parts;
    for (const auto &id : hitIds) {
        const DocNode *n = doc.find(id);
        if (n && n->kind == NodeKind::Connector)
            planConnectorRemove(opId, id, &parts);
        else
            parts.push_back(makeRemoveEdit(opId, id));
    }
    return doc.commitGesture(opId, std::move(parts));
}

} // namespace document
} // namespace epaper
