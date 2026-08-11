/**
 * @implements [SRS-IN-01] Spatial index for viewport-scoped paint
 *
 * Criticism of ml-mindmap VectorRendererLayer: it paints *every* element every
 * frame (`for (const element of doc.getElements())`) with an explicit cull TODO.
 * At tens–hundreds of thousands of dense polylines that will not fly.
 *
 * Strategy:
 * - Small N: flat list scan (cheaper than tree overhead).
 * - Large N / dense ink at document root: quadtree over AABBs.
 * - Later grouping: recurse stacking tree; *per level* still needs the same
 *   N-sibling query — so the index stays relevant at every node.
 */

import type { Aabb } from "./Viewport";
import { aabbIntersects } from "./Viewport";
import type { Primitive } from "./primitives";

const FLAT_THRESHOLD = 256;
const MAX_DEPTH = 12;
const MAX_PER_NODE = 12;

interface QuadNode {
  bounds: Aabb;
  depth: number;
  items: Primitive[];
  children: QuadNode[] | null;
}

function makeNode(bounds: Aabb, depth: number): QuadNode {
  return { bounds, depth, items: [], children: null };
}

function split(node: QuadNode): void {
  const { minX, minY, maxX, maxY } = node.bounds;
  const mx = (minX + maxX) / 2;
  const my = (minY + maxY) / 2;
  node.children = [
    makeNode({ minX, minY, maxX: mx, maxY: my }, node.depth + 1),
    makeNode({ minX: mx, minY, maxX, maxY: my }, node.depth + 1),
    makeNode({ minX, minY: my, maxX: mx, maxY }, node.depth + 1),
    makeNode({ minX: mx, minY: my, maxX, maxY }, node.depth + 1),
  ];
  const kept = node.items;
  node.items = [];
  for (const item of kept) insertNode(node, item);
}

function insertNode(node: QuadNode, item: Primitive): void {
  if (node.children) {
    let placed = false;
    for (const child of node.children) {
      if (
        item.bounds.minX >= child.bounds.minX &&
        item.bounds.maxX <= child.bounds.maxX &&
        item.bounds.minY >= child.bounds.minY &&
        item.bounds.maxY <= child.bounds.maxY
      ) {
        insertNode(child, item);
        placed = true;
        break;
      }
    }
    if (!placed) node.items.push(item);
    return;
  }
  node.items.push(item);
  if (node.items.length > MAX_PER_NODE && node.depth < MAX_DEPTH) {
    split(node);
  }
}

function queryNode(node: QuadNode, q: Aabb, out: Primitive[]): void {
  if (!aabbIntersects(node.bounds, q)) return;
  for (const item of node.items) {
    if (aabbIntersects(item.bounds, q)) out.push(item);
  }
  if (node.children) {
    for (const child of node.children) queryNode(child, q, out);
  }
}

export class SpatialIndex {
  private flat: Primitive[] = [];
  private root: QuadNode | null = null;
  private world: Aabb = { minX: -1e6, minY: -1e6, maxX: 1e6, maxY: 1e6 };

  rebuild(items: Primitive[]): void {
    this.flat = items.slice();
    if (items.length <= FLAT_THRESHOLD) {
      this.root = null;
      return;
    }
    let minX = Infinity,
      minY = Infinity,
      maxX = -Infinity,
      maxY = -Infinity;
    for (const it of items) {
      minX = Math.min(minX, it.bounds.minX);
      minY = Math.min(minY, it.bounds.minY);
      maxX = Math.max(maxX, it.bounds.maxX);
      maxY = Math.max(maxY, it.bounds.maxY);
    }
    if (!Number.isFinite(minX)) {
      this.root = null;
      return;
    }
    // Pad so edge items can subdivide cleanly.
    const pad = Math.max(1, (maxX - minX + maxY - minY) * 0.05);
    this.world = {
      minX: minX - pad,
      minY: minY - pad,
      maxX: maxX + pad,
      maxY: maxY + pad,
    };
    this.root = makeNode(this.world, 0);
    for (const it of items) insertNode(this.root, it);
  }

  /** Candidates intersecting the visible world AABB. */
  query(viewportWorld: Aabb): Primitive[] {
    if (!this.root) {
      return this.flat.filter((it) => aabbIntersects(it.bounds, viewportWorld));
    }
    const out: Primitive[] = [];
    queryNode(this.root, viewportWorld, out);
    return out;
  }

  get size(): number {
    return this.flat.length;
  }

  get mode(): "flat" | "quadtree" {
    return this.root ? "quadtree" : "flat";
  }
}
