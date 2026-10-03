#pragma once
/**
 * Camera / document rasterize attribution.
 *
 * Always-on (like ink-path). One line per rasterizeVectors. Use it to split
 * pan vs zoom, FullClear vs InPlaceDirty, and warp/render/update ms before
 * changing the camera paint path.
 *
 * @implements [SRS-EP-03] camera coalesce paint attribution
 * @implements [SRS-EP-01] do not steal GUI thread — measure first
 *
 * Env:
 *   EPAPER_RASTER=0     disable
 *   EPAPER_RASTER_LOG   path (default /tmp/epaper-raster.log)
 *
 * How to read a line:
 *   reason=camera blit=1             — pan/zoom preview, or a warped job
 *   reason=camera blit=0 sharp=1     — LatestJob swap (strips / AA). cam=none = exact camera
 *   reason=camera blit=0 cam=first   — GUI vector (first paint / orientation)
 *   reason=dirty inplace=1           — transform / erase AABB (should stay small)
 *   render_ms >> warp_ms+update_ms   — Qt stroke of visible vectors
 *   skip≈0 and pts huge              — dense on-camera ink; cull cannot help
 */

#include <cmath>
#include <cstdio>
#include <string>

namespace epaper {
namespace rasterprobe {

enum class Why { Unknown, Camera, Dirty, Mutated, Enclose, DocLoad };

enum class CamKind { None, First, Pan, Zoom, PanZoom, Orient };

/** What rasterizeVectors should do for a camera step. */
enum class CameraPaint { Skip, BlitPan, BlitScale, Vector };

struct CameraPlan {
    CameraPaint paint = CameraPaint::Vector;
    /** Unused: strips are filled by the LatestJob vector, not the blit. */
    bool fillStrips = false;
};

struct CamBox {
    bool valid = false;
    double minX = 0;
    double minY = 0;
    double maxX = 0;
    double maxY = 0;
};

struct CamDelta {
    CamKind kind = CamKind::None;
    double zoom = 1.0;    /**< new world-width / old; 1 = same scale. */
    double panPx = 0;     /**< origin shift in panel px at the old scale. */
};

struct Record {
    Why why = Why::Unknown;
    CamDelta cam;
    bool inplace = false;
    bool sharp = false;
    bool blit = false;
    int totalMs = 0;
    int warpMs = 0;
    int renderMs = 0;
    int updateMs = 0;
    int ink = 0;
    int nodes = 0;
    int samples = 0;
    int visits = 0;
    int skipped = 0;
    int polylines = 0;
    int pts = 0;
};

inline const char *whyName(Why w)
{
    switch (w) {
    case Why::Camera:
        return "camera";
    case Why::Dirty:
        return "dirty";
    case Why::Mutated:
        return "mutated";
    case Why::Enclose:
        return "enclose";
    case Why::DocLoad:
        return "doc_load";
    case Why::Unknown:
        break;
    }
    return "unknown";
}

inline const char *camName(CamKind k)
{
    switch (k) {
    case CamKind::First:
        return "first";
    case CamKind::Pan:
        return "pan";
    case CamKind::Zoom:
        return "zoom";
    case CamKind::PanZoom:
        return "panzoom";
    case CamKind::Orient:
        return "orient";
    case CamKind::None:
        break;
    }
    return "none";
}

/** Classify a camera step. @p orientChanged forces kind=orient when geometry is unchanged. */
inline CamDelta classifyCamera(CamBox prev, CamBox next, double panelW, bool orientChanged)
{
    CamDelta d;
    if (!prev.valid) {
        d.kind = CamKind::First;
        return d;
    }
    if (!next.valid)
        return d;

    const double oldW = prev.maxX - prev.minX;
    const double oldH = prev.maxY - prev.minY;
    const double newW = next.maxX - next.minX;
    const double newH = next.maxY - next.minY;
    d.zoom = oldW > 1e-9 ? newW / oldW : 1.0;
    const double zoomH = oldH > 1e-9 ? newH / oldH : 1.0;
    const bool sizeSame = std::abs(d.zoom - 1.0) < 0.005 && std::abs(zoomH - 1.0) < 0.005;
    const double dx = next.minX - prev.minX;
    const double dy = next.minY - prev.minY;
    const bool originMoved = std::abs(dx) > 1e-6 || std::abs(dy) > 1e-6;
    const double scale = (oldW > 1e-9 && panelW > 1.0) ? (panelW / oldW) : 1.0;
    d.panPx = std::hypot(dx, dy) * scale;

    if (sizeSame && !originMoved) {
        d.kind = orientChanged ? CamKind::Orient : CamKind::None;
        return d;
    }
    if (sizeSame && originMoved)
        d.kind = CamKind::Pan;
    else if (!sizeSame && !originMoved)
        d.kind = CamKind::Zoom;
    else
        d.kind = CamKind::PanZoom;
    if (orientChanged && d.kind == CamKind::None)
        d.kind = CamKind::Orient;
    return d;
}

/**
 * Camera paint plan. Soft pan/zoom blit the previous panel (SRS-EP-03 ghosting).
 * Newly revealed content and AA come from a LatestJob vector, not GUI-thread
 * strip fill. First / orient stay vector (no previous bitmap).
 * @implements [SRS-EP-03]
 */
inline CameraPlan planCameraPaint(CamKind kind, bool /*sharp*/)
{
    CameraPlan p;
    switch (kind) {
    case CamKind::None:
        p.paint = CameraPaint::Skip;
        return p;
    case CamKind::Pan:
        p.paint = CameraPaint::BlitPan;
        return p;
    case CamKind::Zoom:
    case CamKind::PanZoom:
        p.paint = CameraPaint::BlitScale;
        return p;
    case CamKind::First:
    case CamKind::Orient:
        break;
    }
    p.paint = CameraPaint::Vector;
    return p;
}

inline std::string formatRasterLine(const Record &r)
{
    char buf[768];
    std::snprintf(buf, sizeof(buf),
                  "[raster] reason=%s cam=%s blit=%d inplace=%d sharp=%d total_ms=%d warp_ms=%d "
                  "render_ms=%d update_ms=%d ink=%d nodes=%d samples=%d visits=%d skip=%d "
                  "polylines=%d pts=%d pan_px=%.1f zoom=%.3f\n",
                  whyName(r.why), camName(r.cam.kind), r.blit ? 1 : 0, r.inplace ? 1 : 0,
                  r.sharp ? 1 : 0, r.totalMs, r.warpMs, r.renderMs, r.updateMs, r.ink, r.nodes,
                  r.samples, r.visits, r.skipped, r.polylines, r.pts, r.cam.panPx, r.cam.zoom);
    return std::string(buf);
}

bool enabled();
void logRaster(const Record &r);

void resetForTest();
void setEnabledForTest(bool on);
std::string takeLastLogForTest();

} // namespace rasterprobe
} // namespace epaper
