#include "rendering/rendering.hpp"

/**
 * Document → panel walk.
 * @implements [SRS-EP-76] nested RenderingContext + content AABB clip
 * @implements [STORY-EP-077] overflow neither painted nor hittable
 */

#include "document/affine.hpp"
#include "document/nested_inkbox.hpp"
#include "document/recognize_enclose.hpp"
#include "document/surround_create.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace epaper {
namespace render {
namespace {

using document::DocNode;
using document::NodeKind;
using document::PrimitiveKind;

bool aabbOverlap(const WorldAabb &a, double minX, double minY, double maxX, double maxY)
{
    if (!a.valid())
        return true; // no clip → everything visible
    return !(maxX < a.minX || minX > a.maxX || maxY < a.minY || minY > a.maxY);
}

bool smartBoundsOverlap(const WorldAabb &clip, const document::SmartBounds &b)
{
    const double maxX = b.x + b.width;
    const double maxY = b.y + b.height;
    const double minX = std::min(b.x, maxX);
    const double maxXn = std::max(b.x, maxX);
    const double minY = std::min(b.y, maxY);
    const double maxYn = std::max(b.y, maxY);
    return aabbOverlap(clip, minX, minY, maxXn, maxYn);
}

void boundsFromWorldPoints(double minX, double minY, double maxX, double maxY,
                           document::SmartBounds *out)
{
    if (!out || !std::isfinite(minX))
        return;
    out->x = minX;
    out->y = minY;
    out->width = std::max(0.0, maxX - minX);
    out->height = std::max(0.0, maxY - minY);
}

/** Ink/primitive bounds after RenderingContext affine. */
bool paintableWorldAabb(const DocNode &n, const document::Affine &worldMap, document::SmartBounds &out)
{
    if (n.kind == NodeKind::Ink) {
        if (n.samples.empty())
            return false;
        double minX = std::numeric_limits<double>::infinity();
        double minY = std::numeric_limits<double>::infinity();
        double maxX = -std::numeric_limits<double>::infinity();
        double maxY = -std::numeric_limits<double>::infinity();
        for (const auto &s : n.samples) {
            double wx = 0;
            double wy = 0;
            worldMap.apply(s.x, s.y, &wx, &wy);
            minX = std::min(minX, wx);
            minY = std::min(minY, wy);
            maxX = std::max(maxX, wx);
            maxY = std::max(maxY, wy);
        }
        boundsFromWorldPoints(minX, minY, maxX, maxY, &out);
        return out.width >= 0 && out.height >= 0;
    }

    if (n.kind == NodeKind::Primitive) {
        double minX = std::numeric_limits<double>::infinity();
        double minY = std::numeric_limits<double>::infinity();
        double maxX = -std::numeric_limits<double>::infinity();
        double maxY = -std::numeric_limits<double>::infinity();
        auto acc = [&](double x, double y) {
            double wx = 0;
            double wy = 0;
            worldMap.apply(x, y, &wx, &wy);
            minX = std::min(minX, wx);
            minY = std::min(minY, wy);
            maxX = std::max(maxX, wx);
            maxY = std::max(maxY, wy);
        };
        if (n.geomKind == PrimitiveKind::Line) {
            acc(n.x1, n.y1);
            acc(n.x2, n.y2);
        } else if (n.geomKind == PrimitiveKind::Rect) {
            const double xs[2] = {n.gx, n.gx + n.gw};
            const double ys[2] = {n.gy, n.gy + n.gh};
            for (double x : xs) {
                for (double y : ys)
                    acc(x, y);
            }
        } else if (n.geomKind == PrimitiveKind::Ellipse) {
            acc(n.cx - n.rx, n.cy - n.ry);
            acc(n.cx + n.rx, n.cy + n.ry);
        } else {
            return false;
        }
        boundsFromWorldPoints(minX, minY, maxX, maxY, &out);
        return std::isfinite(minX);
    }

    return document::nodeWorldAabb(n, out);
}

bool nodeOverlapsClip(const DocNode &n, const document::Affine &worldMap, const WorldAabb &clip)
{
    if (!clip.valid())
        return true;
    if (n.kind == NodeKind::Connector) {
        double minX = 0;
        double maxX = 0;
        double minY = 0;
        double maxY = 0;
        bool any = false;
        auto acc = [&](double x, double y) {
            if (!any) {
                minX = maxX = x;
                minY = maxY = y;
                any = true;
            } else {
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }
        };
        for (const auto &s : n.warpedSamples)
            acc(s.x, s.y);
        for (const auto &poly : n.warpedStyleInk) {
            for (const auto &s : poly)
                acc(s.x, s.y);
        }
        if (!any)
            return true;
        return aabbOverlap(clip, minX, minY, maxX, maxY);
    }
    document::SmartBounds b;
    if (n.kind == NodeKind::Ink || n.kind == NodeKind::Primitive) {
        if (!paintableWorldAabb(n, worldMap, b))
            return true;
    } else if (!document::nodeWorldAabb(n, b)) {
        return true;
    }
    return smartBoundsOverlap(clip, b);
}

double widthMulFor(const RenderRequest &req, const std::string &id)
{
    const auto it = req.styles.find(id);
    if (it == req.styles.end())
        return 1.0;
    return it->second.widthMul > 0.0 ? it->second.widthMul : 1.0;
}

WorldAabb worldAabbFromSmart(const document::SmartBounds &b)
{
    WorldAabb w;
    w.minX = std::min(b.x, b.x + b.width);
    w.maxX = std::max(b.x, b.x + b.width);
    w.minY = std::min(b.y, b.y + b.height);
    w.maxY = std::max(b.y, b.y + b.height);
    return w;
}

/** active=false → no restriction. active && !box.valid() → reject all (empty intersection). */
struct ContentClip {
    bool active = false;
    WorldAabb box;
    bool rejectsAll() const { return active && !box.valid(); }
    bool clips() const { return active && box.valid(); }
};

ContentClip fromWorldClip(const WorldAabb &w)
{
    ContentClip c;
    if (w.valid()) {
        c.active = true;
        c.box = w;
    }
    return c;
}

ContentClip intersectContent(ContentClip a, const WorldAabb &b)
{
    ContentClip o;
    o.active = true;
    o.box = a.active ? intersectWorldAabb(a.box, b) : b;
    return o;
}

bool clipT(double p, double q, double *t0, double *t1)
{
    if (std::abs(p) < 1e-15)
        return q >= -1e-15;
    const double r = q / p;
    if (p < 0) {
        if (r > *t1)
            return false;
        if (r > *t0)
            *t0 = r;
    } else {
        if (r < *t0)
            return false;
        if (r < *t1)
            *t1 = r;
    }
    return true;
}

bool clipSegmentToAabb(double x0, double y0, double x1, double y1, const WorldAabb &c, double *ox0,
                       double *oy0, double *ox1, double *oy1)
{
    double t0 = 0;
    double t1 = 1;
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    if (!clipT(-dx, x0 - c.minX, &t0, &t1))
        return false;
    if (!clipT(dx, c.maxX - x0, &t0, &t1))
        return false;
    if (!clipT(-dy, y0 - c.minY, &t0, &t1))
        return false;
    if (!clipT(dy, c.maxY - y0, &t0, &t1))
        return false;
    *ox0 = x0 + t0 * dx;
    *oy0 = y0 + t0 * dy;
    *ox1 = x0 + t1 * dx;
    *oy1 = y0 + t1 * dy;
    return true;
}

void clipPolylineToAabb(const std::vector<std::pair<double, double>> &pts, const ContentClip &clip,
                        std::vector<std::vector<std::pair<double, double>>> *out)
{
    if (!out)
        return;
    if (clip.rejectsAll())
        return;
    if (!clip.clips()) {
        if (pts.size() >= 2)
            out->push_back(pts);
        return;
    }
    std::vector<std::pair<double, double>> cur;
    auto flush = [&]() {
        if (cur.size() >= 2)
            out->push_back(cur);
        cur.clear();
    };
    auto same = [](double ax, double ay, double bx, double by) {
        return std::abs(ax - bx) < 1e-9 && std::abs(ay - by) < 1e-9;
    };
    for (std::size_t i = 1; i < pts.size(); ++i) {
        double ax = 0;
        double ay = 0;
        double bx = 0;
        double by = 0;
        if (!clipSegmentToAabb(pts[i - 1].first, pts[i - 1].second, pts[i].first, pts[i].second,
                               clip.box, &ax, &ay, &bx, &by)) {
            flush();
            continue;
        }
        if (cur.empty()) {
            cur.push_back({ax, ay});
        } else if (!same(cur.back().first, cur.back().second, ax, ay)) {
            flush();
            cur.push_back({ax, ay});
        }
        cur.push_back({bx, by});
    }
    flush();
}

ContentClip ancestorGroupClip(const document::DeviceDocument &doc, const std::string &nodeId,
                              ContentClip base)
{
    ContentClip clip = base;
    document::DeviceDocument::NodePlace pl;
    std::string cur = nodeId;
    std::vector<const DocNode *> sgs;
    while (doc.findPlace(cur, &pl) && !pl.parentId.empty()) {
        const DocNode *p = doc.find(pl.parentId);
        if (!p)
            break;
        if (p->kind == NodeKind::SmartGroup)
            sgs.push_back(p);
        cur = pl.parentId;
    }
    for (int i = int(sgs.size()) - 1; i >= 0; --i) {
        const DocNode *p = sgs[size_t(i)];
        const document::SmartBounds b =
            document::smartGroupWorldBounds(*p, document::ancestorContentContext(doc, p->id));
        clip = intersectContent(clip, worldAabbFromSmart(b));
    }
    return clip;
}

void emitPolyline(IPixelSink &sink, PanelPolyline poly, LastPaintStats *st)
{
    if (poly.pts.size() < 2)
        return;
    if (st) {
        ++st->polylines;
        st->pts += poly.pts.size();
    }
    sink.drawPolyline(poly);
}

void emitWorldPolyline(IPixelSink &sink, const FrameProjector &proj, double lineW,
                       const std::vector<std::pair<double, double>> &worldPts,
                       const ContentClip &clip, LastPaintStats *st)
{
    std::vector<std::vector<std::pair<double, double>>> frags;
    clipPolylineToAabb(worldPts, clip, &frags);
    for (const auto &frag : frags) {
        PanelPolyline poly;
        poly.width = lineW;
        poly.pts.reserve(frag.size());
        for (const auto &w : frag) {
            double px = 0;
            double py = 0;
            proj.worldToPanel(w.first, w.second, &px, &py);
            poly.pts.push_back({px, py});
        }
        emitPolyline(sink, std::move(poly), st);
    }
}

bool shouldEmit(const RenderRequest &req, const std::string &id)
{
    if (req.suppressIds.count(id))
        return false;
    if (!req.includeIds.empty() && !req.includeIds.count(id))
        return false;
    return true;
}

void emitInkOrPrimitive(const DocNode &node, const document::Affine &worldMap,
                        const FrameProjector &proj, const RenderRequest &req, IPixelSink &sink,
                        LastPaintStats *st, const ContentClip &clip)
{
    if (!shouldEmit(req, node.id))
        return;
    if (node.kind != NodeKind::Ink && node.kind != NodeKind::Primitive)
        return;
    if (clip.rejectsAll())
        return;

    const double worldSw = node.style.strokeWidth;
    const double sPanel = proj.panelScale();
    const double mul = widthMulFor(req, node.id);
    const double lineW = std::max(1.0, worldSw * sPanel * mul);

    auto toWorld = [&](double x, double y) {
        double wx = 0;
        double wy = 0;
        worldMap.apply(x, y, &wx, &wy);
        return std::make_pair(wx, wy);
    };

    if (node.kind == NodeKind::Ink) {
        if (node.samples.size() < 2)
            return;
        std::vector<std::pair<double, double>> worldPts;
        worldPts.reserve(node.samples.size());
        for (const auto &s : node.samples)
            worldPts.push_back(toWorld(s.x, s.y));
        emitWorldPolyline(sink, proj, lineW, worldPts, clip, st);
        return;
    }

    if (node.geomKind == PrimitiveKind::Line) {
        emitWorldPolyline(sink, proj, lineW, {toWorld(node.x1, node.y1), toWorld(node.x2, node.y2)},
                          clip, st);
        return;
    }
    if (node.geomKind == PrimitiveKind::Rect) {
        const double x1 = node.gx + node.gw;
        const double y1 = node.gy + node.gh;
        emitWorldPolyline(sink, proj, lineW,
                          {toWorld(node.gx, node.gy), toWorld(x1, node.gy), toWorld(x1, y1),
                           toWorld(node.gx, y1), toWorld(node.gx, node.gy)},
                          clip, st);
        return;
    }
    if (node.geomKind == PrimitiveKind::Ellipse) {
        constexpr int kSeg = 32;
        std::vector<std::pair<double, double>> worldPts;
        worldPts.reserve(kSeg + 1);
        for (int i = 0; i <= kSeg; ++i) {
            const double t = (2.0 * 3.14159265358979323846 * double(i)) / double(kSeg);
            worldPts.push_back(
                toWorld(node.cx + node.rx * std::cos(t), node.cy + node.ry * std::sin(t)));
        }
        emitWorldPolyline(sink, proj, lineW, worldPts, clip, st);
    }
}

bool nodeOverlapsContentClip(const DocNode &n, const document::Affine &worldMap,
                             const ContentClip &clip)
{
    if (clip.rejectsAll())
        return false;
    if (!clip.clips())
        return true;
    return nodeOverlapsClip(n, worldMap, clip.box);
}

bool smartOverlapsContentClip(const document::SmartBounds &b, const ContentClip &clip)
{
    if (clip.rejectsAll())
        return false;
    if (!clip.clips())
        return true;
    return smartBoundsOverlap(clip.box, b);
}

void emitConnector(const DocNode &conn, const FrameProjector &proj, const RenderRequest &req,
                   IPixelSink &sink, LastPaintStats *st, const ContentClip &clip)
{
    if (!shouldEmit(req, conn.id))
        return;
    if (clip.rejectsAll())
        return;
    double worldSw = conn.style.strokeWidth;
    if (worldSw <= 0.0 && !conn.children.empty())
        worldSw = conn.children.front().style.strokeWidth;
    if (worldSw <= 0.0)
        worldSw = 2.0;
    const double mul = widthMulFor(req, conn.id);
    const double lineW = std::max(1.0, worldSw * proj.panelScale() * mul);
    if (conn.warpedSamples.size() >= 2) {
        std::vector<std::pair<double, double>> worldPts;
        worldPts.reserve(conn.warpedSamples.size());
        for (const auto &s : conn.warpedSamples)
            worldPts.push_back({s.x, s.y});
        emitWorldPolyline(sink, proj, lineW, worldPts, clip, st);
    }
    for (const auto &stylePoly : conn.warpedStyleInk) {
        if (stylePoly.size() < 2)
            continue;
        std::vector<std::pair<double, double>> worldPts;
        worldPts.reserve(stylePoly.size());
        for (const auto &s : stylePoly)
            worldPts.push_back({s.x, s.y});
        emitWorldPolyline(sink, proj, lineW, worldPts, clip, st);
    }
}

void walkFlat(const std::vector<DocNode> &nodes, const document::Affine &ctx,
              const FrameProjector &proj, const RenderRequest &req, IPixelSink &sink,
              LastPaintStats *st, const ContentClip &clip);

void walkSmartFlat(const DocNode &sg, const document::Affine &ctx, const FrameProjector &proj,
                   const RenderRequest &req, IPixelSink &sink, LastPaintStats *st,
                   const ContentClip &ancestorClip)
{
    const document::Affine outcome = document::outcomeAffine(ctx, sg);
    const document::Affine content = document::contentOutcome(ctx, sg);
    const ContentClip contentClip =
        intersectContent(ancestorClip, worldAabbFromSmart(document::smartGroupWorldBounds(sg, ctx)));
    for (const auto &node : sg.children) {
        if (req.cancel && req.cancel->load())
            return;
        ++st->visits;
        if (node.kind == NodeKind::SmartGroup)
            walkSmartFlat(node, content, proj, req, sink, st, contentClip);
        else if (node.kind == NodeKind::Frame || node.kind == NodeKind::Group)
            walkFlat(node.children, document::affineIdentity(), proj, req, sink, st, contentClip);
        else if (node.kind == NodeKind::Connector)
            emitConnector(node, proj, req, sink, st, contentClip);
        else {
            const std::string role = node.role ? *node.role : std::string("content");
            const document::Affine used = (role == "boundary") ? outcome : content;
            const ContentClip &usedClip = (role == "boundary") ? ancestorClip : contentClip;
            emitInkOrPrimitive(node, used, proj, req, sink, st, usedClip);
        }
    }
}

void walkFlat(const std::vector<DocNode> &nodes, const document::Affine &ctx,
              const FrameProjector &proj, const RenderRequest &req, IPixelSink &sink,
              LastPaintStats *st, const ContentClip &clip)
{
    for (const auto &node : nodes) {
        if (req.cancel && req.cancel->load())
            return;
        ++st->visits;
        if (node.kind == NodeKind::SmartGroup)
            walkSmartFlat(node, ctx, proj, req, sink, st, clip);
        else if (node.kind == NodeKind::Frame || node.kind == NodeKind::Group)
            walkFlat(node.children, document::affineIdentity(), proj, req, sink, st, clip);
        else if (node.kind == NodeKind::Connector)
            emitConnector(node, proj, req, sink, st, clip);
        else
            emitInkOrPrimitive(node, ctx, proj, req, sink, st, clip);
    }
}

void walkCull(const std::vector<DocNode> &nodes, const document::Affine &ctx,
              const FrameProjector &proj, const RenderRequest &req, IPixelSink &sink,
              LastPaintStats *st, const ContentClip &clip);

void walkSmartCull(const DocNode &sg, const document::Affine &ctx, const FrameProjector &proj,
                   const RenderRequest &req, IPixelSink &sink, LastPaintStats *st,
                   const ContentClip &ancestorClip)
{
    const document::Affine outcome = document::outcomeAffine(ctx, sg);
    const document::Affine content = document::contentOutcome(ctx, sg);
    const ContentClip contentClip =
        intersectContent(ancestorClip, worldAabbFromSmart(document::smartGroupWorldBounds(sg, ctx)));
    for (const auto &node : sg.children) {
        if (req.cancel && req.cancel->load())
            return;
        ++st->visits;
        if (node.kind == NodeKind::SmartGroup) {
            const document::SmartBounds b = document::smartGroupWorldBounds(node, content);
            if (!smartOverlapsContentClip(b, contentClip)) {
                ++st->skipped;
                continue;
            }
            walkSmartCull(node, content, proj, req, sink, st, contentClip);
            continue;
        }
        if (node.kind == NodeKind::Frame || node.kind == NodeKind::Group) {
            if (!nodeOverlapsContentClip(node, document::affineIdentity(), contentClip)) {
                ++st->skipped;
                continue;
            }
            walkCull(node.children, document::affineIdentity(), proj, req, sink, st, contentClip);
            continue;
        }
        const std::string role = node.role ? *node.role : std::string("content");
        const document::Affine used = (role == "boundary") ? outcome : content;
        const ContentClip &usedClip = (role == "boundary") ? ancestorClip : contentClip;
        if (!nodeOverlapsContentClip(node, used, usedClip)) {
            ++st->skipped;
            continue;
        }
        if (node.kind == NodeKind::Connector)
            emitConnector(node, proj, req, sink, st, usedClip);
        else
            emitInkOrPrimitive(node, used, proj, req, sink, st, usedClip);
    }
}

void walkCull(const std::vector<DocNode> &nodes, const document::Affine &ctx,
              const FrameProjector &proj, const RenderRequest &req, IPixelSink &sink,
              LastPaintStats *st, const ContentClip &clip)
{
    for (const auto &node : nodes) {
        if (req.cancel && req.cancel->load())
            return;
        ++st->visits;
        if (node.kind == NodeKind::SmartGroup) {
            const document::SmartBounds b = document::smartGroupWorldBounds(node, ctx);
            if (!smartOverlapsContentClip(b, clip)) {
                ++st->skipped;
                continue;
            }
            walkSmartCull(node, ctx, proj, req, sink, st, clip);
            continue;
        }
        if (node.kind == NodeKind::Frame || node.kind == NodeKind::Group) {
            if (!nodeOverlapsContentClip(node, document::affineIdentity(), clip)) {
                ++st->skipped;
                continue;
            }
            walkCull(node.children, document::affineIdentity(), proj, req, sink, st, clip);
            continue;
        }
        if (!nodeOverlapsContentClip(node, ctx, clip)) {
            ++st->skipped;
            continue;
        }
        if (node.kind == NodeKind::Connector)
            emitConnector(node, proj, req, sink, st, clip);
        else
            emitInkOrPrimitive(node, ctx, proj, req, sink, st, clip);
    }
}

void collectPaintableIds(const std::vector<DocNode> &nodes, std::unordered_set<std::string> *out)
{
    if (!out)
        return;
    for (const auto &node : nodes) {
        if (node.kind == NodeKind::Ink || node.kind == NodeKind::Primitive)
            out->insert(node.id);
        else if (node.kind == NodeKind::SmartGroup || node.kind == NodeKind::Frame
                 || node.kind == NodeKind::Group)
            collectPaintableIds(node.children, out);
    }
}

void emitBoundConnectors(const document::DeviceDocument &doc, const std::string &sgId,
                         const FrameProjector &proj, const RenderRequest &req, IPixelSink &sink,
                         LastPaintStats *st)
{
    for (const auto &node : doc.rootChildren) {
        if (node.kind != NodeKind::Connector)
            continue;
        if (node.fromNodeId != sgId && node.toNodeId != sgId)
            continue;
        emitConnector(node, proj, req, sink, st, ContentClip{});
    }
}

void paintSubtreeWalk(const document::DeviceDocument &doc, const DocNode *root,
                      const FrameProjector &proj, const RenderRequest &req, IPixelSink &sink,
                      LastPaintStats *st, bool cull)
{
    if (!root || root->kind != NodeKind::SmartGroup)
        return;
    const document::Affine anc = document::ancestorContentContext(doc, root->id);
    const ContentClip clip = ancestorGroupClip(doc, root->id, fromWorldClip(req.worldClip));
    if (cull)
        walkSmartCull(*root, anc, proj, req, sink, st, clip);
    else
        walkSmartFlat(*root, anc, proj, req, sink, st, clip);
    emitBoundConnectors(doc, root->id, proj, req, sink, st);
}

} // namespace

void FrameProjector::worldToPanel(double wx, double wy, double *px, double *py) const
{
    if (!frame || !px || !py)
        return;
    const auto p = frame->worldToPanel(wx, wy);
    *px = p.x;
    *py = p.y;
}

WorldAabb FrameProjector::worldToPanelAabb(WorldAabb w) const
{
    double x0 = 0;
    double y0 = 0;
    double x1 = 0;
    double y1 = 0;
    worldToPanel(w.minX, w.minY, &x0, &y0);
    worldToPanel(w.maxX, w.maxY, &x1, &y1);
    WorldAabb out;
    out.minX = std::min(x0, x1);
    out.maxX = std::max(x0, x1);
    out.minY = std::min(y0, y1);
    out.maxY = std::max(y0, y1);
    return out;
}

double FrameProjector::panelScale() const
{
    if (!frame)
        return 1.0;
    const double rw = frame->drawingRegion.valid
        ? (frame->drawingRegion.maxX - frame->drawingRegion.minX)
        : std::max(1.0, frame->panelW);
    return frame->panelW / std::max(1e-6, rw);
}

WorldAabb FrameProjector::drawingWorldClip() const
{
    WorldAabb c;
    if (!frame || !frame->drawingRegion.valid)
        return c;
    c.minX = frame->drawingRegion.minX;
    c.minY = frame->drawingRegion.minY;
    c.maxX = frame->drawingRegion.maxX;
    c.maxY = frame->drawingRegion.maxY;
    return c;
}

void FlatWalkAlgorithm::paint(const document::DeviceDocument &doc, const FrameProjector &proj,
                              const RenderRequest &req, IPixelSink &sink)
{
    m_stats = {};
    walkFlat(doc.rootChildren, document::affineIdentity(), proj, req, sink, &m_stats,
             ContentClip{});
}

void HierarchyCullAlgorithm::paint(const document::DeviceDocument &doc, const FrameProjector &proj,
                                   const RenderRequest &req, IPixelSink &sink)
{
    m_stats = {};
    walkCull(doc.rootChildren, document::affineIdentity(), proj, req, sink, &m_stats,
             fromWorldClip(req.worldClip));
}

void DocumentRenderer::setAlgorithm(std::unique_ptr<IRenderAlgorithm> alg)
{
    m_alg = std::move(alg);
}

void DocumentRenderer::render(const document::DeviceDocument &doc, const FrameProjector &proj,
                              const RenderRequest &req, IPixelSink &sink)
{
    if (!m_alg)
        return;
    sink.begin(req.sharp);
    if (req.mode == RenderRequest::BufferMode::FullClear)
        sink.clearFull();
    else
        sink.clearRect(req.dirtyPanelX, req.dirtyPanelY, req.dirtyPanelW, req.dirtyPanelH);
    m_alg->paint(doc, proj, req, sink);
    sink.end();
}

void collectManipSuppressIds(const document::DeviceDocument &doc, const std::string &sgId,
                             std::unordered_set<std::string> *out)
{
    if (!out)
        return;
    const DocNode *root = doc.find(sgId);
    if (!root || root->kind != NodeKind::SmartGroup)
        return;
    collectPaintableIds(root->children, out);
    for (const auto &node : doc.rootChildren) {
        if (node.kind != NodeKind::Connector)
            continue;
        if (node.fromNodeId == sgId || node.toNodeId == sgId)
            out->insert(node.id);
    }
}

void DocumentRenderer::renderSubtree(const document::DeviceDocument &doc, const FrameProjector &proj,
                                     const RenderRequest &req, const std::string &rootId,
                                     IPixelSink &sink)
{
    const DocNode *root = doc.find(rootId);
    if (!root)
        return;
    sink.begin(req.sharp);
    LastPaintStats st;
    paintSubtreeWalk(doc, root, proj, req, sink, &st, /*cull=*/false);
    sink.end();
}

} // namespace render
} // namespace epaper
