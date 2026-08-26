/**
 * Host tests for epaper::render — FlatWalk vs HierarchyCull (no Qt).
 */

#include "drawing/canvas_frame.hpp"
#include "document/device_document.hpp"
#include "rendering/rendering.hpp"

#include <cmath>
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
    std::vector<double> widths;

    void begin(bool) override {}
    void clearFull() override {}
    void clearRect(double, double, double, double) override {}
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

int main()
{
    test_full_clip_same_polylines();
    test_hierarchy_cull_skips_far_sg();
    test_sg_child_visible_when_panned_into_view();
    test_suppress_and_style();
    if (g_fails) {
        std::cerr << "rendering_test: " << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "rendering_test OK\n";
    return 0;
}
