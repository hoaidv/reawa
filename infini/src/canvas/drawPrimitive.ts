/**
 * @implements [SRS-IN-01] Draw a primitive in *world* space (CTM already applied).
 */

import type { Primitive } from "./primitives";

export function drawPrimitiveWorld(
  ctx: CanvasRenderingContext2D,
  p: Primitive,
  /** Divide stroke by this if you want screen-constant width; 1 = world width. */
  strokeScale = 1,
): void {
  ctx.save();
  ctx.strokeStyle = p.style.stroke;
  ctx.fillStyle = p.style.fill ?? "transparent";
  ctx.lineWidth = p.style.strokeWidth / strokeScale;
  ctx.lineJoin = "round";
  ctx.lineCap = "round";

  switch (p.kind) {
    case "line":
      ctx.beginPath();
      ctx.moveTo(p.x1, p.y1);
      ctx.lineTo(p.x2, p.y2);
      ctx.stroke();
      break;
    case "rect":
      ctx.beginPath();
      ctx.rect(p.x, p.y, p.w, p.h);
      if (p.style.fill) ctx.fill();
      ctx.stroke();
      break;
    case "ellipse":
      ctx.beginPath();
      ctx.ellipse(p.cx, p.cy, p.rx, p.ry, 0, 0, Math.PI * 2);
      if (p.style.fill) ctx.fill();
      ctx.stroke();
      break;
    case "path":
      if (p.points.length < 2) break;
      ctx.beginPath();
      ctx.moveTo(p.points[0].x, p.points[0].y);
      for (let i = 1; i < p.points.length; i++) {
        ctx.lineTo(p.points[i].x, p.points[i].y);
      }
      ctx.stroke();
      break;
  }
  ctx.restore();
}
