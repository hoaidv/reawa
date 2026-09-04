#pragma once
/**
 * Draw-into membership — ordinary Pen ink joins an existing Smart Group.
 * @implements [SRS-EP-10] draw-into membership
 * @implements [SRS-EP-14] membership latency budget (caller times pen-up)
 *
 * Port of infini/src/document/membership.ts. ADR-0022 step 2: after endpoint-ink,
 * before enclose. Qualify by polyline length inside boundary ink (not AABB samples).
 */

#include "device_document.hpp"
#include "recognize_enclose.hpp"

#include <memory>
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
 * Highest-paint SmartGroup whose boundary ink contains ≥80% of stroke length.
 * @implements [SRS-EP-10] draw-into membership length-in-boundary
 */
inline const DocNode *qualifyingMembershipGroup(const DeviceDocument &doc,
                                                const std::vector<InkSample> &samples)
{
    std::vector<const DocNode *> groups;
    smartGroupsInPaintOrder(doc.rootChildren, groups);
    const DocNode *winner = nullptr;
    for (const DocNode *sg : groups) {
        if (fractionStrokeInsideBoundary(*sg, samples) >= 0.8)
            winner = sg;
    }
    return winner;
}

/**
 * After ordinary ink is committed, try reparent into a Smart Group.
 * @implements [SRS-EP-10] membership on pen-up
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

    const DocNode *winner = qualifyingMembershipGroup(doc, node->samples);
    if (!winner) {
        out.reason = "no_qualifying_group";
        return out;
    }

    JoinSmartGroupEdit edit(inkId, winner->id);
    edit.setId(std::string("join_smart_group:") + inkId + ":" + winner->id);
    edit.setSource("epaper");
    const ApplyResult r = doc.commitEdit(edit);
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
