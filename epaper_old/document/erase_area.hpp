#pragma once

/**
 * Area erase: polygon clip of Ink + fully-inside remove of other kinds.
 * @implements [SRS-EP-57] area erase
 * @implements [STORY-EP-065] Area erase clip and fully-inside remove
 */

#include "erase_commit.hpp"
#include "manipulate.hpp"
#include "operations/remove_node_edit.hpp"

#include <cmath>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace epaper {
namespace document {

inline std::vector<ErasePt> autoCloseErasePoly(std::vector<ErasePt> poly)
{
    if (poly.size() < 2)
        return poly;
    const double dx = poly.back().x - poly.front().x;
    const double dy = poly.back().y - poly.front().y;
    if (std::hypot(dx, dy) > 1e-6)
        poly.push_back(poly.front());
    return poly;
}

inline bool aabbFullyInsidePolygon(const SmartBounds &b, const std::vector<ErasePt> &poly)
{
    if (poly.size() < 3 || b.width < 0 || b.height < 0)
        return false;
    const double xs[4] = {b.x, b.x + b.width, b.x + b.width, b.x};
    const double ys[4] = {b.y, b.y, b.y + b.height, b.y + b.height};
    for (int i = 0; i < 4; ++i) {
        if (!erasePointInPolygon(xs[i], ys[i], poly))
            return false;
    }
    return true;
}

inline bool samplesFullyInsidePolygon(const std::vector<InkSample> &s,
                                      const std::vector<ErasePt> &poly)
{
    if (s.empty() || poly.size() < 3)
        return false;
    for (const auto &p : s) {
        if (!erasePointInPolygon(p.x, p.y, poly))
            return false;
    }
    return true;
}

/**
 * Connector remove. REQ-14 attachments are not in the tree yet; body children
 * go with the connector. Undo is RemoveNodeEdit's inverse.
 */
inline void planConnectorRemove(const std::string &opId, const std::string &connectorId,
                                std::vector<std::unique_ptr<DocEdit>> *parts)
{
    parts->push_back(makeRemoveEdit(opId, connectorId));
}

inline bool nodeFullyInsideArea(const DocNode &n, const std::vector<ErasePt> &poly)
{
    if (n.kind == NodeKind::Frame)
        return false;
    if (n.kind == NodeKind::Connector)
        return samplesFullyInsidePolygon(connectorWorldPath(n), poly);
    SmartBounds wb;
    if (!boundsOf(n, wb))
        return false;
    return aabbFullyInsidePolygon(wb, poly);
}

inline void collectAreaRemoves(const std::vector<DocNode> &nodes, const std::vector<ErasePt> &poly,
                               std::vector<std::string> *removeIds,
                               std::unordered_set<std::string> *skipInkUnder)
{
    for (const auto &n : nodes) {
        if (n.kind == NodeKind::Frame) {
            collectAreaRemoves(n.children, poly, removeIds, skipInkUnder);
            continue;
        }
        if (n.kind == NodeKind::Ink)
            continue;
        if (nodeFullyInsideArea(n, poly)) {
            removeIds->push_back(n.id);
            skipInkUnder->insert(n.id);
            continue;
        }
        if (n.kind == NodeKind::Group)
            collectAreaRemoves(n.children, poly, removeIds, skipInkUnder);
    }
}

inline ApplyResult commitAreaErase(DeviceDocument &doc, const std::string &opId,
                                   std::vector<ErasePt> poly)
{
    poly = autoCloseErasePoly(std::move(poly));
    if (poly.size() < 3)
        return {true, "noop"};
    const ClipRegion region = polygonRegion(poly);

    std::vector<std::string> removeIds;
    std::unordered_set<std::string> skipInkUnder;
    collectAreaRemoves(doc.rootChildren, poly, &removeIds, &skipInkUnder);

    std::vector<std::unique_ptr<DocEdit>> parts;
    for (const auto &id : removeIds) {
        const DocNode *n = doc.find(id);
        if (n && n->kind == NodeKind::Connector)
            planConnectorRemove(opId, id, &parts);
        else
            parts.push_back(makeRemoveEdit(opId, id));
    }

    auto inkParts = planEraseEdits(doc, opId, region, &skipInkUnder);
    for (auto &e : inkParts)
        parts.push_back(std::move(e));

    if (parts.empty())
        return {true, "noop"};
    return doc.commitGesture(opId, std::move(parts));
}

} // namespace document
} // namespace epaper
