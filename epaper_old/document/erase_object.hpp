#pragma once

/**
 * Object erase: 80% table, whole-node remove, 0 remnants.
 * @implements [SRS-EP-58] object erase
 * @implements [STORY-EP-066] Object erase 80 percent table
 */

#include "erase_area.hpp"
#include "surround_create.hpp"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace epaper {
namespace document {

template <typename Pt>
inline std::vector<Pt> downsamplePolyline(const std::vector<Pt> &p, size_t maxPts)
{
    if (maxPts < 2 || p.size() <= maxPts)
        return p;
    std::vector<Pt> o;
    o.reserve(maxPts);
    const size_t last = p.size() - 1;
    for (size_t i = 0; i < maxPts; ++i) {
        const size_t j = i * last / (maxPts - 1);
        o.push_back(p[j]);
    }
    return o;
}

inline std::vector<ErasePt> downsampleErasePoly(const std::vector<ErasePt> &p, size_t maxPts)
{
    return downsamplePolyline(p, maxPts);
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

inline std::vector<InkSample> smartGroupBoundaryWorldSamples(const DocNode &sg)
{
    std::vector<InkSample> world = sg.boundaryPolyline;
    for (auto &s : world) {
        const Vec2 w = smartLocalToWorld(s.x, s.y, sg, "boundary", std::nullopt, nullptr);
        s.x = w.x;
        s.y = w.y;
    }
    return world;
}

enum class ObjectErasePass { Commit, Overlay };

/** Compute copies only. Paint uses the full freeform. */
constexpr size_t kObjectEraseOverlayLassoMax = 48;
constexpr size_t kObjectEraseBoundaryMax = 48;

/** 8×8 even-odd area of `poly` that also lies in `lasso`. `poly` should already be coarsened. */
inline double fractionPolyAreaInside(const std::vector<InkSample> &poly,
                                     const std::vector<InkSample> &lasso)
{
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

inline double fractionBoundaryAreaInside(const DocNode &sg, const std::vector<InkSample> &lasso)
{
    return fractionPolyAreaInside(
        downsamplePolyline(smartGroupBoundaryWorldSamples(sg), kObjectEraseBoundaryMax), lasso);
}

struct ObjectEraseSubject {
    std::string id;
    NodeKind kind = NodeKind::Ink;
    SmartBounds aabb;
    std::vector<InkSample> inkSamples;
    std::vector<InkSample> boundaryWorld;
    std::vector<InkSample> connectorPath;
};

struct ObjectEraseOverlayJob {
    std::uint64_t gen = 0;
    std::vector<InkSample> lasso;
    std::vector<ObjectEraseSubject> subjects;
};

struct ObjectEraseOverlayResult {
    std::uint64_t gen = 0;
    std::vector<std::string> ids;
};

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

inline bool objectEraseHitsSubject(const ObjectEraseSubject &s, const std::vector<InkSample> &lasso)
{
    if (s.kind == NodeKind::Frame)
        return false;
    if (s.kind == NodeKind::Ink) {
        if (s.inkSamples.empty())
            return false;
        if (s.inkSamples.size() < 2)
            return pointInPolygonEvenOdd(s.inkSamples[0].x, s.inkSamples[0].y, lasso);
        return fractionArcLengthInside(s.inkSamples, lasso) >= 0.8;
    }
    if (s.kind == NodeKind::SmartGroup)
        return fractionPolyAreaInside(s.boundaryWorld, lasso) >= 0.8;
    if (s.kind == NodeKind::Connector)
        return fractionArcLengthInside(s.connectorPath, lasso) >= 0.8;
    if (s.kind == NodeKind::Primitive || s.kind == NodeKind::Text)
        return fractionAabbInsidePolygon(s.aabb, lasso) >= 0.8;
    return false;
}

inline bool objectEraseHits(const DocNode &n, const std::vector<InkSample> &lasso,
                            ObjectErasePass pass = ObjectErasePass::Commit)
{
    (void)pass;
    if (n.kind == NodeKind::Frame)
        return false;
    if (n.kind == NodeKind::Ink) {
        if (n.samples.empty())
            return false;
        if (n.samples.size() < 2)
            return pointInPolygonEvenOdd(n.samples[0].x, n.samples[0].y, lasso);
        return fractionArcLengthInside(n.samples, lasso) >= 0.8;
    }
    if (n.kind == NodeKind::SmartGroup)
        return fractionBoundaryAreaInside(n, lasso) >= 0.8;
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

inline void collectObjectEraseSubjects(const std::vector<DocNode> &nodes, const SmartBounds &lassoAabb,
                                       ObjectErasePass pass, std::vector<ObjectEraseSubject> *out)
{
    for (const auto &n : nodes) {
        if (n.kind == NodeKind::Frame) {
            collectObjectEraseSubjects(n.children, lassoAabb, pass, out);
            continue;
        }
        if (n.kind == NodeKind::Group)
            continue;
        if (pass == ObjectErasePass::Overlay
            && (n.kind == NodeKind::Ink || n.kind == NodeKind::Connector))
            continue;
        ObjectEraseSubject s;
        s.id = n.id;
        s.kind = n.kind;
        if (!objectEraseCullAabb(n, &s.aabb) || !aabbIntersectsInclusive(s.aabb, lassoAabb))
            continue;
        if (n.kind == NodeKind::Ink)
            s.inkSamples = n.samples;
        else if (n.kind == NodeKind::SmartGroup)
            s.boundaryWorld =
                downsamplePolyline(smartGroupBoundaryWorldSamples(n), kObjectEraseBoundaryMax);
        else if (n.kind == NodeKind::Connector)
            s.connectorPath = connectorWorldPath(n);
        else if (n.kind == NodeKind::Primitive || n.kind == NodeKind::Text) {
            if (!boundsOf(n, s.aabb))
                continue;
        }
        out->push_back(std::move(s));
    }
}

inline std::vector<std::string> objectEraseHitIds(const std::vector<ObjectEraseSubject> &subjects,
                                                  const std::vector<InkSample> &lasso,
                                                  const std::atomic<bool> *cancel = nullptr)
{
    std::vector<std::string> ids;
    for (const auto &s : subjects) {
        if (cancel && cancel->load())
            break;
        if (objectEraseHitsSubject(s, lasso))
            ids.push_back(s.id);
    }
    return ids;
}

inline void collectObjectHits(const std::vector<DocNode> &nodes, const std::vector<InkSample> &lasso,
                              const SmartBounds &lassoAabb, ObjectErasePass pass,
                              std::vector<std::string> *hitIds)
{
    std::vector<ObjectEraseSubject> subjects;
    collectObjectEraseSubjects(nodes, lassoAabb, pass, &subjects);
    *hitIds = objectEraseHitIds(subjects, lasso);
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
    collectObjectHits(doc.rootChildren, lasso, samplesAabb(lasso), pass, &ids);
    return ids;
}

inline ObjectEraseOverlayJob objectEraseMakeOverlayJob(const DeviceDocument &doc,
                                                       const std::vector<ErasePt> &poly,
                                                       std::uint64_t gen)
{
    ObjectEraseOverlayJob job;
    job.gen = gen;
    const auto closed =
        downsampleErasePoly(autoCloseErasePoly(poly), kObjectEraseOverlayLassoMax);
    job.lasso = erasePolyToSamples(closed);
    if (job.lasso.size() >= 3)
        collectObjectEraseSubjects(doc.rootChildren, samplesAabb(job.lasso),
                                   ObjectErasePass::Overlay, &job.subjects);
    return job;
}

inline ObjectEraseOverlayResult objectEraseRunOverlayJob(const ObjectEraseOverlayJob &job,
                                                         const std::atomic<bool> &cancel)
{
    ObjectEraseOverlayResult out;
    out.gen = job.gen;
    if (cancel.load() || job.lasso.size() < 3)
        return out;
    out.ids = objectEraseHitIds(job.subjects, job.lasso, &cancel);
    return out;
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
    std::vector<std::unique_ptr<DocEdit>> parts;
    for (const auto &id : hitIds) {
        const DocNode *n = doc.find(id);
        if (n && n->kind == NodeKind::Connector)
            planConnectorRemove(opId, id, &parts);
        else
            parts.push_back(makeRemoveEdit(opId, id));
    }
    std::unordered_set<std::string> removed(hitIds.begin(), hitIds.end());
    auto styleParts = planStyleInkObjectErase(doc, opId, lasso, removed);
    for (auto &e : styleParts)
        parts.push_back(std::move(e));
    if (parts.empty())
        return {true, "noop"};
    return doc.commitGesture(opId, std::move(parts));
}

} // namespace document
} // namespace epaper
