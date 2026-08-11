/**
 * Bridge flattened tree drawables → canvas Primitive[] for WorldLayer paint.
 * @implements [SRS-IN-04] WorldLayer cull/paint from flattenDrawables
 */

import type { Primitive } from "../canvas/primitives";
import {
  makeEllipse,
  makeLine,
  makePath,
  makeRect,
} from "../canvas/primitives";
import type { Drawable } from "./types";

export function drawablesToPrimitives(drawables: Drawable[]): Primitive[] {
  const out: Primitive[] = [];
  for (const d of drawables) {
    switch (d.kind) {
      case "ink": {
        const points = d.samples.map((s) => ({ x: s.x, y: s.y }));
        out.push(makePath(d.id, points, d.style));
        break;
      }
      case "primitive": {
        const g = d.geom;
        if (g.kind === "rect") {
          out.push(makeRect(d.id, g.x, g.y, g.w, g.h, d.style));
        } else if (g.kind === "ellipse") {
          out.push(makeEllipse(d.id, g.cx, g.cy, g.rx, g.ry, d.style));
        } else {
          out.push(makeLine(d.id, g.x1, g.y1, g.x2, g.y2, d.style));
        }
        break;
      }
      case "text": {
        // Text paint deferred; contribute AABB as empty path for cull presence.
        out.push(
          makeRect(
            d.id,
            d.box.minX,
            d.box.minY,
            d.box.maxX - d.box.minX,
            d.box.maxY - d.box.minY,
            { ...d.style, fill: d.style.fill ?? "transparent" },
          ),
        );
        break;
      }
      case "connector": {
        if (d.invalid || d.path.length < 2) break;
        out.push(makePath(d.id, d.path, { stroke: "#5B6B7C", strokeWidth: 1.5 }));
        break;
      }
    }
  }
  return out;
}
