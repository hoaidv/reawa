#pragma once
/**
 * Nested ink-box hit, composed bounds, and move reparent.
 * @implements [SRS-EP-76] ancestor RenderingContext
 * @implements [SRS-EP-77] tap-hit + move reparent; overflow not hittable
 * @implements [STORY-EP-074] nested tap-select
 * @implements [STORY-EP-076] move reparent
 * @implements [STORY-EP-077] skip subtree outside natural AABB
 * @fix [CHL-0032] nested boxes selectable
 */

#include "nested_flatten.hpp"
#include "recognize_enclose.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace epaper {
namespace document {

inline Affine ancestorContentContext(const DeviceDocument &doc, const std::string &nodeId)
{
    std::vector<const DocNode *> chain;
    DeviceDocument::NodePlace pl;
    std::string cur = nodeId;
    while (doc.findPlace(cur, &pl) && !pl.parentId.empty()) {
        const DocNode *p = doc.find(pl.parentId);
        if (!p)
            break;
        chain.push_back(p);
        cur = pl.parentId;
    }
    Affine ctx = affineIdentity();
    for (int i = int(chain.size()) - 1; i >= 0; --i) {
        const DocNode *p = chain[size_t(i)];
        if (p->kind == NodeKind::SmartGroup)
            ctx = contentOutcome(ctx, *p);
    }
    return ctx;
}

inline Affine parentContentContext(const DeviceDocument &doc, const std::string &parentId)
{
    if (parentId.empty())
        return affineIdentity();
    const DocNode *p = doc.find(parentId);
    if (!p)
        return affineIdentity();
    if (p->kind == NodeKind::SmartGroup)
        return contentOutcome(ancestorContentContext(doc, parentId), *p);
    return affineIdentity();
}

inline void worldToAncestorContent(const DeviceDocument &doc, const std::string &nodeId, double wx,
                                   double wy, double *ox, double *oy)
{
    affineInverse(ancestorContentContext(doc, nodeId)).apply(wx, wy, ox, oy);
}

inline bool composedBoundsOf(const DeviceDocument &doc, const DocNode &n, SmartBounds &out)
{
    if (n.kind == NodeKind::SmartGroup) {
        out = smartGroupWorldBounds(n, ancestorContentContext(doc, n.id));
        return out.width >= 0 && out.height >= 0;
    }
    if (n.kind == NodeKind::Ink) {
        DeviceDocument::NodePlace pl;
        if (doc.findPlace(n.id, &pl) && !pl.parentId.empty()) {
            const DocNode *p = doc.find(pl.parentId);
            if (p && p->kind == NodeKind::SmartGroup) {
                const Affine ctx = ancestorContentContext(doc, n.id);
                const std::string role = n.role ? *n.role : std::string("content");
                const Affine used = (role == "boundary") ? outcomeAffine(ctx, *p)
                                                         : contentOutcome(ctx, *p);
                double minX = std::numeric_limits<double>::infinity();
                double minY = std::numeric_limits<double>::infinity();
                double maxX = -std::numeric_limits<double>::infinity();
                double maxY = -std::numeric_limits<double>::infinity();
                for (const auto &s : n.samples) {
                    double wx = 0;
                    double wy = 0;
                    used.apply(s.x, s.y, &wx, &wy);
                    minX = std::min(minX, wx);
                    minY = std::min(minY, wy);
                    maxX = std::max(maxX, wx);
                    maxY = std::max(maxY, wy);
                }
                if (!std::isfinite(minX))
                    return false;
                out.x = minX;
                out.y = minY;
                out.width = std::max(0.0, maxX - minX);
                out.height = std::max(0.0, maxY - minY);
                return true;
            }
        }
        if (n.samples.empty())
            return false;
        out = samplesAabb(n.samples);
        return true;
    }
    if (n.kind == NodeKind::Frame) {
        out.x = n.bounds.minX;
        out.y = n.bounds.minY;
        out.width = n.bounds.maxX - n.bounds.minX;
        out.height = n.bounds.maxY - n.bounds.minY;
        return true;
    }
    if (n.kind == NodeKind::Text) {
        out.x = n.box.minX;
        out.y = n.box.minY;
        out.width = n.box.maxX - n.box.minX;
        out.height = n.box.maxY - n.box.minY;
        return true;
    }
    if (n.kind == NodeKind::Primitive) {
        out.x = n.gx;
        out.y = n.gy;
        out.width = n.gw;
        out.height = n.gh;
        return true;
    }
    return false;
}

inline bool unionComposedAabbOfIds(const DeviceDocument &doc, const std::vector<std::string> &ids,
                                   SmartBounds &out)
{
    bool any = false;
    double minX = 0, minY = 0, maxX = 0, maxY = 0;
    for (const auto &id : ids) {
        const DocNode *n = doc.find(id);
        if (!n)
            continue;
        SmartBounds b;
        if (!composedBoundsOf(doc, *n, b))
            continue;
        if (!any) {
            minX = b.x;
            minY = b.y;
            maxX = b.x + b.width;
            maxY = b.y + b.height;
            any = true;
        } else {
            minX = std::min(minX, b.x);
            minY = std::min(minY, b.y);
            maxX = std::max(maxX, b.x + b.width);
            maxY = std::max(maxY, b.y + b.height);
        }
    }
    if (!any)
        return false;
    out.x = minX;
    out.y = minY;
    out.width = maxX - minX;
    out.height = maxY - minY;
    return true;
}

inline double worldAabbArea(const SmartBounds &b)
{
    return std::max(0.0, b.width) * std::max(0.0, b.height);
}

inline double worldAabbOverlap(const SmartBounds &a, const SmartBounds &b)
{
    const double x0 = std::max(a.x, b.x);
    const double y0 = std::max(a.y, b.y);
    const double x1 = std::min(a.x + a.width, b.x + b.width);
    const double y1 = std::min(a.y + a.height, b.y + b.height);
    if (x1 <= x0 || y1 <= y0)
        return 0;
    return (x1 - x0) * (y1 - y0);
}

inline double fractionAabbInsideAabb(const SmartBounds &inner, const SmartBounds &outer)
{
    const double a = worldAabbArea(inner);
    if (a <= 1e-12)
        return 0;
    return worldAabbOverlap(inner, outer) / a;
}

inline bool pointInSmartWorld(const DocNode &sg, const Affine &ctx, double wx, double wy)
{
    const SmartBounds b = smartGroupWorldBounds(sg, ctx);
    return wx >= b.x && wx <= b.x + b.width && wy >= b.y && wy <= b.y + b.height;
}

/**
 * Deepest SmartGroup containing (wx,wy). Children before ancestors; later siblings first.
 * @implements [SRS-EP-77] tap-select any level
 */
inline const DocNode *hitTapSmartGroup(const std::vector<DocNode> &nodes, const Affine &ctx,
                                       double wx, double wy)
{
    for (int i = int(nodes.size()) - 1; i >= 0; --i) {
        const DocNode &n = nodes[size_t(i)];
        if (n.kind == NodeKind::Frame || n.kind == NodeKind::Group) {
            if (const DocNode *h = hitTapSmartGroup(n.children, affineIdentity(), wx, wy))
                return h;
            continue;
        }
        if (n.kind != NodeKind::SmartGroup)
            continue;
        // Overflow past this group's natural AABB is not hittable ([SRS-EP-77]).
        if (!pointInSmartWorld(n, ctx, wx, wy))
            continue;
        const Affine childCtx = contentOutcome(ctx, n);
        if (const DocNode *inner = hitTapSmartGroup(n.children, childCtx, wx, wy))
            return inner;
        return &n;
    }
    return nullptr;
}

inline const DocNode *hitTapSmartGroup(const DeviceDocument &doc, double wx, double wy)
{
    return hitTapSmartGroup(doc.rootChildren, affineIdentity(), wx, wy);
}

inline bool isDescendantId(const DeviceDocument &doc, const std::string &anc, const std::string &id)
{
    DeviceDocument::NodePlace pl;
    std::string cur = id;
    while (doc.findPlace(cur, &pl)) {
        if (pl.parentId.empty())
            return false;
        if (pl.parentId == anc)
            return true;
        cur = pl.parentId;
    }
    return false;
}

inline SmartBounds movingNaturalWorld(const DocNode &moving, const Affine &movingCtx)
{
    if (moving.kind == NodeKind::SmartGroup)
        return smartGroupWorldBounds(moving, movingCtx);
    return samplesAabb(moving.samples);
}

inline double fractionNaturalAreaInsideBoundary(const DocNode &moving, const Affine &movingCtx,
                                                const DocNode &candidate, const Affine &candCtx)
{
    const SmartBounds nat = movingNaturalWorld(moving, movingCtx);
    const double area = worldAabbArea(nat);
    if (area <= 1e-12)
        return 0;
    if (candidate.kind != NodeKind::SmartGroup) {
        SmartBounds cb;
        if (candidate.kind == NodeKind::Frame) {
            cb.x = candidate.bounds.minX;
            cb.y = candidate.bounds.minY;
            cb.width = candidate.bounds.maxX - candidate.bounds.minX;
            cb.height = candidate.bounds.maxY - candidate.bounds.minY;
        } else {
            return 0;
        }
        return worldAabbOverlap(nat, cb) / area;
    }
    const auto poly = smartGroupBoundaryWorld(candidate, candCtx);
    if (poly.size() < 3) {
        const SmartBounds cb = smartGroupWorldBounds(candidate, candCtx);
        return worldAabbOverlap(nat, cb) / area;
    }
    int hit = 0;
    int n = 0;
    for (int gy = 0; gy < 5; ++gy) {
        for (int gx = 0; gx < 5; ++gx) {
            const double x = nat.x + nat.width * (gx + 0.5) / 5.0;
            const double y = nat.y + nat.height * (gy + 0.5) / 5.0;
            ++n;
            if (pointInBoundary(x, y, poly))
                ++hit;
        }
    }
    return n == 0 ? 0 : double(hit) / double(n);
}

/**
 * Highest paint-order container that contains ≥80% natural area, else "".
 */
inline std::string chooseMoveParentId(const std::vector<DocNode> &nodes, const Affine &ctx,
                                      const DeviceDocument &doc, const DocNode &moving,
                                      const Affine &movingCtx, const std::string &movingId)
{
    for (int i = int(nodes.size()) - 1; i >= 0; --i) {
        const DocNode &n = nodes[size_t(i)];
        if (n.id == movingId || isDescendantId(doc, movingId, n.id))
            continue;
        if (n.kind == NodeKind::Frame || n.kind == NodeKind::Group) {
            const std::string inner =
                chooseMoveParentId(n.children, affineIdentity(), doc, moving, movingCtx, movingId);
            if (!inner.empty())
                return inner;
            SmartBounds cb;
            if (n.kind == NodeKind::Frame) {
                cb.x = n.bounds.minX;
                cb.y = n.bounds.minY;
                cb.width = n.bounds.maxX - n.bounds.minX;
                cb.height = n.bounds.maxY - n.bounds.minY;
                if (fractionAabbInsideAabb(movingNaturalWorld(moving, movingCtx), cb) >= 0.8)
                    return n.id;
            }
            continue;
        }
        if (n.kind != NodeKind::SmartGroup)
            continue;
        const Affine childCtx = contentOutcome(ctx, n);
        const std::string inner =
            chooseMoveParentId(n.children, childCtx, doc, moving, movingCtx, movingId);
        if (!inner.empty())
            return inner;
        if (fractionNaturalAreaInsideBoundary(moving, movingCtx, n, ctx) >= 0.8)
            return n.id;
    }
    return {};
}

inline std::string chooseMoveParentId(const DeviceDocument &doc, const std::string &movingId)
{
    const DocNode *moving = doc.find(movingId);
    if (!moving)
        return {};
    const Affine movingCtx = ancestorContentContext(doc, movingId);
    return chooseMoveParentId(doc.rootChildren, affineIdentity(), doc, *moving, movingCtx,
                              movingId);
}

} // namespace document
} // namespace epaper
