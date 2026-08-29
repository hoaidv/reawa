#pragma once

/**
 * Remnant split → DocEdits (set_ink_samples / append_ink / remove_node).
 * @implements [SRS-EP-55] remnant commit plan
 * @implements [ADR-0034] longest keeps id; extras append_ink; 0 → remove
 */

#include "device_document.hpp"
#include "erase_clip.hpp"
#include "operations/append_ink_edit.hpp"
#include "operations/remove_node_edit.hpp"
#include "operations/set_ink_samples_edit.hpp"
#include "recognize_enclose.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace epaper {
namespace document {

inline std::vector<ErasePt> samplesToErasePts(const std::vector<InkSample> &s)
{
    std::vector<ErasePt> out;
    out.reserve(s.size());
    for (const auto &p : s)
        out.push_back({p.x, p.y});
    return out;
}

inline std::vector<InkSample> worldInkSamples(const DocNode &ink, const DocNode *sg)
{
    if (!sg)
        return ink.samples;
    const std::string role = ink.role ? *ink.role : std::string("content");
    std::vector<InkSample> out = ink.samples;
    for (auto &s : out) {
        const Vec2 w = smartLocalToWorld(s.x, s.y, *sg, role, ink.layoutOffset, nullptr);
        s.x = w.x;
        s.y = w.y;
    }
    return out;
}

inline std::vector<InkSample> localInkSamples(const std::vector<InkSample> &world, const DocNode *sg,
                                              const std::string &role)
{
    if (!sg)
        return world;
    std::vector<InkSample> out = world;
    for (auto &s : out) {
        const Vec2 loc = smartWorldToLocal(s.x, s.y, *sg, role);
        s.x = loc.x;
        s.y = loc.y;
    }
    return out;
}

struct EraseInkRef {
    const DocNode *ink = nullptr;
    const DocNode *smartGroup = nullptr;
    std::string parentId;
    int index = 0;
};

inline void collectEraseInks(const std::vector<DocNode> &nodes, const std::string &parentId,
                             const DocNode *sg, bool underConnector,
                             std::vector<EraseInkRef> *out)
{
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        const DocNode &n = nodes[static_cast<size_t>(i)];
        if (n.kind == NodeKind::Connector)
            continue;
        if (n.kind == NodeKind::Ink && !underConnector) {
            EraseInkRef r;
            r.ink = &n;
            r.smartGroup = sg;
            r.parentId = parentId;
            r.index = i;
            out->push_back(r);
            continue;
        }
        if (n.kind == NodeKind::SmartGroup)
            collectEraseInks(n.children, n.id, &n, false, out);
        else if (n.kind == NodeKind::Frame || n.kind == NodeKind::Group)
            collectEraseInks(n.children, n.id, sg, underConnector, out);
    }
}

inline std::unique_ptr<AppendInkEdit> makeRemnantAppend(const std::string &opId,
                                                        const std::string &nodeId,
                                                        const std::string &parentId,
                                                        const std::string &insertAfter,
                                                        const DocNode &src,
                                                        std::vector<InkSample> samples)
{
    auto e = std::make_unique<AppendInkEdit>();
    e->setId(opId);
    e->setNodeId(nodeId);
    if (!parentId.empty())
        e->setParentId(parentId);
    e->setInsertAfter(insertAfter);
    e->setStyle(src.style);
    e->setRole(src.role);
    e->setSamples(std::move(samples));
    return e;
}

/** `{base}_rN` unused in the tree and this gesture. @implements [SRS-EP-55] remnant ids */
inline std::string allocEraseRemnantId(const DeviceDocument &doc, const std::string &base,
                                       std::unordered_set<std::string> *used)
{
    for (int n = 1; n < 1000000; ++n) {
        const std::string nid = base + "_r" + std::to_string(n);
        if (used->count(nid) || doc.find(nid))
            continue;
        used->insert(nid);
        return nid;
    }
    return base + "_rx";
}

/**
 * Plan clip edits against the current tree. Empty vector = no-op (0 undo).
 * Clip never mutates SmartGroup::boundaryPolyline.
 */
inline std::vector<std::unique_ptr<DocEdit>> planEraseEdits(DeviceDocument &doc,
                                                            const std::string &opId,
                                                            const ClipRegion &region)
{
    std::vector<std::unique_ptr<DocEdit>> parts;
    std::vector<EraseInkRef> inks;
    collectEraseInks(doc.rootChildren, "", nullptr, false, &inks);
    const EraseAabb regionBox = clipRegionAabb(region);

    std::unordered_map<std::string, int> sgInkRemain;
    for (const auto &ref : inks) {
        if (ref.smartGroup)
            sgInkRemain[ref.smartGroup->id] += 1;
    }

    std::unordered_set<std::string> remnantIds;
    for (const auto &ref : inks) {
        const DocNode &ink = *ref.ink;
        const std::string role = ink.role ? *ink.role : std::string("content");
        const std::vector<InkSample> world = worldInkSamples(ink, ref.smartGroup);
        if (!samplesOverlapAabb(world, regionBox))
            continue;
        const ClipResult clipped = clipInkPolyline(world, region);
        if (!clipped.hit)
            continue;

        std::vector<std::vector<InkSample>> remnants = clipped.remnants;
        for (auto &rem : remnants)
            rem = localInkSamples(rem, ref.smartGroup, role);

        if (remnants.empty()) {
            parts.push_back(makeRemoveEdit(opId, ink.id));
            if (ref.smartGroup)
                sgInkRemain[ref.smartGroup->id] -= 1;
            continue;
        }

        size_t longest = 0;
        double bestLen = polylineArcLength(remnants[0]);
        for (size_t i = 1; i < remnants.size(); ++i) {
            const double len = polylineArcLength(remnants[i]);
            if (len > bestLen) {
                bestLen = len;
                longest = i;
            }
        }

        auto set = std::make_unique<SetInkSamplesEdit>(ink.id, remnants[longest]);
        set->setId(opId);
        parts.push_back(std::move(set));

        std::string afterId = ink.id;
        for (size_t i = 0; i < remnants.size(); ++i) {
            if (i == longest)
                continue;
            const std::string nid = allocEraseRemnantId(doc, ink.id, &remnantIds);
            auto e = makeRemnantAppend(opId, nid, ref.parentId, afterId, ink, remnants[i]);
            if (ref.smartGroup && role == "content")
                e->setLayoutOffset(seedLayoutOffset(remnants[i], ref.smartGroup->smartBounds));
            parts.push_back(std::move(e));
            afterId = nid;
        }
    }

    for (const auto &kv : sgInkRemain) {
        if (kv.second > 0)
            continue;
        const DocNode *sg = doc.find(kv.first);
        if (sg && sg->kind == NodeKind::SmartGroup)
            parts.push_back(makeRemoveEdit(opId, kv.first));
    }
    return parts;
}

inline ApplyResult commitEraseRegion(DeviceDocument &doc, const std::string &opId,
                                     const ClipRegion &region)
{
    auto parts = planEraseEdits(doc, opId, region);
    if (parts.empty())
        return {true, "noop"};
    return doc.commitGesture(opId, std::move(parts));
}

} // namespace document
} // namespace epaper
