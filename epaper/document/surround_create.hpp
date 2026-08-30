#pragma once
/**
 * Selection-create: surround stroke required. Invoked only from cta.enclose.
 * @implements [SRS-EP-10] selection create surround
 * @implements [SRS-EP-11] rect / freeform pickable sets
 */

#include "device_document.hpp"
#include "membership.hpp"
#include "recognize_enclose.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace epaper {
namespace document {

struct SelectionCreateResult {
    bool created = false;
    std::string reason;
    std::string smartGroupId;
    std::string boundaryId;
    std::vector<std::string> childIds;
};

inline bool pointInPolygonEvenOdd(double x, double y, const std::vector<InkSample> &poly)
{
    if (poly.size() < 3)
        return false;
    bool inside = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const double xi = poly[i].x, yi = poly[i].y;
        const double xj = poly[j].x, yj = poly[j].y;
        const bool intersect = ((yi > y) != (yj > y))
            && (x < (xj - xi) * (y - yi) / ((yj - yi) + 0.0) + xi);
        if (intersect)
            inside = !inside;
    }
    return inside;
}

inline std::vector<InkSample> closedPathForTest(const std::vector<InkSample> &samples)
{
    std::vector<InkSample> path = samples;
    if (path.empty())
        return path;
    const auto &first = path.front();
    const auto &last = path.back();
    if (first.x != last.x || first.y != last.y) {
        InkSample close = first;
        path.push_back(close);
    }
    return path;
}

inline double fractionInsidePolygon(const std::vector<InkSample> &samples,
                                    const std::vector<InkSample> &poly)
{
    if (samples.empty())
        return 0;
    int n = 0;
    for (const auto &s : samples) {
        if (pointInPolygonEvenOdd(s.x, s.y, poly))
            ++n;
    }
    return static_cast<double>(n) / static_cast<double>(samples.size());
}

inline bool qualifiesAsSurround(const DocNode &candidate, const std::vector<const DocNode *> &others)
{
    if (candidate.kind != NodeKind::Ink)
        return false;
    const auto poly = closedPathForTest(candidate.samples);
    if (poly.size() < 4)
        return false;
    for (const DocNode *o : others) {
        if (!o || o->kind != NodeKind::Ink)
            continue;
        if (fractionInsidePolygon(o->samples, poly) < 0.8)
            return false;
    }
    return true;
}

inline bool pointInAabb(double x, double y, const SmartBounds &r)
{
    return x >= r.x && x <= r.x + r.width && y >= r.y && y <= r.y + r.height;
}

inline double fractionInsideRect(const std::vector<InkSample> &samples, const SmartBounds &r)
{
    if (samples.empty())
        return 0;
    int n = 0;
    for (const auto &s : samples) {
        if (pointInAabb(s.x, s.y, r))
            ++n;
    }
    return static_cast<double>(n) / static_cast<double>(samples.size());
}

inline double aabbOverlapFraction(const SmartBounds &node, const SmartBounds &sel)
{
    const double ix = std::max(0.0, std::min(node.x + node.width, sel.x + sel.width)
                                       - std::max(node.x, sel.x));
    const double iy = std::max(0.0, std::min(node.y + node.height, sel.y + sel.height)
                                       - std::max(node.y, sel.y));
    const double area = std::max(0.0, node.width) * std::max(0.0, node.height);
    if (area <= 1e-9)
        return 0;
    return (ix * iy) / area;
}

inline double fractionAabbInsidePolygon(const SmartBounds &b, const std::vector<InkSample> &poly)
{
    int hit = 0;
    int n = 0;
    for (int gy = 0; gy < 5; ++gy) {
        for (int gx = 0; gx < 5; ++gx) {
            const double x = b.x + b.width * (gx + 0.5) / 5.0;
            const double y = b.y + b.height * (gy + 0.5) / 5.0;
            ++n;
            if (pointInPolygonEvenOdd(x, y, poly))
                ++hit;
        }
    }
    return n == 0 ? 0 : static_cast<double>(hit) / static_cast<double>(n);
}

inline bool nodeWorldAabb(const DocNode &n, SmartBounds &out)
{
    if (n.kind == NodeKind::Ink) {
        if (n.samples.size() < 1)
            return false;
        out = samplesAabb(n.samples);
        return true;
    }
    if (n.kind == NodeKind::SmartGroup) {
        out = smartGroupWorldBounds(n);
        return out.width >= 0 && out.height >= 0;
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
    if (n.kind == NodeKind::Frame) {
        out.x = n.bounds.minX;
        out.y = n.bounds.minY;
        out.width = n.bounds.maxX - n.bounds.minX;
        out.height = n.bounds.maxY - n.bounds.minY;
        return true;
    }
    return false;
}

/** Top-level pickable nodes (not ToolChip; not children of SmartGroup). */
inline void collectPickable(const std::vector<DocNode> &nodes, std::vector<const DocNode *> &out)
{
    for (const auto &n : nodes) {
        if (n.kind == NodeKind::SmartGroup) {
            out.push_back(&n);
            continue;
        }
        if (n.kind == NodeKind::Ink || n.kind == NodeKind::Text || n.kind == NodeKind::Primitive
            || n.kind == NodeKind::Frame) {
            out.push_back(&n);
        }
        if (n.kind == NodeKind::Group)
            collectPickable(n.children, out);
    }
}

inline std::vector<std::string> selectByRect(const DeviceDocument &doc, const SmartBounds &rect)
{
    std::vector<const DocNode *> pick;
    collectPickable(doc.rootChildren, pick);
    std::vector<std::string> ids;
    for (const DocNode *n : pick) {
        if (n->kind == NodeKind::Ink) {
            if (fractionInsideRect(n->samples, rect) >= 0.8)
                ids.push_back(n->id);
            continue;
        }
        SmartBounds b;
        if (!nodeWorldAabb(*n, b))
            continue;
        if (aabbOverlapFraction(b, rect) >= 0.8)
            ids.push_back(n->id);
    }
    return ids;
}

inline std::vector<std::string> selectByFreeform(const DeviceDocument &doc,
                                                 const std::vector<InkSample> &polyline)
{
    const auto poly = closedPathForTest(polyline);
    std::vector<const DocNode *> pick;
    collectPickable(doc.rootChildren, pick);
    std::vector<std::string> ids;
    for (const DocNode *n : pick) {
        if (n->kind == NodeKind::Ink) {
            if (fractionInsidePolygon(n->samples, poly) >= 0.8)
                ids.push_back(n->id);
            continue;
        }
        SmartBounds b;
        if (!nodeWorldAabb(*n, b))
            continue;
        if (fractionAabbInsidePolygon(b, poly) >= 0.8)
            ids.push_back(n->id);
    }
    return ids;
}

inline bool unionAabbOfIds(const DeviceDocument &doc, const std::vector<std::string> &ids,
                           SmartBounds &out)
{
    bool any = false;
    double minX = 0, minY = 0, maxX = 0, maxY = 0;
    for (const auto &id : ids) {
        const DocNode *n = doc.find(id);
        if (!n)
            continue;
        SmartBounds b;
        if (!nodeWorldAabb(*n, b))
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

inline void listFreeInkPaintOrder(const std::vector<DocNode> &nodes, std::vector<std::string> &out)
{
    for (const auto &n : nodes) {
        if (n.kind == NodeKind::Ink)
            out.push_back(n.id);
        if (n.kind == NodeKind::Frame || n.kind == NodeKind::Group)
            listFreeInkPaintOrder(n.children, out);
    }
}

/**
 * Create Smart Group from selected node ids, or refuse (no tree mutation).
 * SmartGroup in selection → refuse (CHL-0011).
 */
inline SelectionCreateResult createSmartGroupFromSelection(DeviceDocument &doc,
                                                           const std::vector<std::string> &selectedIds)
{
    SelectionCreateResult out;
    for (const auto &id : selectedIds) {
        const DocNode *n = doc.find(id);
        if (n && n->kind == NodeKind::SmartGroup) {
            out.reason = "smartgroup_in_selection";
            return out;
        }
    }

    std::vector<std::string> paint;
    listFreeInkPaintOrder(doc.rootChildren, paint);
    std::vector<const DocNode *> orderedInks;
    for (const auto &id : paint) {
        if (std::find(selectedIds.begin(), selectedIds.end(), id) == selectedIds.end())
            continue;
        const DocNode *n = doc.find(id);
        if (n && n->kind == NodeKind::Ink)
            orderedInks.push_back(n);
    }
    if (orderedInks.size() < 2) {
        out.reason = "need_at_least_two";
        return out;
    }

    std::vector<const DocNode *> qualifiers;
    for (const DocNode *cand : orderedInks) {
        std::vector<const DocNode *> others;
        for (const DocNode *o : orderedInks) {
            if (o->id != cand->id)
                others.push_back(o);
        }
        if (qualifiesAsSurround(*cand, others))
            qualifiers.push_back(cand);
    }
    if (qualifiers.empty()) {
        out.reason = "no_surround";
        return out;
    }
    const DocNode *winner = qualifiers.back();
    const std::string winnerId = winner->id;
    const SmartBounds world = samplesAabb(winner->samples);

    SmartBounds bounds;
    bounds.x = 0;
    bounds.y = 0;
    bounds.width = world.width;
    bounds.height = world.height;

    std::vector<DocNode> children;
    std::vector<std::string> captureIds;
    std::vector<InkSample> boundary = winner->samples;
    for (auto &s : boundary) {
        s.x -= world.x;
        s.y -= world.y;
    }
    children.push_back(makeInkChild(winnerId, "boundary", boundary, winner->style, std::nullopt));

    for (const DocNode *ink : orderedInks) {
        captureIds.push_back(ink->id);
        if (ink->id == winnerId)
            continue;
        std::vector<InkSample> local = ink->samples;
        for (auto &s : local) {
            s.x -= world.x;
            s.y -= world.y;
        }
        const auto uv = seedLayoutOffset(local, bounds);
        children.push_back(makeInkChild(ink->id, "content", local, ink->style, uv));
    }

    std::vector<std::string> capturedIds;
    capturedIds.reserve(orderedInks.size());
    for (const DocNode *ink : orderedInks)
        capturedIds.push_back(ink->id);

    const std::string smartGroupId = doc.generateNodeId();
    CreateSmartGroupEdit edit;
    edit.setId(std::string("create_smart_group:") + smartGroupId);
    edit.setSource("epaper");
    edit.setNodeId(smartGroupId);
    edit.setBounds(bounds);
    SmartTransform xf;
    xf.x = world.x;
    xf.y = world.y;
    edit.setTransform(xf);
    edit.setInkScaleMode("fixedInk");
    edit.setCaptureIds(std::move(captureIds));
    edit.setChildren(std::move(children));
    edit.setBoundaryPolyline(closedPolylineCopy(boundary));

    const ApplyResult r = doc.commitEdit(edit);
    if (!r.applied) {
        out.reason = r.reason.empty() ? "create_failed" : r.reason;
        return out;
    }
    out.created = true;
    out.smartGroupId = smartGroupId;
    out.boundaryId = winnerId;
    out.childIds = std::move(capturedIds);
    return out;
}

} // namespace document
} // namespace epaper
