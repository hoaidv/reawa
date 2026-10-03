#pragma once

/**
 * When a full document rasterize may run.
 *
 * Live ink and the erase ghost must not be wiped by a FullClear of TabletCanvas.
 * Live move/resize/rotate must *not* defer: TransformGesture punches suppressIds
 * so the origin disappears on the document surface while ToolCanvas paints the
 * live node.
 *
 * Vector FullClear also waits while a pointer/pinch callback is on the stack
 * (deferVectorRasterize). Camera blit may still run.
 * @implements [SRS-EP-11] live node on ToolCanvas; origin hidden on CanvasLayer
 * @implements [SRS-EP-56] erase ghost vs document rasterize
 * @implements [SRS-EP-01] do not steal GUI thread on the pointer stack
 */

namespace epaper {
namespace render {

inline bool deferFullDocumentRasterize(bool inkStrokeActive, bool erasePointerActive)
{
    return inkStrokeActive || erasePointerActive;
}

/** Vector FullClear must not run on the pointer stack (SRS-EP-01). Camera blit may. */
inline bool deferVectorRasterize(bool inkStrokeActive, bool erasePointerActive, bool pointerBusy)
{
    return inkStrokeActive || erasePointerActive || pointerBusy;
}

} // namespace render
} // namespace epaper
