#pragma once

/**
 * When a full document rasterize may run.
 *
 * Live ink and the erase ghost must not be wiped by a FullClear of TabletCanvas.
 * Live move/resize/rotate must *not* defer: TransformGesture punches suppressIds
 * so the origin disappears on the document surface while ToolCanvas paints the
 * live node.
 * @implements [SRS-EP-11] live node on ToolCanvas; origin hidden on CanvasLayer
 * @implements [SRS-EP-56] erase ghost vs document rasterize
 */

namespace epaper {
namespace render {

inline bool deferFullDocumentRasterize(bool inkStrokeActive, bool erasePointerActive)
{
    return inkStrokeActive || erasePointerActive;
}

} // namespace rend
} // namespace epaper
