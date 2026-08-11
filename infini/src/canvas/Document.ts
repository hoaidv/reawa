/**
 * @implements [SRS-IN-01] In-memory document + spatial index
 * @implements [SRS-IN-04] sync flattened tree into WorldLayer index
 */

import { SpatialIndex } from "./SpatialIndex";
import type { Primitive } from "./primitives";
import type { Aabb } from "./Viewport";
import type { VectorDocument } from "../document/VectorDocument";
import { drawablesToPrimitives } from "../document/toPrimitives";

export class InfiniDocument {
  private items: Primitive[] = [];
  private index = new SpatialIndex();
  private version = 0;

  get size(): number {
    return this.items.length;
  }

  get docVersion(): number {
    return this.version;
  }

  get indexMode(): "flat" | "quadtree" {
    return this.index.mode;
  }

  clear(): void {
    this.items = [];
    this.index.rebuild(this.items);
    this.version++;
  }

  setPrimitives(items: Primitive[]): void {
    this.items = items.slice();
    this.index.rebuild(this.items);
    this.version++;
  }

  /**
   * Project vector tree → spatial index for cull/paint.
   * @implements [SRS-IN-04] WorldLayer from flattenDrawables
   */
  syncFromVectorDoc(tree: VectorDocument): void {
    this.setPrimitives(drawablesToPrimitives(tree.flatten()));
  }

  add(p: Primitive): void {
    this.items.push(p);
    this.index.rebuild(this.items);
    this.version++;
  }

  all(): readonly Primitive[] {
    return this.items;
  }

  queryVisible(worldView: Aabb): Primitive[] {
    return this.index.query(worldView);
  }
}
