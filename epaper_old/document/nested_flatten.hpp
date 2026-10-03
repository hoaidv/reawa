#pragma once
/**
 * Empty ink-box flatten (Rule 1). No recognize_enclose include — avoids cycles.
 * @implements [SRS-EP-75] flatten empty child
 * @fix [CHL-0032] letter-as-empty-box joins parent
 */

#include "affine.hpp"
#include "doc_model.hpp"

#include <string>
#include <utility>
#include <vector>

namespace epaper {
namespace document {

inline bool isEmptySmartGroup(const DocNode &n)
{
    if (n.kind != NodeKind::SmartGroup)
        return false;
    bool anyBoundary = false;
    for (const auto &c : n.children) {
        if (c.kind == NodeKind::SmartGroup)
            return false;
        if (c.kind == NodeKind::Ink) {
            const std::string role = c.role ? *c.role : std::string("content");
            if (role != "boundary")
                return false;
            anyBoundary = true;
        }
    }
    return anyBoundary;
}

inline void applyAffineToSamples(std::vector<InkSample> &samples, const Affine &m)
{
    for (auto &s : samples) {
        double x = 0;
        double y = 0;
        m.apply(s.x, s.y, &x, &y);
        s.x = x;
        s.y = y;
    }
}

inline std::pair<double, double> uvFromLocalSamples(const std::vector<InkSample> &samples,
                                                    const SmartBounds &bounds)
{
    double sx = 0;
    double sy = 0;
    int n = 0;
    for (const auto &s : samples) {
        sx += s.x;
        sy += s.y;
        ++n;
    }
    const double cx = n ? sx / double(n) : 0;
    const double cy = n ? sy / double(n) : 0;
    const double w = bounds.width != 0 ? bounds.width : 1.0;
    const double h = bounds.height != 0 ? bounds.height : 1.0;
    return {(cx - bounds.x) / w, (cy - bounds.y) / h};
}

/**
 * Bake empty SmartGroup own-transform into world, then shift into parent-local.
 * Caller adds the wrapper id to captureIds.
 */
inline void flattenEmptyToContentInks(const DocNode &empty, double worldOriginX, double worldOriginY,
                                      const SmartBounds &parentBounds, std::vector<DocNode> *children)
{
    if (!children || empty.kind != NodeKind::SmartGroup)
        return;
    const Affine own = affineOwn(empty.transform);
    for (const auto &c : empty.children) {
        if (c.kind != NodeKind::Ink)
            continue;
        DocNode ink = c;
        ink.role = std::string("content");
        applyAffineToSamples(ink.samples, own);
        for (auto &s : ink.samples) {
            s.x -= worldOriginX;
            s.y -= worldOriginY;
        }
        applyAffineToSamples(ink.boundaryPolyline, own);
        for (auto &s : ink.boundaryPolyline) {
            s.x -= worldOriginX;
            s.y -= worldOriginY;
        }
        ink.layoutOffset = uvFromLocalSamples(ink.samples, parentBounds);
        children->push_back(std::move(ink));
    }
}

/** Nested non-empty box: keep wrapper, shift translate into parent local. */
inline void remapSmartGroupToParentLocal(DocNode &sg, double worldOriginX, double worldOriginY)
{
    sg.transform.x -= worldOriginX;
    sg.transform.y -= worldOriginY;
}

/**
 * Append a captured SmartGroup as nested child or flattened content inks.
 * @implements [SRS-EP-75] nested capture + flatten
 */
inline void captureSmartGroupInto(const DocNode &sg, double worldOriginX, double worldOriginY,
                                  const SmartBounds &parentBounds, std::vector<DocNode> *children,
                                  std::vector<std::string> *captureIds)
{
    if (!children || !captureIds)
        return;
    captureIds->push_back(sg.id);
    if (isEmptySmartGroup(sg)) {
        flattenEmptyToContentInks(sg, worldOriginX, worldOriginY, parentBounds, children);
        return;
    }
    DocNode nested = sg;
    remapSmartGroupToParentLocal(nested, worldOriginX, worldOriginY);
    children->push_back(std::move(nested));
}

/** Map own-transform so world pose is unchanged under a new parent content ctx. */
inline void remapOwnIntoParentCtx(DocNode &sg, const Affine &oldCtx, const Affine &newParentContent)
{
    const Affine worldOwn = affineCompose(oldCtx, affineOwn(sg.transform));
    const Affine local = affineCompose(affineInverse(newParentContent), worldOwn);
    sg.transform.x = local.e;
    sg.transform.y = local.f;
}

} // namespace document
} // namespace epaper
