#pragma once

/**
 * Viewport — camera / rasterize sink for NavigationOperation (not HostCaps).
 * @implements [SRS-EP-21] @implements [SRS-EP-24]
 */

#include "../canvas_frame.hpp"
#include "document/hand_touch.hpp"

namespace epaper {
namespace tools {

class Viewport {
public:
    virtual ~Viewport() = default;
    virtual void applyCamera(const epaper::handtouch::WorldAabb &region, bool markValid) = 0;
    virtual void publishViewport(bool settle) = 0;
    virtual void scheduleRasterize(bool sharp) = 0;
    virtual void ensureDrawingRegion() = 0;
    virtual epaper::canvasframe::WorldAabb drawingRegion() const = 0;
    virtual epaper::handtouch::FollowDirection follow() const = 0;
    virtual epaper::handtouch::TwoFingerContacts uvPair(double ax, double ay, double bx,
                                                        double by) const = 0;
    virtual void worldThroughPanOrigin(const epaper::canvasframe::WorldAabb &origin, double px,
                                       double py, double *wx, double *wy) const = 0;
};

} // namespace tools
} // namespace epaper
