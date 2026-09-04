#pragma once

/**
 * Process-global in-document clipboard: slot + clone / paste algebra.
 * @implements [SRS-EP-31] clipboard singleton clone paste
 * @implements [ADR-0037] device clipboard singleton
 * @fix [CHL-0031] tap-origin paste into ink-box
 */

#include "document/device_document.hpp"
#include "document/doc_model.hpp"
#include "document/surround_create.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace epaper {
namespace tools {

struct ClipboardSlot {
    std::vector<epaper::document::DocNode> nodes;
    std::vector<std::string> sourceIds;
    int seq = 0;

    bool empty() const { return nodes.empty(); }
    void clear()
    {
        nodes.clear();
        sourceIds.clear();
    }
    void reset()
    {
        clear();
        seq = 0;
    }
    std::string nextOpId() { return std::string("clip-") + std::to_string(++seq); }
};

inline ClipboardSlot &clipboard()
{
    static ClipboardSlot slot;
    return slot;
}

namespace clipops {

inline bool isLegalParentKind(epaper::document::NodeKind k)
{
    using epaper::document::NodeKind;
    return k == NodeKind::SmartGroup || k == NodeKind::Frame || k == NodeKind::Group;
}

inline void collectIds(const epaper::document::DocNode &n, std::vector<std::string> *out)
{
    out->push_back(n.id);
    for (const auto &c : n.children)
        collectIds(c, out);
}

inline bool subtreeHasSelected(const epaper::document::DocNode &n,
                               const std::unordered_set<std::string> &selected)
{
    if (selected.count(n.id))
        return true;
    for (const auto &c : n.children) {
        if (subtreeHasSelected(c, selected))
            return true;
    }
    return false;
}

inline bool anyDescendantSelected(const epaper::document::DocNode &n,
                                  const std::unordered_set<std::string> &selected)
{
    for (const auto &c : n.children) {
        if (selected.count(c.id) || anyDescendantSelected(c, selected))
            return true;
    }
    return false;
}

/** @fix [CHL-0031] full subtree when only the root is selected */
inline epaper::document::DocNode filterSelected(const epaper::document::DocNode &n,
                                                const std::unordered_set<std::string> &selected)
{
    if (selected.count(n.id) && !anyDescendantSelected(n, selected))
        return n;
    epaper::document::DocNode out = n;
    out.children.clear();
    for (const auto &c : n.children) {
        if (!subtreeHasSelected(c, selected))
            continue;
        out.children.push_back(filterSelected(c, selected));
    }
    return out;
}

inline bool isDescendantOf(const epaper::document::DeviceDocument &doc, const std::string &anc,
                           const std::string &id)
{
    epaper::document::DeviceDocument::NodePlace pl;
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

inline std::vector<std::string> slotRoots(const epaper::document::DeviceDocument &doc,
                                          const std::vector<std::string> &selected)
{
    std::vector<std::string> roots;
    for (const auto &id : selected) {
        if (!doc.find(id))
            continue;
        bool nested = false;
        for (const auto &other : selected) {
            if (other == id)
                continue;
            if (isDescendantOf(doc, other, id)) {
                nested = true;
                break;
            }
        }
        if (!nested)
            roots.push_back(id);
    }
    return roots;
}

inline void copyToSlot(const epaper::document::DeviceDocument &doc,
                       const std::vector<std::string> &selected, ClipboardSlot &slot)
{
    slot.clear();
    std::unordered_set<std::string> sel(selected.begin(), selected.end());
    for (const auto &id : slotRoots(doc, selected)) {
        const epaper::document::DocNode *n = doc.find(id);
        if (!n)
            continue;
        slot.nodes.push_back(filterSelected(*n, sel));
    }
    for (const auto &n : slot.nodes)
        collectIds(n, &slot.sourceIds);
}

inline bool unionAabb(const std::vector<epaper::document::DocNode> &nodes,
                      epaper::document::SmartBounds &out)
{
    bool any = false;
    for (const auto &n : nodes) {
        epaper::document::SmartBounds b;
        if (!epaper::document::nodeWorldAabb(n, b))
            continue;
        if (!any) {
            out = b;
            any = true;
            continue;
        }
        const double minX = std::min(out.x, b.x);
        const double minY = std::min(out.y, b.y);
        const double maxX = std::max(out.x + out.width, b.x + b.width);
        const double maxY = std::max(out.y + out.height, b.y + b.height);
        out.x = minX;
        out.y = minY;
        out.width = maxX - minX;
        out.height = maxY - minY;
    }
    return any;
}

inline void translateNode(epaper::document::DocNode &n, double dx, double dy)
{
    using epaper::document::NodeKind;
    if (n.kind == NodeKind::SmartGroup) {
        n.transform.x += dx;
        n.transform.y += dy;
        return;
    }
    for (auto &s : n.samples) {
        s.x += dx;
        s.y += dy;
    }
    for (auto &s : n.boundaryPolyline) {
        s.x += dx;
        s.y += dy;
    }
    n.transform.x += dx;
    n.transform.y += dy;
    n.smartBounds.x += dx;
    n.smartBounds.y += dy;
    n.box.minX += dx;
    n.box.minY += dy;
    n.box.maxX += dx;
    n.box.maxY += dy;
    n.bounds.minX += dx;
    n.bounds.minY += dy;
    n.bounds.maxX += dx;
    n.bounds.maxY += dy;
    n.gx += dx;
    n.gy += dy;
    n.x1 += dx;
    n.y1 += dy;
    n.x2 += dx;
    n.y2 += dy;
    n.cx += dx;
    n.cy += dy;
    for (auto &p : n.restSpine) {
        p.x += dx;
        p.y += dy;
    }
    for (auto &p : n.warpedSamples) {
        p.x += dx;
        p.y += dy;
    }
    n.fromPose.x += dx;
    n.fromPose.y += dy;
    n.toPose.x += dx;
    n.toPose.y += dy;
    for (auto &c : n.children)
        translateNode(c, dx, dy);
}

inline void remintTree(epaper::document::DeviceDocument &doc, epaper::document::DocNode &n,
                       std::unordered_map<std::string, std::string> *map)
{
    const std::string old = n.id;
    n.id = doc.generateNodeId();
    (*map)[old] = n.id;
    n.lastOpId.clear();
    for (auto &c : n.children)
        remintTree(doc, c, map);
}

inline void remapRefs(epaper::document::DocNode &n,
                      const std::unordered_map<std::string, std::string> &map)
{
    auto mapId = [&](std::string &id) {
        const auto it = map.find(id);
        if (it != map.end())
            id = it->second;
    };
    mapId(n.fromNodeId);
    mapId(n.toNodeId);
    mapId(n.fromAnchor.nodeId);
    mapId(n.toAnchor.nodeId);
    for (auto &c : n.children)
        remapRefs(c, map);
}

inline double aabbArea(const epaper::document::SmartBounds &b)
{
    return std::max(0.0, b.width) * std::max(0.0, b.height);
}

inline double aabbOverlapArea(const epaper::document::SmartBounds &a,
                              const epaper::document::SmartBounds &b)
{
    const double x0 = std::max(a.x, b.x);
    const double y0 = std::max(a.y, b.y);
    const double x1 = std::min(a.x + a.width, b.x + b.width);
    const double y1 = std::min(a.y + a.height, b.y + b.height);
    if (x1 <= x0 || y1 <= y0)
        return 0;
    return (x1 - x0) * (y1 - y0);
}

inline bool naturalBoundary(const epaper::document::DocNode &n, epaper::document::SmartBounds &out)
{
    using epaper::document::NodeKind;
    if (n.kind == NodeKind::Frame || n.kind == NodeKind::SmartGroup || n.kind == NodeKind::Ink
        || n.kind == NodeKind::Primitive || n.kind == NodeKind::Text)
        return epaper::document::nodeWorldAabb(n, out);
    if (n.kind == NodeKind::Group) {
        bool any = false;
        for (const auto &c : n.children) {
            epaper::document::SmartBounds b;
            if (!naturalBoundary(c, b))
                continue;
            if (!any) {
                out = b;
                any = true;
                continue;
            }
            const double minX = std::min(out.x, b.x);
            const double minY = std::min(out.y, b.y);
            const double maxX = std::max(out.x + out.width, b.x + b.width);
            const double maxY = std::max(out.y + out.height, b.y + b.height);
            out.x = minX;
            out.y = minY;
            out.width = maxX - minX;
            out.height = maxY - minY;
        }
        return any;
    }
    return false;
}

inline double inkLength(const epaper::document::DocNode &n)
{
    if (n.samples.size() < 2)
        return 0;
    double len = 0;
    for (size_t i = 1; i < n.samples.size(); ++i) {
        const double dx = n.samples[i].x - n.samples[i - 1].x;
        const double dy = n.samples[i].y - n.samples[i - 1].y;
        len += std::hypot(dx, dy);
    }
    return len;
}

inline double inkLengthInside(const epaper::document::DocNode &n,
                              const epaper::document::SmartBounds &cand)
{
    if (n.samples.size() < 2)
        return 0;
    auto inside = [&](const epaper::document::InkSample &s) {
        return s.x >= cand.x && s.x <= cand.x + cand.width && s.y >= cand.y
            && s.y <= cand.y + cand.height;
    };
    double len = 0;
    for (size_t i = 1; i < n.samples.size(); ++i) {
        if (inside(n.samples[i - 1]) && inside(n.samples[i])) {
            const double dx = n.samples[i].x - n.samples[i - 1].x;
            const double dy = n.samples[i].y - n.samples[i - 1].y;
            len += std::hypot(dx, dy);
        }
    }
    return len;
}

inline bool overlaps20(const epaper::document::DocNode &item,
                       const epaper::document::DocNode &candidate)
{
    epaper::document::SmartBounds candB;
    if (!naturalBoundary(candidate, candB))
        return false;
    using epaper::document::NodeKind;
    if (item.kind == NodeKind::Ink) {
        const double total = inkLength(item);
        if (total <= 1e-9)
            return false;
        return inkLengthInside(item, candB) / total >= 0.20;
    }
    epaper::document::SmartBounds itemB;
    if (!naturalBoundary(item, itemB))
        return false;
    const double area = aabbArea(itemB);
    if (area <= 1e-9)
        return false;
    return aabbOverlapArea(itemB, candB) / area >= 0.20;
}

inline std::vector<std::string> ancestorChain(const epaper::document::DeviceDocument &doc,
                                              const std::string &id)
{
    std::vector<std::string> chain;
    if (id.empty())
        return chain;
    chain.push_back(id);
    epaper::document::DeviceDocument::NodePlace pl;
    std::string cur = id;
    while (doc.findPlace(cur, &pl) && !pl.parentId.empty()) {
        chain.push_back(pl.parentId);
        cur = pl.parentId;
    }
    return chain;
}

inline std::string chooseParent(const epaper::document::DeviceDocument &doc,
                                const epaper::document::DocNode &item,
                                const std::string &hitId)
{
    const epaper::document::DocNode *hit = doc.find(hitId);
    if (hit && isLegalParentKind(hit->kind))
        return hitId;
    for (const auto &cid : ancestorChain(doc, hitId)) {
        const epaper::document::DocNode *c = doc.find(cid);
        if (!c || !isLegalParentKind(c->kind))
            continue;
        if (overlaps20(item, *c))
            return cid;
    }
    return {};
}

inline bool isLiveSource(const epaper::document::DeviceDocument &doc, const ClipboardSlot &slot,
                         const std::string &id)
{
    if (id.empty() || !doc.find(id))
        return false;
    return std::find(slot.sourceIds.begin(), slot.sourceIds.end(), id) != slot.sourceIds.end();
}

/** Join-style world → SmartGroup local samples + layoutOffset. */
inline void localizeInkIntoSmartGroup(epaper::document::DocNode &ink,
                                      const epaper::document::DocNode &sg)
{
    const epaper::document::SmartTransform &t = sg.transform;
    const bool fixedInk = sg.inkScaleMode == "fixedInk";
    const double sx = t.scaleX != 0 ? t.scaleX : 1.0;
    const double sy = t.scaleY != 0 ? t.scaleY : 1.0;
    for (auto &s : ink.samples) {
        if (fixedInk) {
            s.x = s.x - t.x;
            s.y = s.y - t.y;
        } else {
            s.x = (s.x - t.x) / sx;
            s.y = (s.y - t.y) / sy;
        }
    }
    ink.role = "content";
    const double w = sg.smartBounds.width != 0 ? sg.smartBounds.width : 1.0;
    const double h = sg.smartBounds.height != 0 ? sg.smartBounds.height : 1.0;
    double minX = std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    for (const auto &s : ink.samples) {
        minX = std::min(minX, s.x);
        minY = std::min(minY, s.y);
        maxX = std::max(maxX, s.x);
        maxY = std::max(maxY, s.y);
    }
    const double cx = std::isfinite(minX) ? (minX + maxX) / 2.0 : 0.0;
    const double cy = std::isfinite(minY) ? (minY + maxY) / 2.0 : 0.0;
    ink.layoutOffset = {(cx - sg.smartBounds.x) / w, (cy - sg.smartBounds.y) / h};
}

constexpr const char *kPasteOntoOriginals = "Cannot paste onto the copied original";

enum class PasteRefuse { None, Empty, LiveOriginal, NoBounds };

struct PasteOutcome {
    epaper::document::ApplyResult result;
    PasteRefuse refuse = PasteRefuse::None;
};

inline epaper::document::ApplyResult commitCut(epaper::document::DeviceDocument &doc,
                                               ClipboardSlot &slot,
                                               const std::vector<std::string> &selected,
                                               bool enqueue)
{
    copyToSlot(doc, selected, slot);
    const std::vector<std::string> roots = slotRoots(doc, selected);
    if (roots.empty())
        return {false, "empty"};
    std::vector<std::unique_ptr<epaper::document::DocEdit>> parts;
    for (const auto &id : roots) {
        auto e = std::make_unique<epaper::document::RemoveNodeEdit>(id);
        parts.push_back(std::move(e));
    }
    return doc.commitGesture(slot.nextOpId(), std::move(parts), enqueue);
}

inline PasteOutcome commitPaste(epaper::document::DeviceDocument &doc, ClipboardSlot &slot,
                                double pressWx, double pressWy, const std::string &hitId,
                                bool enqueue)
{
    if (slot.empty())
        return {{false, "empty"}, PasteRefuse::Empty};
    std::vector<epaper::document::DocNode> clones = slot.nodes;
    epaper::document::SmartBounds u;
    if (!unionAabb(clones, u))
        return {{false, "no_bounds"}, PasteRefuse::NoBounds};
    const double dx = pressWx - u.x;
    const double dy = pressWy - u.y;
    for (auto &n : clones)
        translateNode(n, dx, dy);
    std::unordered_map<std::string, std::string> idMap;
    for (auto &n : clones)
        remintTree(doc, n, &idMap);
    for (auto &n : clones)
        remapRefs(n, idMap);
    if (isLiveSource(doc, slot, hitId))
        return {{false, "live_originals"}, PasteRefuse::LiveOriginal};
    std::vector<std::string> parents;
    parents.reserve(clones.size());
    for (const auto &n : clones) {
        const std::string parent = chooseParent(doc, n, hitId);
        if (isLiveSource(doc, slot, parent))
            return {{false, "live_originals"}, PasteRefuse::LiveOriginal};
        parents.push_back(parent);
    }
    std::vector<std::unique_ptr<epaper::document::DocEdit>> parts;
    for (size_t i = 0; i < clones.size(); ++i) {
        if (!parents[i].empty()) {
            const epaper::document::DocNode *p = doc.find(parents[i]);
            if (p && p->kind == epaper::document::NodeKind::SmartGroup
                && clones[i].kind == epaper::document::NodeKind::Ink)
                localizeInkIntoSmartGroup(clones[i], *p);
        }
        auto e = std::make_unique<epaper::document::ReparentEdit>(
            epaper::document::ReparentEdit::restore(clones[i].id, parents[i],
                                                    std::numeric_limits<int>::max(), clones[i]));
        parts.push_back(std::move(e));
    }
    const epaper::document::ApplyResult r =
        doc.commitGesture(slot.nextOpId(), std::move(parts), enqueue);
    return {r, PasteRefuse::None};
}

} // namespace clipops
} // namespace tools
} // namespace epaper
