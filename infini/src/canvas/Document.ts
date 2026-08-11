/**
 * @implements [SRS-IN-01] In-memory document + spatial index
 */

import { SpatialIndex } from "./SpatialIndex";
import type { Primitive } from "./primitives";
import type { Aabb } from "./Viewport";

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
