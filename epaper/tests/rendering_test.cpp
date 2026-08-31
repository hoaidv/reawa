/**
 * Host tests for epaper::render — FlatWalk vs HierarchyCull (no Qt).
 */

#include "drawing/canvas_frame.hpp"
#include "document/device_document.hpp"
#include "rendering/rendering.hpp"

#include <atomic>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace epaper::document;
using namespace epaper::render;
using epaper::canvasframe::CanvasFrame;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

struct RecordingSink : IPixelSink {
    int polylineCount = 0;
    int clearFullCount = 0;
    int clearRectCount = 0;
    std::vector<double> widths;

    void begin(bool) override {}
    void clearFull() override { ++clearFullCount; }
    void clearRect(double, double, double, double) override { ++clearRectCount; }
    void drawPolyline(const PanelPolyline &poly) override
    {
        if (poly.pts.size() < 2)
            return;
        ++polylineCount;
        widths.push_back(poly.width);
    }
    void end() override {}
};

static InkSample sample(double x, double y)
{
    InkSample s;
    s.x = x;
    s.y = y;
    return s;
}

static DocNode makeFreeInk(const std::string &id, double x0, double y0, double x1, double y1)
{
    DocNode n;
    n.id = id;
    n.kind = NodeKind::Ink;
    n.style.strokeWidth = 2;
    n.samples.push_back(sample(x0, y0));
    n.samples.push_back(sample(x1, y1));
    return n;
}

static DocNode makeFarSmartGroup()
{
    DocNode sg;
    sg.id = "sg_far";
    sg.kind = NodeKind::SmartGroup;
    sg.smartBounds = {0, 0, 100, 100};
    sg.transform = {5000, 5000, 0, 1, 1};
    sg.inkScaleMode = "withBounds";

    DocNode child;
    child.id = "sg_far_c";
    child.kind = NodeKind::Ink;
    child.role = std::string("content");
    child.style.strokeWidth = 2;
    child.samples.push_back(sample(10, 10));
    child.samples.push_back(sample(20, 20));
    sg.children.push_back(std::move(child));
    return sg;
}

static void test_full_clip_same_polylines()
{
    DeviceDocument doc;
    doc.rootChildren.push_back(makeFreeInk("ink_near", 10, 10, 20, 20));
    doc.rootChildren.push_back(makeFarSmartGroup());

    CanvasFrame frame;
    frame.setPanelSize(1404, 1872);
    frame.applyDrawingRegion({0, 0, 10000, 10000}, true);

    FrameProjector proj;
    proj.frame = &frame;

    RenderRequest req;
    req.worldClip = proj.drawingWorldClip();
    req.sharp = true;

    RecordingSink flatSink;
    FlatWalkAlgorithm flat;
    flat.paint(doc, proj, req, flatSink);

    RecordingSink cullSink;
    HierarchyCullAlgorithm cull;
    cull.paint(doc, proj, req, cullSink);

    CHECK(flatSink.polylineCount == cullSink.polylineCount);
    CHECK(flatSink.polylineCount == 2); // free ink + SG child
    CHECK(flat.lastVisitCount() >= cull.lastVisitCount());
    CHECK(flat.lastPaintStats().polylines == 2);
    CHECK(cull.lastPaintStats().polylines == 2);
    CHECK(cull.lastPaintStats().pts >= 4);
}

static void test_hierarchy_cull_skips_far_sg()
{
    DeviceDocument doc;
    doc.rootChildren.push_back(makeFreeInk("ink_near", 10, 10, 20, 20));
    doc.rootChildren.push_back(makeFarSmartGroup());

    CanvasFrame frame;
    frame.setPanelSize(1404, 1872);
    // Camera around the origin only — far SG at (5000,5000) is outside.
    frame.applyDrawingRegion({0, 0, 200, 200}, true);

    FrameProjector proj;
    proj.frame = &frame;

    RenderRequest req;
    req.worldClip = proj.drawingWorldClip();
    req.sharp = true;

    RecordingSink flatSink;
    FlatWalkAlgorithm flat;
    flat.paint(doc, proj, req, flatSink);

    RecordingSink cullSink;
    HierarchyCullAlgorithm cull;
    cull.paint(doc, proj, req, cullSink);

    CHECK(flatSink.polylineCount == 2);
    CHECK(cullSink.polylineCount == 1);
    CHECK(cull.lastVisitCount() < flat.lastVisitCount());
    CHECK(cull.lastPaintStats().skipped >= 1);
    CHECK(cull.lastPaintStats().polylines == 1);
}

static void test_sg_child_visible_when_panned_into_view()
{
    DeviceDocument doc;
    doc.rootChildren.push_back(makeFarSmartGroup());

    CanvasFrame frame;
    frame.setPanelSize(1404, 1872);
    // Viewport over the far SG — child ink is at world ~(5010–5020), not local (10–20).
    frame.applyDrawingRegion({5000, 5000, 200, 200}, true);

    FrameProjector proj;
    proj.frame = &frame;

    RenderRequest req;
    req.worldClip = proj.drawingWorldClip();
    req.sharp = true;

    RecordingSink flatSink;
    FlatWalkAlgorithm flat;
    flat.paint(doc, proj, req, flatSink);

    RecordingSink cullSink;
    HierarchyCullAlgorithm cull;
    cull.paint(doc, proj, req, cullSink);

    CHECK(flatSink.polylineCount == 1);
    CHECK(cullSink.polylineCount == 1);
}

static void test_suppress_skips_subtree()
{
    DeviceDocument doc;
    doc.rootChildren.push_back(makeFreeInk("ink_near", 10, 10, 20, 20));
    doc.rootChildren.push_back(makeFarSmartGroup());

    CanvasFrame frame;
    frame.setPanelSize(1404, 1872);
    frame.applyDrawingRegion({0, 0, 10000, 10000}, true);
    FrameProjector proj;
    proj.frame = &frame;

    RenderRequest req;
    req.worldClip = proj.drawingWorldClip();
    req.sharp = true;
    req.suppressIds.insert("sg_far_c");

    RecordingSink flatSink;
    FlatWalkAlgorithm flat;
    flat.paint(doc, proj, req, flatSink);

    RecordingSink cullSink;
    HierarchyCullAlgorithm cull;
    cull.paint(doc, proj, req, cullSink);

    CHECK(flatSink.polylineCount == 1);
    CHECK(cullSink.polylineCount == 1);
}

static DocNode makeSgWithConnector()
{
    DocNode sg = makeFarSmartGroup();
    sg.id = "sg_a";
    sg.transform = {0, 0, 0, 1, 1};
    sg.children[0].id = "sg_a_c";
    return sg;
}

static ConnectorRestPt restPt(double x, double y)
{
    ConnectorRestPt p;
    p.x = x;
    p.y = y;
    return p;
}

static void test_renderSubtree_sg_and_connector()
{
    DeviceDocument doc;
    DocNode sg = makeSgWithConnector();
    doc.rootChildren.push_back(std::move(sg));
    DocNode conn;
    conn.id = "conn_a";
    conn.kind = NodeKind::Connector;
    conn.fromNodeId = "sg_a";
    conn.toNodeId = "other";
    conn.style.strokeWidth = 2;
    conn.warpedSamples.push_back(restPt(5, 5));
    conn.warpedSamples.push_back(restPt(50, 50));
    doc.rootChildren.push_back(std::move(conn));

    CanvasFrame frame;
    frame.setPanelSize(100, 100);
    frame.applyDrawingRegion({0, 0, 100, 100}, true);
    FrameProjector proj;
    proj.frame = &frame;

    RenderRequest req;
    req.sharp = true;

    RecordingSink sink;
    DocumentRenderer renderer;
    renderer.setAlgorithm(std::make_unique<FlatWalkAlgorithm>());
    renderer.renderSubtree(doc, proj, req, "sg_a", sink);
    CHECK(sink.polylineCount == 2); // child ink + bound connector
}

static void test_collectManipSuppressIds()
{
    DeviceDocument doc;
    DocNode sg = makeSgWithConnector();
    doc.rootChildren.push_back(std::move(sg));
    DocNode conn;
    conn.id = "conn_a";
    conn.kind = NodeKind::Connector;
    conn.fromNodeId = "sg_a";
    conn.toNodeId = "other";
    doc.rootChildren.push_back(std::move(conn));

    std::unordered_set<std::string> ids;
    collectManipSuppressIds(doc, "sg_a", &ids);
    CHECK(ids.count("sg_a_c") == 1);
    CHECK(ids.count("conn_a") == 1);
}

static void test_suppress_and_style()
{
    DeviceDocument doc;
    doc.rootChildren.push_back(makeFreeInk("a", 0, 0, 5, 5));
    doc.rootChildren.push_back(makeFreeInk("b", 1, 1, 6, 6));

    CanvasFrame frame;
    frame.setPanelSize(100, 100);
    frame.applyDrawingRegion({0, 0, 100, 100}, true);
    FrameProjector proj;
    proj.frame = &frame;

    RenderRequest req;
    req.worldClip = proj.drawingWorldClip();
    req.suppressIds.insert("a");
    req.styles["b"] = StyleOverride{3.0};

    RecordingSink sink;
    HierarchyCullAlgorithm cull;
    cull.paint(doc, proj, req, sink);
    CHECK(sink.polylineCount == 1);
    CHECK(!sink.widths.empty());
    CHECK(sink.widths[0] > 2.0); // widthMul applied
}

static void test_cancel_aborts_walk()
{
    DeviceDocument doc;
    for (int i = 0; i < 20; ++i)
        doc.rootChildren.push_back(makeFreeInk("n" + std::to_string(i), 0, 0, 10, 10));

    CanvasFrame frame;
    frame.setPanelSize(100, 100);
    frame.applyDrawingRegion({0, 0, 100, 100}, true);
    FrameProjector proj;
    proj.frame = &frame;

    std::atomic<bool> cancel{true};
    RenderRequest req;
    req.worldClip = proj.drawingWorldClip();
    req.cancel = &cancel;

    RecordingSink sink;
    HierarchyCullAlgorithm cull;
    cull.paint(doc, proj, req, sink);
    CHECK(sink.polylineCount == 0);
}

static void test_includeIds_skips_neighbors()
{
    DeviceDocument doc;
    doc.rootChildren.push_back(makeFreeInk("keep", 0, 0, 5, 5));
    doc.rootChildren.push_back(makeFreeInk("skip", 1, 1, 6, 6));

    CanvasFrame frame;
    frame.setPanelSize(100, 100);
    frame.applyDrawingRegion({0, 0, 100, 100}, true);
    FrameProjector proj;
    proj.frame = &frame;

    RenderRequest req;
    req.worldClip = proj.drawingWorldClip();
    req.includeIds.insert("keep");
    req.styles["keep"] = StyleOverride{2.0};

    RecordingSink sink;
    HierarchyCullAlgorithm cull;
    cull.paint(doc, proj, req, sink);
    CHECK(sink.polylineCount == 1);
}

static void test_inplace_dirty_clearRect_and_tight_clip()
{
    DeviceDocument doc;
    doc.rootChildren.push_back(makeFreeInk("near", 10, 10, 20, 20));
    doc.rootChildren.push_back(makeFreeInk("far", 8000, 8000, 8010, 8010));

    CanvasFrame frame;
    frame.setPanelSize(1404, 1872);
    frame.applyDrawingRegion({0, 0, 10000, 10000}, true);
    FrameProjector proj;
    proj.frame = &frame;

    RenderRequest req;
    req.sharp = true;
    req.mode = RenderRequest::BufferMode::InPlaceDirty;
    req.dirtyPanelX = 0;
    req.dirtyPanelY = 0;
    req.dirtyPanelW = 50;
    req.dirtyPanelH = 50;
    WorldAabb dirtyWorld;
    dirtyWorld.minX = 0;
    dirtyWorld.minY = 0;
    dirtyWorld.maxX = 50;
    dirtyWorld.maxY = 50;
    req.worldClip = intersectWorldAabb(proj.drawingWorldClip(), dirtyWorld);

    RecordingSink sink;
    DocumentRenderer renderer;
    renderer.setAlgorithm(std::make_unique<HierarchyCullAlgorithm>());
    renderer.render(doc, proj, req, sink);
    CHECK(sink.clearFullCount == 0);
    CHECK(sink.clearRectCount == 1);
    CHECK(sink.polylineCount == 1);
}

int main()
{
    test_full_clip_same_polylines();
    test_hierarchy_cull_skips_far_sg();
    test_sg_child_visible_when_panned_into_view();
    test_suppress_skips_subtree();
    test_renderSubtree_sg_and_connector();
    test_collectManipSuppressIds();
    test_suppress_and_style();
    test_includeIds_skips_neighbors();
    test_cancel_aborts_walk();
    test_inplace_dirty_clearRect_and_tight_clip();
    if (g_fails) {
        std::cerr << "rendering_test: " << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "rendering_test OK\n";
    return 0;
}
