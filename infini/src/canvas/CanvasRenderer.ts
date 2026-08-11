/**
 * @implements [SRS-IN-01] Viewport-scoped paint with optional tile LOD
 *
 * ml-mindmap painted the entire Map every wheel tick. We:
 * 1) compute visible world AABB
 * 2) query spatial index
 * 3) below TILE_LOD_SCALE, blit cached tiles instead of live vectors
 */

import type { InfiniDocument } from "./Document";
import { drawPrimitiveWorld } from "./drawPrimitive";
import {
  TileCache,
  shouldUseTileLod,
  tilesForAabb,
  TILE_WORLD_SIZE,
} from "./TileCache";
import type { Viewport } from "./Viewport";
import { visibleWorldAabb, worldToScreen } from "./Viewport";

export interface PaintStats {
  candidates: number;
  usedTileLod: boolean;
  indexMode: "flat" | "quadtree";
}

export class CanvasRenderer {
  readonly tiles = new TileCache();

  invalidateTiles(): void {
    this.tiles.invalidate();
  }

  paint(
    ctx: CanvasRenderingContext2D,
    cssW: number,
    cssH: number,
    dpr: number,
    vp: Viewport,
    doc: InfiniDocument,
  ): PaintStats {
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

    // Paper + grid in screen space (matches design common.css feel).
    ctx.fillStyle = "#F2F4F7";
    ctx.fillRect(0, 0, cssW, cssH);
    this.drawGrid(ctx, cssW, cssH, vp);

    const view = visibleWorldAabb(cssW, cssH, vp);
    const candidates = doc.queryVisible(view);
    const usedTileLod = shouldUseTileLod(vp);

    if (usedTileLod) {
      this.paintTiles(ctx, vp, view, candidates, dpr);
    } else {
      ctx.save();
      // screen = (world + t) * s  →  translate(t*s) then scale(s)
      ctx.translate(vp.translate.x * vp.scale, vp.translate.y * vp.scale);
      ctx.scale(vp.scale, vp.scale);
      for (const p of candidates) {
        drawPrimitiveWorld(ctx, p, 1);
      }
      ctx.restore();
    }

    return {
      candidates: candidates.length,
      usedTileLod,
      indexMode: doc.indexMode,
    };
  }

  private paintTiles(
    ctx: CanvasRenderingContext2D,
    vp: Viewport,
    view: ReturnType<typeof visibleWorldAabb>,
    candidates: ReturnType<InfiniDocument["queryVisible"]>,
    dpr: number,
  ): void {
    const keys = tilesForAabb(view, TILE_WORLD_SIZE);
    for (const key of keys) {
      const tile = this.tiles.getOrRasterize(key, candidates, dpr, TILE_WORLD_SIZE);
      const worldX = key.tx * TILE_WORLD_SIZE;
      const worldY = key.ty * TILE_WORLD_SIZE;
      const tl = worldToScreen({ x: worldX, y: worldY }, vp);
      const br = worldToScreen(
        { x: worldX + TILE_WORLD_SIZE, y: worldY + TILE_WORLD_SIZE },
        vp,
      );
      ctx.drawImage(tile, tl.x, tl.y, br.x - tl.x, br.y - tl.y);
    }
  }

  private drawGrid(
    ctx: CanvasRenderingContext2D,
    cssW: number,
    cssH: number,
    vp: Viewport,
  ): void {
    // Keep line count bounded — dense grids dominate fill-rate when zoomed in.
    let step = 48 * vp.scale;
    while (step < 12) step *= 2;
    while (step > 96) step /= 2;
    if (step < 8) return;

    const origin = worldToScreen({ x: 0, y: 0 }, vp);
    ctx.strokeStyle = "#C5CCD6";
    ctx.lineWidth = 1;
    ctx.beginPath();
    const ox = ((origin.x % step) + step) % step;
    const oy = ((origin.y % step) + step) % step;
    for (let x = ox; x <= cssW; x += step) {
      ctx.moveTo(x + 0.5, 0);
      ctx.lineTo(x + 0.5, cssH);
    }
    for (let y = oy; y <= cssH; y += step) {
      ctx.moveTo(0, y + 0.5);
      ctx.lineTo(cssW, y + 0.5);
    }
    ctx.stroke();
  }
}
