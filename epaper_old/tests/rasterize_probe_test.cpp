/**
 * Camera rasterize probe: pan vs zoom classify + log format.
 * Host test, no Qt.
 */
#include "debug/rasterize_probe.hpp"

#include <cstdio>
#include <string>

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
            return 1;                                                                          \
        }                                                                                      \
    } while (0)

using epaper::rasterprobe::CamBox;
using epaper::rasterprobe::CamKind;
using epaper::rasterprobe::CameraPaint;
using epaper::rasterprobe::Record;
using epaper::rasterprobe::Why;
using epaper::rasterprobe::classifyCamera;
using epaper::rasterprobe::formatRasterLine;
using epaper::rasterprobe::planCameraPaint;

static CamBox box(double minX, double minY, double maxX, double maxY)
{
    CamBox b;
    b.valid = true;
    b.minX = minX;
    b.minY = minY;
    b.maxX = maxX;
    b.maxY = maxY;
    return b;
}

int main()
{
    CamBox invalid;
    const auto first = classifyCamera(invalid, box(0, 0, 1404, 1872), 1404, false);
    CHECK(first.kind == CamKind::First);

    const auto pan = classifyCamera(box(0, 0, 1404, 1872), box(40, 10, 1444, 1882), 1404, false);
    CHECK(pan.kind == CamKind::Pan);
    CHECK(pan.zoom > 0.99 && pan.zoom < 1.01);
    CHECK(pan.panPx > 10.0);

    const auto zoom = classifyCamera(box(0, 0, 1404, 1872), box(0, 0, 2808, 3744), 1404, false);
    CHECK(zoom.kind == CamKind::Zoom);
    CHECK(zoom.zoom > 1.9 && zoom.zoom < 2.1);

    const auto pinch =
        classifyCamera(box(0, 0, 1404, 1872), box(-200, -100, 1600, 2100), 1404, false);
    CHECK(pinch.kind == CamKind::PanZoom);

    const auto none = classifyCamera(box(0, 0, 1404, 1872), box(0, 0, 1404, 1872), 1404, false);
    CHECK(none.kind == CamKind::None);

    const auto orient = classifyCamera(box(0, 0, 1404, 1872), box(0, 0, 1404, 1872), 1404, true);
    CHECK(orient.kind == CamKind::Orient);

    CHECK(planCameraPaint(CamKind::None, false).paint == CameraPaint::Skip);
    CHECK(planCameraPaint(CamKind::None, true).paint == CameraPaint::Skip);
    const auto panSoft = planCameraPaint(CamKind::Pan, false);
    CHECK(panSoft.paint == CameraPaint::BlitPan);
    CHECK(!panSoft.fillStrips);
    const auto panSharp = planCameraPaint(CamKind::Pan, true);
    CHECK(panSharp.paint == CameraPaint::BlitPan);
    CHECK(!panSharp.fillStrips);
    CHECK(planCameraPaint(CamKind::Zoom, false).paint == CameraPaint::BlitScale);
    CHECK(planCameraPaint(CamKind::PanZoom, false).paint == CameraPaint::BlitScale);
    CHECK(planCameraPaint(CamKind::Zoom, true).paint == CameraPaint::BlitScale);
    CHECK(planCameraPaint(CamKind::PanZoom, true).paint == CameraPaint::BlitScale);
    CHECK(planCameraPaint(CamKind::First, false).paint == CameraPaint::Vector);
    CHECK(planCameraPaint(CamKind::Orient, true).paint == CameraPaint::Vector);

    Record r;
    r.why = Why::Camera;
    r.cam = pan;
    r.inplace = false;
    r.sharp = false;
    r.totalMs = 3210;
    r.warpMs = 14;
    r.renderMs = 3102;
    r.updateMs = 88;
    r.ink = 812;
    r.nodes = 940;
    r.samples = 44102;
    r.visits = 940;
    r.skipped = 12;
    r.polylines = 800;
    r.pts = 43880;
    const std::string line = formatRasterLine(r);
    CHECK(line.find("reason=camera") != std::string::npos);
    CHECK(line.find("cam=pan") != std::string::npos);
    CHECK(line.find("blit=0") != std::string::npos);
    CHECK(line.find("inplace=0") != std::string::npos);
    CHECK(line.find("render_ms=3102") != std::string::npos);
    CHECK(line.find("pts=43880") != std::string::npos);

    r.blit = true;
    const std::string blitLine = formatRasterLine(r);
    CHECK(blitLine.find("blit=1") != std::string::npos);

    epaper::rasterprobe::resetForTest();
    epaper::rasterprobe::setEnabledForTest(false);
    epaper::rasterprobe::logRaster(r);
    CHECK(epaper::rasterprobe::takeLastLogForTest().empty());

    epaper::rasterprobe::resetForTest();
    epaper::rasterprobe::setEnabledForTest(true);
    epaper::rasterprobe::logRaster(r);
    const std::string logged = epaper::rasterprobe::takeLastLogForTest();
    CHECK(logged.find("[raster]") != std::string::npos);
    CHECK(logged.find("reason=camera") != std::string::npos);

    std::printf("rasterize_probe_test: OK\n");
    return 0;
}
