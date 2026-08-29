/**
 * Live manip overlay contract: origin hidden on document rasterize, still
 * paintable on the tool layer; rasterize deferral is erase/ink only.
 * @implements [SRS-EP-11] live node on ToolCanvas; origin hidden on CanvasLayer
 * @implements [BR-B19] live node paint is overlay, settle is document
 */

#include "drawing/canvas_frame.hpp"
#include "drawing/rasterize_gate.hpp"
#include "document/device_document.hpp"
#include "rendering/rendering.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>

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
    void begin(bool) override {}
    void clearFull() override {}
    void clearRect(double, double, double, double) override {}
    void drawPolyline(const PanelPolyline &poly) override
    {
        if (poly.pts.size() >= 2)
            ++polylineCount;
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

static DocNode makeSmartGroup(const std::string &id)
{
    DocNode sg;
    sg.id = id;
    sg.kind = NodeKind::SmartGroup;
    sg.smartBounds = {0, 0, 80, 60};
    sg.transform = {10, 20, 0, 1, 1};
    sg.inkScaleMode = "withBounds";
    DocNode child;
    child.id = id + "_c";
    child.kind = NodeKind::Ink;
    child.role = std::string("content");
    child.style.strokeWidth = 2;
    child.samples.push_back(sample(0, 0));
    child.samples.push_back(sample(40, 20));
    sg.children.push_back(std::move(child));
    return sg;
}

static DocNode makeConnector(const std::string &id, const std::string &from, const std::string &to)
{
    DocNode conn;
    conn.id = id;
    conn.kind = NodeKind::Connector;
    conn.fromNodeId = from;
    conn.toNodeId = to;
    conn.style.strokeWidth = 2;
    ConnectorRestPt a;
    a.x = 5;
    a.y = 5;
    ConnectorRestPt b;
    b.x = 50;
    b.y = 40;
    conn.warpedSamples.push_back(a);
    conn.warpedSamples.push_back(b);
    return conn;
}

static FrameProjector projector()
{
    static CanvasFrame frame;
    frame.setPanelSize(200, 200);
    frame.applyDrawingRegion({0, 0, 200, 200}, true);
    FrameProjector proj;
    proj.frame = &frame;
    return proj;
}

static int paintCount(const DeviceDocument &doc, const RenderRequest &req)
{
    RecordingSink sink;
    HierarchyCullAlgorithm cull;
    cull.paint(doc, projector(), req, sink);
    return sink.polylineCount;
}

static int subtreeCount(const DeviceDocument &doc, const std::string &id)
{
    RecordingSink sink;
    DocumentRenderer renderer;
    renderer.setAlgorithm(std::make_unique<FlatWalkAlgorithm>());
    RenderRequest req;
    req.sharp = true;
    renderer.renderSubtree(doc, projector(), req, id, sink);
    return sink.polylineCount;
}

static DeviceDocument makeLiveManipDoc()
{
    DeviceDocument doc;
    doc.rootChildren.push_back(makeSmartGroup("sg_a"));
    DocNode bg;
    bg.id = "ink_bg";
    bg.kind = NodeKind::Ink;
    bg.style.strokeWidth = 2;
    bg.samples.push_back(sample(90, 90));
    bg.samples.push_back(sample(120, 110));
    doc.rootChildren.push_back(std::move(bg));
    doc.rootChildren.push_back(makeConnector("conn_a", "sg_a", "other"));
    return doc;
}

static void test_rasterize_defer_erase_not_transform()
{
    CHECK(deferFullDocumentRasterize(true, false));
    CHECK(deferFullDocumentRasterize(false, true));
    CHECK(deferFullDocumentRasterize(true, true));
    CHECK(!deferFullDocumentRasterize(false, false));
}

static void test_move_hides_origin_tool_layer_keeps_it()
{
    DeviceDocument doc = makeLiveManipDoc();
    RenderRequest shown;
    shown.worldClip = projector().drawingWorldClip();
    CHECK(paintCount(doc, shown) == 3); // child ink + bg + connector

    std::unordered_set<std::string> suppress;
    collectManipSuppressIds(doc, "sg_a", &suppress);
    CHECK(suppress.count("sg_a_c") == 1);
    CHECK(suppress.count("conn_a") == 1);
    CHECK(suppress.count("ink_bg") == 0);

    RenderRequest punched = shown;
    punched.suppressIds = suppress;
    CHECK(paintCount(doc, punched) == 1); // only uninvolved background ink
    CHECK(subtreeCount(doc, "sg_a") == 2); // tool layer: child + bound connector
}

static void test_resize_same_suppress_as_move()
{
    DeviceDocument doc = makeLiveManipDoc();
    std::unordered_set<std::string> ids;
    collectManipSuppressIds(doc, "sg_a", &ids);
    // Handle vs body is a gesture choice; the origin punch is always the SG
    // subtree + bound connectors so resize cannot leave a second copy behind.
    CHECK(ids.count("sg_a_c") == 1);
    CHECK(ids.count("conn_a") == 1);
}

static void test_rotate_live_geometry_still_punches_origin()
{
    DeviceDocument doc = makeLiveManipDoc();
    DocNode *sg = doc.mutableFind("sg_a");
    CHECK(sg != nullptr);
    sg->transform.rotation = 0.4;
    std::unordered_set<std::string> suppress;
    collectManipSuppressIds(doc, "sg_a", &suppress);
    RenderRequest req;
    req.worldClip = projector().drawingWorldClip();
    req.suppressIds = suppress;
    CHECK(paintCount(doc, req) == 1);
    CHECK(subtreeCount(doc, "sg_a") >= 1);
}

static void test_commit_clears_suppress_origin_returns()
{
    DeviceDocument doc = makeLiveManipDoc();
    std::unordered_set<std::string> suppress;
    collectManipSuppressIds(doc, "sg_a", &suppress);
    RenderRequest live;
    live.worldClip = projector().drawingWorldClip();
    live.suppressIds = suppress;
    CHECK(paintCount(doc, live) == 1);

    RenderRequest settled;
    settled.worldClip = live.worldClip;
    CHECK(paintCount(doc, settled) == 3);
}

static void test_connector_to_node_also_hidden()
{
    DeviceDocument doc;
    doc.rootChildren.push_back(makeSmartGroup("sg_a"));
    doc.rootChildren.push_back(makeSmartGroup("sg_b"));
    doc.rootChildren.push_back(makeConnector("conn_in", "sg_b", "sg_a"));
    std::unordered_set<std::string> ids;
    collectManipSuppressIds(doc, "sg_a", &ids);
    CHECK(ids.count("conn_in") == 1);
}

int main()
{
    test_rasterize_defer_erase_not_transform();
    test_move_hides_origin_tool_layer_keeps_it();
    test_resize_same_suppress_as_move();
    test_rotate_live_geometry_still_punches_origin();
    test_commit_clears_suppress_origin_returns();
    test_connector_to_node_also_hidden();
    if (g_fails) {
        std::cerr << "live_manip_overlay_test: " << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "live_manip_overlay_test OK\n";
    return 0;
}
