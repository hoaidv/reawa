#pragma once

/**
 * Qt-free document → panel primitives. Algorithms own any acceleration structures;
 * DocNode stays a value POD with no render caches.
 */

#include "document/device_document.hpp"
#include "drawing/canvas_frame.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace epaper {
namespace render {

struct WorldAabb {
    double minX = 0;
    double minY = 0;
    double maxX = 0;
    double maxY = 0;

    bool valid() const { return maxX > minX && maxY > minY; }
};

/** [D03] Invalid clip means “no restriction”. */
inline WorldAabb intersectWorldAabb(WorldAabb a, WorldAabb b)
{
    if (!a.valid())
        return b;
    if (!b.valid())
        return a;
    WorldAabb o;
    o.minX = a.minX > b.minX ? a.minX : b.minX;
    o.minY = a.minY > b.minY ? a.minY : b.minY;
    o.maxX = a.maxX < b.maxX ? a.maxX : b.maxX;
    o.maxY = a.maxY < b.maxY ? a.maxY : b.maxY;
    return o;
}

struct StyleOverride {
    double widthMul = 1.0;
};

struct RenderRequest {
    WorldAabb worldClip;
    bool sharp = true;
    enum class BufferMode { FullClear, InPlaceDirty } mode = BufferMode::FullClear;
    /** Panel-space dirty when mode == InPlaceDirty (ignored for FullClear). */
    double dirtyPanelX = 0;
    double dirtyPanelY = 0;
    double dirtyPanelW = 0;
    double dirtyPanelH = 0;
    std::unordered_set<std::string> suppressIds;
    std::unordered_map<std::string, StyleOverride> styles;
};

struct PanelPolyline {
    std::vector<std::pair<double, double>> pts;
    double width = 1;
};

class IPixelSink {
public:
    virtual ~IPixelSink() = default;
    virtual void begin(bool sharp) = 0;
    virtual void clearFull() = 0;
    virtual void clearRect(double x, double y, double w, double h) = 0;
    virtual void drawPolyline(const PanelPolyline &poly) = 0;
    virtual void end() = 0;
};

/** World→panel using CanvasFrame (no Qt). */
struct FrameProjector {
    const canvasframe::CanvasFrame *frame = nullptr;

    void worldToPanel(double wx, double wy, double *px, double *py) const;
    WorldAabb worldToPanelAabb(WorldAabb w) const;
    double panelScale() const;
    WorldAabb drawingWorldClip() const;
};

class IRenderAlgorithm {
public:
    virtual ~IRenderAlgorithm() = default;
    virtual void invalidateAll() {}
    virtual void invalidateIds(const std::vector<std::string> &) {}
    /** Nodes entered during the last paint (containers + leaves). */
    virtual std::size_t lastVisitCount() const { return 0; }
    virtual void paint(const document::DeviceDocument &doc, const FrameProjector &proj,
                       const RenderRequest &req, IPixelSink &sink) = 0;
};

/** DFS paint — Θ(N) visit. */
class FlatWalkAlgorithm : public IRenderAlgorithm {
public:
    std::size_t lastVisitCount() const override { return m_visits; }
    void paint(const document::DeviceDocument &doc, const FrameProjector &proj,
               const RenderRequest &req, IPixelSink &sink) override;

private:
    std::size_t m_visits = 0;
};

/** Skip container subtrees whose world AABB misses worldClip. */
class HierarchyCullAlgorithm : public IRenderAlgorithm {
public:
    std::size_t lastVisitCount() const override { return m_visits; }
    void paint(const document::DeviceDocument &doc, const FrameProjector &proj,
               const RenderRequest &req, IPixelSink &sink) override;

private:
    std::size_t m_visits = 0;
};

/** Ink/primitive ids under a SmartGroup plus bound root connectors — for live-manip suppress. */
void collectManipSuppressIds(const document::DeviceDocument &doc, const std::string &sgId,
                             std::unordered_set<std::string> *out);

class DocumentRenderer {
public:
    void setAlgorithm(std::unique_ptr<IRenderAlgorithm> alg);
    IRenderAlgorithm *algorithm() { return m_alg.get(); }
    const IRenderAlgorithm *algorithm() const { return m_alg.get(); }

    void render(const document::DeviceDocument &doc, const FrameProjector &proj,
                const RenderRequest &req, IPixelSink &sink);

    /** SmartGroup children + bound connectors — ToolCanvas live-manip ghost. */
    void renderSubtree(const document::DeviceDocument &doc, const FrameProjector &proj,
                       const RenderRequest &req, const std::string &rootId, IPixelSink &sink);

private:
    std::unique_ptr<IRenderAlgorithm> m_alg;
};

} // namespace render
} // namespace epaper
