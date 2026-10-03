/**
 * Phase 1 — CanvasFrame host tests (no Qt).
 * Round-trip panel⇄world, orientation, AABB intents.
 */

#include "drawing/canvas_frame.hpp"

#include <cmath>
#include <iostream>
#include <string>

using namespace epaper::canvasframe;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static bool near(double a, double b, double eps = 1e-6)
{
    return std::abs(a - b) <= eps;
}

static void test_normalize_orientation()
{
    CHECK(normalizeOrientation("portrait") == "gutToLeft");
    CHECK(normalizeOrientation("landscape") == "gutOnTop");
    CHECK(normalizeOrientation("gutToRight") == "gutToRight");
    CHECK(normalizeOrientation("bogus") == "gutToLeft");
}

static void test_set_orientation_intent()
{
    CanvasFrame f;
    CHECK(f.setOrientation("gutToLeft") == FrameIntent::None);
    CHECK(has(f.setOrientation("gutOnTop"), FrameIntent::OrientationChanged));
    CHECK(f.orientation == "gutOnTop");
    CHECK(f.landscape());
    CHECK(f.setOrientation("gutOnTop") == FrameIntent::None);
}

static void test_ensure_local_camera()
{
    CanvasFrame f;
    f.setPanelSize(1404, 1872);
    CHECK(!f.drawingRegion.valid);
    CHECK(has(f.ensureLocalDrawingRegion(), FrameIntent::CameraChanged));
    CHECK(f.drawingRegion.valid);
    CHECK(near(f.drawingRegion.maxX, 1404));
    CHECK(near(f.drawingRegion.maxY, 1872));
    CHECK(f.ensureLocalDrawingRegion() == FrameIntent::None);
}

static void test_identity_without_camera()
{
    CanvasFrame f;
    f.setPanelSize(100, 200);
    const WorldPt w = f.panelToWorld({10, 20});
    CHECK(near(w.x, 10) && near(w.y, 20));
    const PanelPt p = f.worldToPanel(30, 40);
    CHECK(near(p.x, 30) && near(p.y, 40));
}

static void test_panel_world_roundtrip_portrait()
{
    CanvasFrame f;
    f.setPanelSize(100, 200);
    f.orientation = "gutToLeft";
    f.applyDrawingRegion({0, 0, 50, 100}, false);
    const PanelPt corners[] = {{0, 0}, {100, 0}, {0, 200}, {100, 200}, {50, 100}};
    for (const PanelPt &p : corners) {
        const WorldPt w = f.panelToWorld(p);
        const PanelPt back = f.worldToPanel(w);
        CHECK(near(back.x, p.x) && near(back.y, p.y));
    }
    const WorldPt mid = f.panelToWorld({50, 100});
    CHECK(near(mid.x, 25) && near(mid.y, 50));
}

static void test_panel_world_roundtrip_landscape()
{
    CanvasFrame f;
    f.setPanelSize(100, 200);
    f.setOrientation("gutOnTop");
    f.applyDrawingRegion({0, 0, 50, 100}, false);
    const PanelPt corners[] = {{0, 0}, {100, 0}, {0, 200}, {100, 200}};
    for (const PanelPt &p : corners) {
        const WorldPt w = f.panelToWorld(p);
        const PanelPt back = f.worldToPanel(w);
        CHECK(near(back.x, p.x, 1e-5) && near(back.y, p.y, 1e-5));
    }
}

static void test_aabb_box_setbox()
{
    WorldAabb a;
    a.setBox({1, 2, 3, 4});
    CHECK(a.valid);
    CHECK(near(a.minX, 1) && near(a.maxY, 4));
    const auto b = a.box();
    CHECK(near(b.minX, 1) && near(b.maxX, 3));
    CanvasFrame f;
    CHECK(has(f.applyDrawingRegion({0, 0, 0, 0}, true), FrameIntent::CameraChanged));
    CHECK(!f.drawingRegion.valid); // requireNonEmpty → empty box is invalid
}

static void test_panel_scale_and_lod()
{
    CanvasFrame f;
    f.setPanelSize(100, 100);
    f.applyDrawingRegion({0, 0, 200, 200}, false); // zoomed out 0.5x
    CHECK(f.viewportZoomedOut());
    CHECK(near(f.panelScale(), 0.5));
    // Tiny world box → small panel AABB when zoomed out → lod may fail
    CHECK(f.lodOkPanel(0, 0, 200, 200)); // full region fills panel → ok
}

int main()
{
    test_normalize_orientation();
    test_set_orientation_intent();
    test_ensure_local_camera();
    test_identity_without_camera();
    test_panel_world_roundtrip_portrait();
    test_panel_world_roundtrip_landscape();
    test_aabb_box_setbox();
    test_panel_scale_and_lod();
    if (g_fails) {
        std::cerr << "canvas_frame_test: " << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "canvas_frame_test ok\n";
    return 0;
}
