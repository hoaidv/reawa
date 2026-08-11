/**
 * Zoom-out acceleration: tile / group image cache.
 *
 * Problem (from product critique of ml-mindmap full redraw):
 * - Zoom-in → fewer on-screen vectors → cheaper.
 * - Zoom-out → more vectors → slower.
 *
 * Mitigation: below a scale threshold, paint vectors into world-space tiles once,
 * then blit scaled tile bitmaps. Interaction with *individual* vectors is disabled
 * at that LOD (pick hits the tile / nothing).
 *
 * Threshold defaults are heuristics — tune with STORY-IN-005 frame budget.
 */

import type { Aabb, Viewport } from "./Viewport";
import type { Primitive } from "./primitives";
import { drawPrimitiveWorld } from "./drawPrimitive";

/** Below this scale, prefer tile blit over live vector paint. */
export const TILE_LOD_SCALE = 0.35;

/** World-space tile size (units). */
export const TILE_WORLD_SIZE = 512;

export interface TileKey {
  tx: number;
  ty: number;
}

export function tileKey(tx: number, ty: number): string {
  return `${tx},${ty}`;
}

export function tilesForAabb(aabb: Aabb, tileSize = TILE_WORLD_SIZE): TileKey[] {
  const x0 = Math.floor(aabb.minX / tileSize);
  const y0 = Math.floor(aabb.minY / tileSize);
  const x1 = Math.floor(aabb.maxX / tileSize);
  const y1 = Math.floor(aabb.maxY / tileSize);
  const out: TileKey[] = [];
  for (let ty = y0; ty <= y1; ty++) {
    for (let tx = x0; tx <= x1; tx++) {
      out.push({ tx, ty });
    }
  }
  return out;
}

export function shouldUseTileLod(vp: Viewport): boolean {
  return vp.scale < TILE_LOD_SCALE;
}

/** Individual vector hit-testing / editing allowed only above LOD. */
export function allowIndividualInteraction(vp: Viewport): boolean {
  return !shouldUseTileLod(vp);
}

export class TileCache {
  private tiles = new Map<string, HTMLCanvasElement>();
  private generation = 0;

  invalidate(): void {
    this.tiles.clear();
    this.generation++;
  }

  get generationId(): number {
    return this.generation;
  }

  getOrRasterize(
    key: TileKey,
    items: Primitive[],
    dpr: number,
    tileSize = TILE_WORLD_SIZE,
  ): HTMLCanvasElement {
    const id = tileKey(key.tx, key.ty);
    const hit = this.tiles.get(id);
    if (hit) return hit;

    const world: Aabb = {
      minX: key.tx * tileSize,
      minY: key.ty * tileSize,
      maxX: (key.tx + 1) * tileSize,
      maxY: (key.ty + 1) * tileSize,
    };

    // Rasterize at ~1 CSS px per world unit so scale-down on zoom-out stays sharp enough.
    const css = tileSize;
    const canvas = document.createElement("canvas");
    canvas.width = Math.max(1, Math.floor(css * dpr));
    canvas.height = Math.max(1, Math.floor(css * dpr));
    const ctx = canvas.getContext("2d")!;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.translate(-world.minX, -world.minY);
    for (const p of items) {
      if (
        p.bounds.maxX < world.minX ||
        p.bounds.minX > world.maxX ||
        p.bounds.maxY < world.minY ||
        p.bounds.minY > world.maxY
      ) {
        continue;
      }
      drawPrimitiveWorld(ctx, p, 1);
    }
    this.tiles.set(id, canvas);
    return canvas;
  }
}
