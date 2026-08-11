/**
 * Smart Group selection + hit-test + move/resize gestures.
 * @implements [SRS-IN-11] selection hit-testing and manipulation
 */

import { smartGroupWorldAabb } from "./anchors";
import type { Aabb, DocNode, SmartBounds, SmartGroupNode, SmartTransform, Vec2 } from "./types";
import type { VectorDocument } from "./VectorDocument";
import { TILE_LOD_SCALE } from "../canvas/TileCache";
import type { Viewport } from "../canvas/Viewport";
import { screenToWorld, worldToScreen } from "../canvas/Viewport";

export type InfiniTool = "selection" | "ink_box";

export type ResizeHandle =
  | "nw"
  | "n"
  | "ne"
  | "e"
  | "se"
  | "s"
  | "sw"
  | "w";

export const HANDLE_TOLERANCE_CSS_PX = 8;

function pointInAabb(p: Vec2, box: Aabb, pad = 0): boolean {
  return (
    p.x >= box.minX - pad &&
    p.x <= box.maxX + pad &&
    p.y >= box.minY - pad &&
    p.y <= box.maxY + pad
  );
}

function collectSmartGroups(nodes: readonly DocNode[], out: SmartGroupNode[]): void {
  for (const n of nodes) {
    if (n.kind === "smart_group") out.push(n);
    if (n.kind === "frame" || n.kind === "group") {
      collectSmartGroups(n.children, out);
    }
  }
}

/** Topmost (later sibling) SmartGroup whose world bounds contain the point.
 * @implements [SRS-IN-11] pickable set + resolution order
 */
export function pickSmartGroupAt(
  tree: VectorDocument,
  world: Vec2,
  opts?: { selectedId?: string | null; scale?: number },
): SmartGroupNode | null {
  const scale = opts?.scale ?? 1;
  if (scale < TILE_LOD_SCALE) return null;

  const groups: SmartGroupNode[] = [];
  collectSmartGroups(tree.rootChildren, groups);
  // Later siblings paint above → pick first from the end.
  for (let i = groups.length - 1; i >= 0; i--) {
    const sg = groups[i];
    const box = smartGroupWorldAabb(sg);
    const padWorld =
      opts?.selectedId === sg.id ? HANDLE_TOLERANCE_CSS_PX / Math.max(scale, 1e-6) : 0;
    if (pointInAabb(world, box, padWorld)) return sg;
  }
  return null;
}

/** Pick topmost free (non–SmartGroup) ink whose sample AABB contains the point. */
export function pickFreeInkAt(
  tree: VectorDocument,
  world: Vec2,
): { id: string } | null {
  const byId = tree.indexById();
  const free: string[] = [];
  const walk = (nodes: readonly DocNode[]) => {
    for (const n of nodes) {
      if (n.kind === "ink") free.push(n.id);
      if (n.kind === "frame" || n.kind === "group") walk(n.children);
    }
  };
  walk(tree.rootChildren);
  for (let i = free.length - 1; i >= 0; i--) {
    const ink = byId.get(free[i]);
    if (!ink || ink.kind !== "ink") continue;
    let minX = Infinity,
      minY = Infinity,
      maxX = -Infinity,
      maxY = -Infinity;
    for (const s of ink.samples) {
      minX = Math.min(minX, s.x);
      minY = Math.min(minY, s.y);
      maxX = Math.max(maxX, s.x);
      maxY = Math.max(maxY, s.y);
    }
    if (world.x >= minX && world.x <= maxX && world.y >= minY && world.y <= maxY) {
      return { id: ink.id };
    }
  }
  return null;
}

export function pickingAllowed(vp: Viewport): boolean {
  return vp.scale >= TILE_LOD_SCALE;
}

/** Which resize handle (if any) is under the screen point for a selected group. */
export function hitResizeHandle(
  sg: SmartGroupNode,
  screen: Vec2,
  vp: Viewport,
): ResizeHandle | null {
  const box = smartGroupWorldAabb(sg);
  const corners: { id: ResizeHandle; x: number; y: number }[] = [
    { id: "nw", x: box.minX, y: box.minY },
    { id: "ne", x: box.maxX, y: box.minY },
    { id: "se", x: box.maxX, y: box.maxY },
    { id: "sw", x: box.minX, y: box.maxY },
    { id: "n", x: (box.minX + box.maxX) / 2, y: box.minY },
    { id: "e", x: box.maxX, y: (box.minY + box.maxY) / 2 },
    { id: "s", x: (box.minX + box.maxX) / 2, y: box.maxY },
    { id: "w", x: box.minX, y: (box.minY + box.maxY) / 2 },
  ];
  const tol = HANDLE_TOLERANCE_CSS_PX;
  for (const c of corners) {
    const s = worldToScreen({ x: c.x, y: c.y }, vp);
    if (Math.abs(s.x - screen.x) <= tol && Math.abs(s.y - screen.y) <= tol) {
      return c.id;
    }
  }
  return null;
}

export interface SelectionSession {
  selectedId: string | null;
  /** Free ink multi-select for SRS-IN-16 create. */
  selectedInkIds: string[];
  tool: InfiniTool;
  /** Local drag preview (not yet committed). */
  preview: null | {
    kind: "move" | "resize";
    id: string;
    startWorld: Vec2;
    originTransform: SmartTransform;
    originBounds: SmartBounds;
    handle?: ResizeHandle;
    liveTransform: SmartTransform;
    liveBounds: SmartBounds;
    moved: boolean;
  };
}

export function createSelectionSession(): SelectionSession {
  return {
    selectedId: null,
    selectedInkIds: [],
    tool: "selection",
    preview: null,
  };
}

/**
 * Apply live preview onto a shallow copy of the SmartGroup for paint/overlay.
 */
export function smartGroupWithPreview(
  sg: SmartGroupNode,
  session: SelectionSession,
): SmartGroupNode {
  if (!session.preview || session.preview.id !== sg.id) return sg;
  return {
    ...sg,
    transform: { ...session.preview.liveTransform },
    bounds: { ...session.preview.liveBounds },
  };
}

function resizeBounds(
  origin: SmartBounds,
  handle: ResizeHandle,
  dxLocal: number,
  dyLocal: number,
): SmartBounds {
  let { x, y, width, height } = origin;
  const applyW = (d: number) => {
    width = Math.max(1, width + d);
  };
  const applyH = (d: number) => {
    height = Math.max(1, height + d);
  };
  switch (handle) {
    case "e":
      applyW(dxLocal);
      break;
    case "w":
      x += dxLocal;
      applyW(-dxLocal);
      break;
    case "s":
      applyH(dyLocal);
      break;
    case "n":
      y += dyLocal;
      applyH(-dyLocal);
      break;
    case "se":
      applyW(dxLocal);
      applyH(dyLocal);
      break;
    case "ne":
      applyW(dxLocal);
      y += dyLocal;
      applyH(-dyLocal);
      break;
    case "sw":
      x += dxLocal;
      applyW(-dxLocal);
      applyH(dyLocal);
      break;
    case "nw":
      x += dxLocal;
      applyW(-dxLocal);
      y += dyLocal;
      applyH(-dyLocal);
      break;
  }
  if (width < 1) {
    x += width - 1;
    width = 1;
  }
  if (height < 1) {
    y += height - 1;
    height = 1;
  }
  return { x, y, width, height };
}

export type PointerPhase = "down" | "move" | "up";

export interface PointerResult {
  session: SelectionSession;
  /** Commit op payload when gesture completes (release). */
  commit?: {
    type: "set_smart_transform";
    id: string;
    transform: SmartTransform;
    bounds?: SmartBounds;
  };
  /** True when this pointer event was consumed as selection/manipulate (no pan). */
  consumed: boolean;
  panDelta?: Vec2;
}

/**
 * Selection-tool pointer FSM.
 * @implements [SRS-IN-11] one op per completed gesture
 */
export function handleSelectionPointer(
  tree: VectorDocument,
  session: SelectionSession,
  phase: PointerPhase,
  screen: Vec2,
  vp: Viewport,
  lastScreen?: Vec2,
): PointerResult {
  const world = screenToWorld(screen, vp);
  const next: SelectionSession = {
    ...session,
    preview: session.preview ? { ...session.preview } : null,
  };

  if (!pickingAllowed(vp)) {
    if (phase === "down") {
      next.selectedId = null;
      next.preview = null;
    }
    let panDelta: Vec2 | undefined;
    if (phase === "move" && lastScreen) {
      panDelta = { x: screen.x - lastScreen.x, y: screen.y - lastScreen.y };
    }
    return { session: next, consumed: false, panDelta };
  }

  if (session.tool !== "selection") {
    return { session: next, consumed: false };
  }

  if (phase === "down") {
    const selected =
      next.selectedId != null
        ? (tree.indexById().get(next.selectedId) as SmartGroupNode | undefined)
        : undefined;
    if (selected?.kind === "smart_group") {
      const handle = hitResizeHandle(selected, screen, vp);
      if (handle) {
        next.preview = {
          kind: "resize",
          id: selected.id,
          startWorld: world,
          originTransform: { ...selected.transform },
          originBounds: { ...selected.bounds },
          handle,
          liveTransform: { ...selected.transform },
          liveBounds: { ...selected.bounds },
          moved: false,
        };
        return { session: next, consumed: true };
      }
    }

    const hit = pickSmartGroupAt(tree, world, {
      selectedId: next.selectedId,
      scale: vp.scale,
    });
    if (hit) {
      next.selectedId = hit.id;
      next.preview = {
        kind: "move",
        id: hit.id,
        startWorld: world,
        originTransform: { ...hit.transform },
        originBounds: { ...hit.bounds },
        liveTransform: { ...hit.transform },
        liveBounds: { ...hit.bounds },
        moved: false,
      };
      return { session: next, consumed: true };
    }

    next.selectedId = null;
    next.preview = null;
    return { session: next, consumed: false };
  }

  if (phase === "move" && next.preview) {
    const dx = world.x - next.preview.startWorld.x;
    const dy = world.y - next.preview.startWorld.y;
    if (next.preview.kind === "move") {
      next.preview.liveTransform = {
        ...next.preview.originTransform,
        x: next.preview.originTransform.x + dx,
        y: next.preview.originTransform.y + dy,
      };
      next.preview.moved = next.preview.moved || Math.hypot(dx, dy) > 0.5;
    } else if (next.preview.handle) {
      const node = tree.indexById().get(next.preview.id);
      const mode =
        node?.kind === "smart_group" ? node.inkScaleMode : "withBounds";
      const sx = next.preview.originTransform.scaleX || 1;
      const sy = next.preview.originTransform.scaleY || 1;
      if (mode === "fixedInk") {
        next.preview.liveBounds = resizeBounds(
          next.preview.originBounds,
          next.preview.handle,
          dx / sx,
          dy / sy,
        );
        next.preview.liveTransform = { ...next.preview.originTransform };
      } else {
        // withBounds: grow scale so world AABB follows the handle; local bounds stay.
        const ox = Math.max(1, next.preview.originBounds.width * sx);
        const oy = Math.max(1, next.preview.originBounds.height * sy);
        let newW = ox;
        let newH = oy;
        const h = next.preview.handle;
        if (h.includes("e") || h === "e") newW = Math.max(1, ox + dx);
        if (h.includes("w") || h === "w") newW = Math.max(1, ox - dx);
        if (h.includes("s") || h === "s") newH = Math.max(1, oy + dy);
        if (h.includes("n") || h === "n") newH = Math.max(1, oy - dy);
        next.preview.liveBounds = { ...next.preview.originBounds };
        next.preview.liveTransform = {
          ...next.preview.originTransform,
          scaleX: (newW / next.preview.originBounds.width) || sx,
          scaleY: (newH / next.preview.originBounds.height) || sy,
        };
      }
      next.preview.moved = true;
    }
    return { session: next, consumed: true };
  }

  if (phase === "up" && next.preview) {
    const prev = next.preview;
    next.preview = null;
    if (!prev.moved && prev.kind === "move") {
      // Click-select only — no op.
      return { session: next, consumed: true };
    }
    return {
      session: next,
      consumed: true,
      commit: {
        type: "set_smart_transform",
        id: prev.id,
        transform: prev.liveTransform,
        bounds: prev.kind === "resize" ? prev.liveBounds : undefined,
      },
    };
  }

  if (phase === "move" && lastScreen) {
    return {
      session: next,
      consumed: false,
      panDelta: { x: screen.x - lastScreen.x, y: screen.y - lastScreen.y },
    };
  }

  return { session: next, consumed: false };
}

/** Screen-space overlay rect for selected SmartGroup (CSS px relative to canvas). */
export function selectionOverlayScreenRect(
  sg: SmartGroupNode,
  vp: Viewport,
): { left: number; top: number; width: number; height: number } {
  const box = smartGroupWorldAabb(sg);
  const tl = worldToScreen({ x: box.minX, y: box.minY }, vp);
  const br = worldToScreen({ x: box.maxX, y: box.maxY }, vp);
  return {
    left: tl.x,
    top: tl.y,
    width: br.x - tl.x,
    height: br.y - tl.y,
  };
}
