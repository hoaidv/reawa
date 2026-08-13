#pragma once
/**
 * Draw-into membership — ordinary Pen ink joins an existing Smart Group.
 * @implements [SRS-EP-10] draw-into membership
 * @implements [SRS-EP-14] membership latency budget (caller times pen-up)
 *
 * Port of infini/src/document/membership.ts. Never runs on an Ink-box enclose stroke.
 */

#include "device_document.hpp"
#include "recognize_enclose.hpp"

#include <string>
#include <vector>

namespace epaper {
namespace document {

enum class MembershipKind {
    Joined,
    None,
};

struct MembershipResult {
    MembershipKind kind = MembershipKind::None;
    std::string reason;
    std::string smartGroupId;
    std::string inkId;
};

/** World AABB of SmartGroup geometric bounds after translate + scale (v0). */
inline SmartBounds smartGroupWorldBounds(const DocNode &sg)
{
    SmartBounds w;
    const SmartBounds &b = sg.smartBounds;
    const SmartTransform &t = sg.transform;
    w.x = t.x + b.x * t.scaleX;
    w.y = t.y + b.y * t.scaleY;
    w.width = b.width * t.scaleX;
    w.height = b.height * t.scaleY;
    return w;
}

/** Paint order: tree walk, later siblings last. */
inline void smartGroupsInPaintOrder(const std::vector<DocNode> &nodes, std::vector<const DocNode *> &out)
{
    for (const auto &n : nodes) {
        if (n.kind == NodeKind::SmartGroup)
            out.push_back(&n);
        if (n.kind == NodeKind::Frame || n.kind == NodeKind::Group)
            smartGroupsInPaintOrder(n.children, out);
    }
}

inline bool inkAlreadyMember(const std::vector<const DocNode *> &groups, const std::string &inkId)
{
    for (const DocNode *sg : groups) {
        for (const auto &c : sg->children) {
            if (c.id == inkId)
                return true;
        }
    }
    return false;
}

/**
 * After ordinary ink is committed, try reparent into a Smart Group.
 * @implements [SRS-EP-10] membership on pen-up (Pen latch only)
 */
inline MembershipResult tryDrawIntoMembership(DeviceDocument &doc, const std::string &inkId)
{
    MembershipResult out;
    out.inkId = inkId;

    const DocNode *node = doc.find(inkId);
    if (!node || node->kind != NodeKind::Ink) {
        out.reason = "not_ink";
        return out;
    }

    std::vector<const DocNode *> groups;
    smartGroupsInPaintOrder(doc.rootChildren, groups);
    if (inkAlreadyMember(groups, inkId)) {
        out.reason = "already_member";
        return out;
    }

    std::vector<const DocNode *> qualifiers;
    for (const DocNode *sg : groups) {
        if (fractionSamplesInside(node->samples, smartGroupWorldBounds(*sg)) >= 0.8)
            qualifiers.push_back(sg);
    }
    if (qualifiers.empty()) {
        out.reason = "no_qualifying_group";
        return out;
    }
    const DocNode *winner = qualifiers.back();

    DocOp op;
    op.opId = std::string("join_smart_group:") + inkId + ":" + winner->id;
    op.type = "join_smart_group";
    op.source = "epaper";
    JsonValue::Object payload;
    payload.emplace_back("inkId", JsonValue::string(inkId));
    payload.emplace_back("smartGroupId", JsonValue::string(winner->id));
    op.payload = JsonValue::object(std::move(payload));

    const ApplyResult r = doc.commitOp(op);
    if (!r.applied) {
        out.reason = r.reason.empty() ? "join_failed" : r.reason;
        return out;
    }
    out.kind = MembershipKind::Joined;
    out.smartGroupId = winner->id;
    out.reason = "joined";
    return out;
}

} // namespace document
} // namespace epaper
