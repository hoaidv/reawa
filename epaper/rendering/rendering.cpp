#include "rendering/rendering.hpp"

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

/** Ink/primitive bounds in world space (local samples when inside a SmartGroup). */
bool paintableWorldAabb(const DocNode &n, const DocNode *smartParent, document::SmartBounds &out)
{
    using document::inkSamplesMin;
    using document::smartLocalToWorld;

    if (n.kind == NodeKind::Ink) {
        if (n.samples.empty())
            return false;
        double minX = std::numeric_limits<double>::infinity();
        double minY = std::numeric_limits<double>::infinity();
        double maxX = -std::numeric_limits<double>::infinity();
        double maxY = -std::numeric_limits<double>::infinity();
        if (smartParent) {
            const std::string role = n.role ? *n.role : std::string("content");
            document::Vec2 contentMin{};
            const document::Vec2 *minPtr = nullptr;
            if (role == "content" && smartParent->inkScaleMode == "fixedInk") {
                contentMin = inkSamplesMin(n.samples);
                minPtr = &contentMin;
            }
            for (const auto &s : n.samples) {
                const auto w = smartLocalToWorld(s.x, s.y, *smartParent, role, n.layoutOffset,
                                                 minPtr);
                minX = std::min(minX, w.x);
                minY = std::min(minY, w.y);
                maxX = std::max(maxX, w.x);
                maxY = std::max(maxY, w.y);
            }
        } else {
            for (const auto &s : n.samples) {
                minX = std::min(minX, s.x);
                minY = std::min(minY, s.y);
                maxX = std::max(maxX, s.x);
                maxY = std::max(maxY, s.y);
            }
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
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
        };
        if (n.geomKind == PrimitiveKind::Line) {
            if (smartParent) {
                const auto a = smartLocalToWorld(n.x1, n.y1, *smartParent, "content",
                                                 n.layoutOffset, nullptr);
                const auto b = smartLocalToWorld(n.x2, n.y2, *smartParent, "content",
                                                 n.layoutOffset, nullptr);
                acc(a.x, a.y);
                acc(b.x, b.y);
            } else {
                acc(n.x1, n.y1);
                acc(n.x2, n.y2);
            }
        } else if (n.geomKind == PrimitiveKind::Rect) {
            const double xs[2] = {n.gx, n.gx + n.gw};
            const double ys[2] = {n.gy, n.gy + n.gh};
            for (double x : xs) {
                for (double y : ys) {
                    if (smartParent) {
                        const auto w = smartLocalToWorld(x, y, *smartParent, "content",
                                                         n.layoutOffset, nullptr);
                        acc(w.x, w.y);
                    } else {
                        acc(x, y);
                    }
                }
            }
        } else if (n.geomKind == PrimitiveKind::Ellipse) {
            const double xs[2] = {n.cx - n.rx, n.cx + n.rx};
            const double ys[2] = {n.cy - n.ry, n.cy + n.ry};
            for (double x : xs) {
                for (double y : ys) {
                    if (smartParent) {
                        const auto w = smartLocalToWorld(x, y, *smartParent, "content",
                                                         n.layoutOffset, nullptr);
                        acc(w.x, w.y);
                    } else {
                        acc(x, y);
                    }
                }
            }
        } else {
            return false;
        }
        boundsFromWorldPoints(minX, minY, maxX, maxY, &out);
        return std::isfinite(minX);
    }

    return document::nodeWorldAabb(n, out);
}

/** SmartGroup world AABB including rotation (axis-aligned hull of transformed bounds). */
bool smartGroupWorldAabb(const DocNode &sg, document::SmartBounds &out)
{
    if (sg.kind != NodeKind::SmartGroup)
        return false;
    const document::SmartBounds &b = sg.smartBounds;
    const double xs[2] = {b.x, b.x + b.width};
    const double ys[2] = {b.y, b.y + b.height};
    double minX = std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    for (double x : xs) {
        for (double y : ys) {
            const auto w = document::smartLocalToWorld(x, y, sg, "boundary", std::nullopt, nullptr);
            minX = std::min(minX, w.x);
            minY = std::min(minY, w.y);
            maxX = std::max(maxX, w.x);
            maxY = std::max(maxY, w.y);
        }
    }
    boundsFromWorldPoints(minX, minY, maxX, maxY, &out);
    return std::isfinite(minX);
}

bool nodeOverlapsClip(const DocNode &n, const DocNode *smartParent, const WorldAabb &clip)
{
    if (!clip.valid())
        return true;
    if (n.kind == NodeKind::Connector) {
        if (n.warpedSamples.empty())
            return true;
        double minX = n.warpedSamples[0].x;
        double maxX = minX;
        double minY = n.warpedSamples[0].y;
        double maxY = minY;
        for (const auto &s : n.warpedSamples) {
            minX = std::min(minX, s.x);
            maxX = std::max(maxX, s.x);
            minY = std::min(minY, s.y);
            maxY = std::max(maxY, s.y);
        }
        return aabbOverlap(clip, minX, minY, maxX, maxY);
    }
    document::SmartBounds b;
    if (n.kind == NodeKind::SmartGroup) {
        if (!smartGroupWorldAabb(n, b))
            return true;
    } else if (n.kind == NodeKind::Ink || n.kind == NodeKind::Primitive) {
        if (!paintableWorldAabb(n, smartParent, b))
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

bool shouldEmit(const RenderRequest &req, const std::string &id)
{
    if (req.suppressIds.count(id))
        return false;
    if (!req.includeIds.empty() && !req.includeIds.count(id))
        return false;
    return true;
}

void emitInkOrPrimitive(const DocNode &node, const DocNode *smartParent,
                        const FrameProjector &proj, const RenderRequest &req, IPixelSink &sink,
                        LastPaintStats *st)
{
    if (!shouldEmit(req, node.id))
        return;
    if (node.kind != NodeKind::Ink && node.kind != NodeKind::Primitive)
        return;

    const double worldSw = node.style.strokeWidth;
    const double sPanel = proj.panelScale();
    const double mul = widthMulFor(req, node.id);
    const double lineW = std::max(1.0, worldSw * sPanel * mul);

    auto toPanel = [&](double x, double y) {
        double px = 0;
        double py = 0;
        if (smartParent) {
            const std::string role = node.role ? *node.role : std::string("content");
            document::Vec2 contentMin{};
            const document::Vec2 *minPtr = nullptr;
            if (role == "content" && smartParent->inkScaleMode == "fixedInk") {
                contentMin = document::inkSamplesMin(node.samples);
                minPtr = &contentMin;
            }
            const auto w =
                document::smartLocalToWorld(x, y, *smartParent, role, node.layoutOffset, minPtr);
            proj.worldToPanel(w.x, w.y, &px, &py);
        } else {
            proj.worldToPanel(x, y, &px, &py);
        }
        return std::make_pair(px, py);
    };

    if (node.kind == NodeKind::Ink) {
        if (node.samples.size() < 2)
            return;
        PanelPolyline poly;
        poly.width = lineW;
        poly.pts.reserve(node.samples.size());
        for (const auto &s : node.samples)
            poly.pts.push_back(toPanel(s.x, s.y));
        emitPolyline(sink, std::move(poly), st);
        return;
    }

    if (node.geomKind == PrimitiveKind::Line) {
        PanelPolyline poly;
        poly.width = lineW;
        poly.pts = {toPanel(node.x1, node.y1), toPanel(node.x2, node.y2)};
        emitPolyline(sink, std::move(poly), st);
        return;
    }
    if (node.geomKind == PrimitiveKind::Rect) {
        const auto tl = toPanel(node.gx, node.gy);
        const auto br = toPanel(node.gx + node.gw, node.gy + node.gh);
        const double x0 = std::min(tl.first, br.first);
        const double x1 = std::max(tl.first, br.first);
        const double y0 = std::min(tl.second, br.second);
        const double y1 = std::max(tl.second, br.second);
        PanelPolyline poly;
        poly.width = lineW;
        poly.pts = {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}, {x0, y0}};
        emitPolyline(sink, std::move(poly), st);
        return;
    }
    if (node.geomKind == PrimitiveKind::Ellipse) {
        double cx = 0;
        double cy = 0;
        double ex = 0;
        double ey = 0;
        proj.worldToPanel(node.cx, node.cy, &cx, &cy);
        proj.worldToPanel(node.cx + node.rx, node.cy + node.ry, &ex, &ey);
        const double rx = std::abs(ex - cx);
        const double ry = std::abs(ey - cy);
        constexpr int kSeg = 32;
        PanelPolyline poly;
        poly.width = lineW;
        poly.pts.reserve(kSeg + 1);
        for (int i = 0; i <= kSeg; ++i) {
            const double t = (2.0 * 3.14159265358979323846 * double(i)) / double(kSeg);
            poly.pts.push_back({cx + rx * std::cos(t), cy + ry * std::sin(t)});
        }
        emitPolyline(sink, std::move(poly), st);
    }
}

void emitConnector(const DocNode &conn, const FrameProjector &proj, const RenderRequest &req,
                   IPixelSink &sink, LastPaintStats *st)
{
    if (!shouldEmit(req, conn.id))
        return;
    if (conn.warpedSamples.size() < 2)
        return;
    double worldSw = conn.style.strokeWidth;
    if (worldSw <= 0.0 && !conn.children.empty())
        worldSw = conn.children.front().style.strokeWidth;
    if (worldSw <= 0.0)
        worldSw = 2.0;
    const double mul = widthMulFor(req, conn.id);
    PanelPolyline poly;
    poly.width = std::max(1.0, worldSw * proj.panelScale() * mul);
    poly.pts.reserve(conn.warpedSamples.size());
    for (const auto &s : conn.warpedSamples) {
        double px = 0;
        double py = 0;
        proj.worldToPanel(s.x, s.y, &px, &py);
        poly.pts.push_back({px, py});
    }
    emitPolyline(sink, std::move(poly), st);
}

void walkFlat(const std::vector<DocNode> &nodes, const DocNode *smartParent,
              const FrameProjector &proj, const RenderRequest &req, IPixelSink &sink,
              LastPaintStats *st)
{
    for (const auto &node : nodes) {
        if (req.cancel && req.cancel->load())
            return;
        ++st->visits;
        if (node.kind == NodeKind::SmartGroup)
            walkFlat(node.children, &node, proj, req, sink, st);
        else if (node.kind == NodeKind::Frame || node.kind == NodeKind::Group)
            walkFlat(node.children, nullptr, proj, req, sink, st);
        else if (node.kind == NodeKind::Connector)
            emitConnector(node, proj, req, sink, st);
        else
            emitInkOrPrimitive(node, smartParent, proj, req, sink, st);
    }
}

void walkCull(const std::vector<DocNode> &nodes, const DocNode *smartParent,
              const FrameProjector &proj, const RenderRequest &req, IPixelSink &sink,
              LastPaintStats *st)
{
    for (const auto &node : nodes) {
        if (req.cancel && req.cancel->load())
            return;
        ++st->visits;
        if (node.kind == NodeKind::SmartGroup || node.kind == NodeKind::Frame
            || node.kind == NodeKind::Group) {
            if (!nodeOverlapsClip(node, smartParent, req.worldClip)) {
                ++st->skipped;
                continue;
            }
            if (node.kind == NodeKind::SmartGroup)
                walkCull(node.children, &node, proj, req, sink, st);
            else
                walkCull(node.children, nullptr, proj, req, sink, st);
            continue;
        }
        if (!nodeOverlapsClip(node, smartParent, req.worldClip)) {
            ++st->skipped;
            continue;
        }
        if (node.kind == NodeKind::Connector)
            emitConnector(node, proj, req, sink, st);
        else
            emitInkOrPrimitive(node, smartParent, proj, req, sink, st);
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
        emitConnector(node, proj, req, sink, st);
    }
}

void paintSubtreeWalk(const document::DeviceDocument &doc, const DocNode *root,
                      const FrameProjector &proj, const RenderRequest &req, IPixelSink &sink,
                      LastPaintStats *st, bool cull)
{
    if (!root || root->kind != NodeKind::SmartGroup)
        return;
    if (cull)
        walkCull(root->children, root, proj, req, sink, st);
    else
        walkFlat(root->children, root, proj, req, sink, st);
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
    walkFlat(doc.rootChildren, nullptr, proj, req, sink, &m_stats);
}

void HierarchyCullAlgorithm::paint(const document::DeviceDocument &doc, const FrameProjector &proj,
                                   const RenderRequest &req, IPixelSink &sink)
{
    m_stats = {};
    walkCull(doc.rootChildren, nullptr, proj, req, sink, &m_stats);
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
