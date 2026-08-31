#pragma once

/**
 * Off-GUI vector rasterize of the document at a camera snapshot.
 * TabletCanvas blits m_image for pan/zoom preview; LatestJob runs this and
 * delivers a sharp QImage. Newer camera replaces pending only; in-flight is
 * allowed to finish so the GUI can warp it toward `now` (SRS-EP-03).
 * @implements [SRS-EP-03]
 * @implements [SRS-EP-01]
 */

#include "canvas_frame.hpp"
#include "debug/rasterize_probe.hpp"
#include "document/connector_warp.hpp"
#include "document/device_document.hpp"
#include "document/doc_model.hpp"
#include "rendering/rendering.hpp"
#include "rendering/rendering_qt.hpp"

#include <QElapsedTimer>
#include <QImage>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace epaper {
namespace camerasharp {

struct Job {
    std::uint64_t id = 0;
    std::uint64_t snapEpoch = 0;
    canvasframe::CanvasFrame frame;
    std::shared_ptr<const std::vector<document::DocNode>> tree;
    std::unordered_set<std::string> suppressIds;
    int imageW = 0;
    int imageH = 0;
    QImage::Format format = QImage::Format_RGB32;
};

struct Result {
    std::uint64_t id = 0;
    std::uint64_t snapEpoch = 0;
    rasterprobe::CamBox cam;
    std::string orient;
    QImage image;
    render::LastPaintStats stats;
    int renderMs = 0;
    bool cancelled = false;
};

inline rasterprobe::CamBox camBoxOf(const canvasframe::CanvasFrame &f)
{
    rasterprobe::CamBox b;
    b.valid = f.drawingRegion.valid;
    b.minX = f.drawingRegion.minX;
    b.minY = f.drawingRegion.minY;
    b.maxX = f.drawingRegion.maxX;
    b.maxY = f.drawingRegion.maxY;
    return b;
}

inline Result run(const Job &job, const std::atomic<bool> &cancel)
{
    Result r;
    r.id = job.id;
    r.snapEpoch = job.snapEpoch;
    r.cam = camBoxOf(job.frame);
    r.orient = job.frame.orientation;
    if (cancel.load() || !job.tree || job.imageW < 1 || job.imageH < 1) {
        r.cancelled = true;
        return r;
    }

    document::DeviceDocument doc;
    doc.rootChildren = *job.tree;
    document::refreshAllConnectorWarps(doc);
    if (cancel.load()) {
        r.cancelled = true;
        return r;
    }

    r.image = QImage(job.imageW, job.imageH, job.format);
    if (r.image.isNull()) {
        r.cancelled = true;
        return r;
    }
    r.image.fill(Qt::white);

    render::FrameProjector proj;
    proj.frame = &job.frame;
    render::RenderRequest req;
    req.sharp = true;
    req.mode = render::RenderRequest::BufferMode::FullClear;
    req.worldClip = proj.drawingWorldClip();
    req.suppressIds = job.suppressIds;
    req.cancel = &cancel;

    render::DocumentRenderer renderer;
    renderer.setAlgorithm(std::make_unique<render::HierarchyCullAlgorithm>());
    QElapsedTimer clock;
    clock.start();
    {
        render::QImagePixelSink sink(&r.image);
        renderer.render(doc, proj, req, sink);
    }
    r.renderMs = int(clock.elapsed());
    if (renderer.algorithm())
        r.stats = renderer.algorithm()->lastPaintStats();
    r.cancelled = cancel.load();
    return r;
}

} // namespace camerasharp
} // namespace epaper
