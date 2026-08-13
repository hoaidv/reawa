/**
 * Materialised vector document + idempotent op apply.
 * @implements [SRS-IN-04] tree model and op-log apply
 */

import { resolveAnchor, seedLayoutOffset } from "./anchors";
import { flattenDrawables } from "./flatten";
import type {
  Anchor,
  ConnectorNode,
  DocNode,
  DocOp,
  DocStatus,
  Drawable,
  FrameNode,
  GroupNode,
  Id,
  InkNode,
  InkSample,
  PrimitiveGeom,
  PrimitiveNode,
  SmartBounds,
  SmartGroupNode,
  SmartTransform,
  Style,
  TextNode,
  TextRun,
  VectorDocSnapshot,
} from "./types";
import { DEFAULT_STYLE, IDENTITY_SMART_TRANSFORM } from "./types";

function cloneJson<T>(v: T): T {
  return JSON.parse(JSON.stringify(v)) as T;
}

function isContainer(n: DocNode): n is FrameNode | GroupNode {
  return n.kind === "frame" || n.kind === "group";
}

export class VectorDocument {
  status: DocStatus = "open";
  title?: string;
  path?: string;
  errorMessage?: string;
  rootChildren: DocNode[] = [];
  /**
   * Gap / unknown-op flag. A suspect mirror must not be serialized.
   * @implements [SRS-IN-07] suspect mirror must not be saved
   */
  mirrorSuspect = false;

  private appliedOpIds = new Set<string>();

  /** Deep snapshot for idempotency / tests. */
  toJSON(): VectorDocSnapshot {
    return {
      version: 1,
      status: this.status,
      title: this.title,
      path: this.path,
      errorMessage: this.errorMessage,
      rootChildren: cloneJson(this.rootChildren),
    };
  }

  snapshotString(): string {
    return JSON.stringify(this.toJSON());
  }

  indexById(): Map<string, DocNode> {
    const map = new Map<string, DocNode>();
    const visit = (nodes: DocNode[]) => {
      for (const n of nodes) {
        map.set(n.id, n);
        if (n.kind === "frame" || n.kind === "group") visit(n.children);
        if (n.kind === "smart_group") {
          for (const c of n.children) map.set(c.id, c);
        }
      }
    };
    visit(this.rootChildren);
    return map;
  }

  allIds(): string[] {
    return [...this.indexById().keys()];
  }

  /**
   * Replace materialised tree (e.g. after SVG load). Clears applied opIds.
   * @implements [SRS-IN-09] load materialised tree
   */
  replaceTree(nodes: DocNode[]): void {
    this.rootChildren = nodes;
    this.appliedOpIds.clear();
    this.status = "open";
    this.errorMessage = undefined;
    this.mirrorSuspect = false;
    this.refreshConnectors();
  }

  /**
   * Persistence must refuse a gapped / unknown-op mirror.
   * @implements [SRS-IN-07] do not save a suspect mirror
   */
  canSave(): boolean {
    return !this.mirrorSuspect;
  }

  markSuspect(reason: string): void {
    this.mirrorSuspect = true;
    this.status = "error";
    this.errorMessage = reason;
  }

  hasAppliedOpId(opId: string): boolean {
    return this.appliedOpIds.has(opId);
  }

  /**
   * Apply a document op. Idempotent on opId.
   * @implements [SRS-IN-04] idempotent op apply by opId
   */
  applyOp(op: DocOp): { applied: boolean; reason?: string } {
    if (this.appliedOpIds.has(op.opId)) {
      return { applied: false, reason: "duplicate_opId" };
    }

    try {
      switch (op.type) {
        case "create_frame":
          this.opCreateFrame(op.payload);
          break;
        case "create_group":
          this.opCreateGroup(op.payload);
          break;
        case "create_text":
          this.opCreateText(op.payload);
          break;
        case "create_primitive":
          this.opCreatePrimitive(op.payload);
          break;
        case "append_ink":
          this.opAppendInk(op.payload);
          break;
        case "create_connector":
          this.opCreateConnector(op.payload);
          break;
        case "create_smart_group":
          this.opCreateSmartGroup(op.payload);
          break;
        case "join_smart_group":
          this.opJoinSmartGroup(op.payload);
          break;
        case "set_smart_transform":
        case "set_smart_group_transform":
          this.opSetSmartTransform(op.payload);
          break;
        case "set_ink_scale_mode":
          this.opSetInkScaleMode(op.payload);
          break;
        case "reparent":
          this.opReparent(op.payload);
          break;
        case "remove":
        case "remove_node":
          this.opRemove(op.payload);
          break;
        case "restore_snapshot":
          this.opRestoreSnapshot(op.payload);
          break;
        case "translate_node":
          this.opTranslateNode(op.payload);
          break;
        default:
          return { applied: false, reason: `unknown_type:${op.type}` };
      }
    } catch (e) {
      const msg = e instanceof Error ? e.message : String(e);
      return { applied: false, reason: msg };
    }

    this.appliedOpIds.add(op.opId);
    this.status = "dirty";
    this.refreshConnectors();
    return { applied: true };
  }

  flatten(): Drawable[] {
    return flattenDrawables(this);
  }

  private assertUniqueId(id: Id): void {
    if (this.indexById().has(id)) {
      throw new Error(`duplicate_id:${id}`);
    }
  }

  private insertRoot(node: DocNode): void {
    this.rootChildren.push(node);
  }

  private insertUnder(parentId: Id | undefined, node: DocNode): void {
    if (!parentId) {
      this.insertRoot(node);
      return;
    }
    const parent = this.indexById().get(parentId);
    if (!parent || !isContainer(parent)) {
      throw new Error(`bad_parent:${parentId}`);
    }
    if (node.kind === "frame") {
      throw new Error("frame_not_under_container");
    }
    parent.children.push(node);
  }

  private opCreateFrame(p: Record<string, unknown>): void {
    const id = String(p.id);
    this.assertUniqueId(id);
    const bounds = p.bounds as FrameNode["bounds"];
    const frame: FrameNode = {
      id,
      kind: "frame",
      bounds: cloneJson(bounds),
      children: [],
    };
    // Frames only at root
    this.insertRoot(frame);
  }

  private opCreateGroup(p: Record<string, unknown>): void {
    const id = String(p.id);
    this.assertUniqueId(id);
    const group: GroupNode = { id, kind: "group", children: [] };
    this.insertUnder(p.parentId as string | undefined, group);
  }

  private opCreateText(p: Record<string, unknown>): void {
    const id = String(p.id);
    this.assertUniqueId(id);
    const node: TextNode = {
      id,
      kind: "text",
      box: cloneJson(p.box as TextNode["box"]),
      runs: cloneJson((p.runs as TextRun[]) ?? [{ text: "" }]),
      style: cloneJson((p.style as Style) ?? DEFAULT_STYLE),
    };
    this.insertUnder(p.parentId as string | undefined, node);
  }

  private opCreatePrimitive(p: Record<string, unknown>): void {
    const id = String(p.id);
    this.assertUniqueId(id);
    const node: PrimitiveNode = {
      id,
      kind: "primitive",
      geom: cloneJson(p.geom as PrimitiveGeom),
      style: cloneJson((p.style as Style) ?? DEFAULT_STYLE),
    };
    this.insertUnder(p.parentId as string | undefined, node);
  }

  private opAppendInk(p: Record<string, unknown>): void {
    const id = String(p.id);
    this.assertUniqueId(id);
    const node: InkNode = {
      id,
      kind: "ink",
      samples: cloneJson(p.samples as InkSample[]),
      style: cloneJson((p.style as Style) ?? DEFAULT_STYLE),
      role: p.role as InkNode["role"],
    };
    this.insertUnder(p.parentId as string | undefined, node);
  }

  private opCreateConnector(p: Record<string, unknown>): void {
    const id = String(p.id);
    this.assertUniqueId(id);
    const from = cloneJson(p.from as Anchor);
    const to = cloneJson(p.to as Anchor);
    const byId = this.indexById();
    const invalid = !byId.has(from.nodeId) || !byId.has(to.nodeId);
    const node: ConnectorNode = {
      id,
      kind: "connector",
      from,
      to,
      invalid,
    };
    this.insertUnder(p.parentId as string | undefined, node);
  }

  private opCreateSmartGroup(p: Record<string, unknown>): void {
    const id = String(p.id);
    this.assertUniqueId(id);
    const captureIds = (p.captureIds as string[] | undefined) ?? [];
    for (const cid of captureIds) {
      const detached = this.detachInk(cid);
      if (!detached) throw new Error(`capture_missing:${cid}`);
    }
    const children = cloneJson((p.children as InkNode[]) ?? []);
    for (const c of children) {
      if (c.kind !== "ink") throw new Error("smart_group_ink_only");
      // Ids may be new (boundary) or previously detached (content).
      if (this.indexById().has(c.id)) {
        throw new Error(`duplicate_id:${c.id}`);
      }
    }
    const node: SmartGroupNode = {
      id,
      kind: "smart_group",
      bounds: cloneJson(p.bounds as SmartBounds),
      transform: cloneJson(
        (p.transform as SmartTransform) ?? IDENTITY_SMART_TRANSFORM,
      ),
      inkScaleMode: (p.inkScaleMode as SmartGroupNode["inkScaleMode"]) ?? "fixedInk",
      children,
    };
    this.insertUnder(p.parentId as string | undefined, node);
  }

  /**
   * Remove an ink node from the tree (root / frame / group only — not inside SmartGroup).
   * @implements [SRS-IN-10] reparent capture into Smart Group
   */
  detachInk(id: Id): InkNode | null {
    const removeFrom = (nodes: DocNode[]): InkNode | null => {
      for (let i = 0; i < nodes.length; i++) {
        const n = nodes[i];
        if (n.kind === "ink" && n.id === id) {
          nodes.splice(i, 1);
          return n;
        }
        if (n.kind === "frame" || n.kind === "group") {
          const found = removeFrom(n.children);
          if (found) return found;
        }
      }
      return null;
    };
    return removeFrom(this.rootChildren);
  }

  /**
   * Reparent free ink into an existing Smart Group as content.
   * @implements [SRS-IN-15] join membership
   */
  private opJoinSmartGroup(p: Record<string, unknown>): void {
    const inkId = String(p.inkId);
    const smartGroupId = String(p.smartGroupId);
    const sg = this.indexById().get(smartGroupId);
    if (!sg || sg.kind !== "smart_group") {
      throw new Error(`not_smart_group:${smartGroupId}`);
    }
    const detached = this.detachInk(inkId);
    if (!detached) throw new Error(`join_missing:${inkId}`);
    const content: InkNode = cloneJson(detached);
    content.role = "content";
    // World → group-local (match enclose / device join); do not expand bounds (SRS-IN-15 / EP-10)
    const t = sg.transform;
    const sx = t.scaleX !== 0 ? t.scaleX : 1;
    const sy = t.scaleY !== 0 ? t.scaleY : 1;
    content.samples = content.samples.map((s) => ({
      ...s,
      x: (s.x - t.x) / sx,
      y: (s.y - t.y) / sy,
    }));
    content.layoutOffset = seedLayoutOffset(content.samples, sg.bounds);
    sg.children.push(content);
  }

  private opSetSmartTransform(p: Record<string, unknown>): void {
    const id = String(p.id);
    const node = this.indexById().get(id);
    if (!node || node.kind !== "smart_group") throw new Error(`not_smart_group:${id}`);
    if (p.transform) {
      node.transform = cloneJson(p.transform as SmartTransform);
    }
    // Optional bounds update for resize gestures (SRS-IN-11 handles).
    if (p.bounds) {
      node.bounds = cloneJson(p.bounds as SmartBounds);
    }
  }

  private opSetInkScaleMode(p: Record<string, unknown>): void {
    const id = String(p.id);
    const node = this.indexById().get(id);
    if (!node || node.kind !== "smart_group") throw new Error(`not_smart_group:${id}`);
    const mode = p.inkScaleMode;
    if (mode !== "withBounds" && mode !== "fixedInk") {
      throw new Error(`bad_ink_scale_mode:${String(mode)}`);
    }
    node.inkScaleMode = mode;
  }

  private opTranslateNode(p: Record<string, unknown>): void {
    const id = String(p.id);
    const dx = Number(p.dx);
    const dy = Number(p.dy);
    const node = this.indexById().get(id);
    if (!node) throw new Error(`missing:${id}`);
    switch (node.kind) {
      case "primitive":
        if (node.geom.kind === "rect") {
          node.geom.x += dx;
          node.geom.y += dy;
        } else if (node.geom.kind === "ellipse") {
          node.geom.cx += dx;
          node.geom.cy += dy;
        } else if (node.geom.kind === "line") {
          node.geom.x1 += dx;
          node.geom.y1 += dy;
          node.geom.x2 += dx;
          node.geom.y2 += dy;
        }
        break;
      case "text":
        node.box.minX += dx;
        node.box.maxX += dx;
        node.box.minY += dy;
        node.box.maxY += dy;
        break;
      case "frame":
        node.bounds.minX += dx;
        node.bounds.maxX += dx;
        node.bounds.minY += dy;
        node.bounds.maxY += dy;
        break;
      case "smart_group":
        node.transform.x += dx;
        node.transform.y += dy;
        break;
      case "ink":
        for (const s of node.samples) {
          s.x += dx;
          s.y += dy;
        }
        break;
      default:
        throw new Error(`cannot_translate:${node.kind}`);
    }
  }

  /**
   * Move a node to a new parent (draw-into membership / tree edit).
   * @implements [SRS-IN-07] reparent transmit op
   */
  private opReparent(p: Record<string, unknown>): void {
    const id = String(p.id);
    const newParentId =
      p.newParentId == null || p.newParentId === ""
        ? undefined
        : String(p.newParentId);
    const index = typeof p.index === "number" ? p.index : undefined;
    if (newParentId === id) throw new Error(`reparent_cycle:${id}`);
    if (newParentId && this.isUnder(id, newParentId)) {
      throw new Error(`reparent_cycle:${id}->${newParentId}`);
    }
    const detached = this.detachNode(id);
    if (!detached) throw new Error(`reparent_missing:${id}`);
    this.insertAt(newParentId, detached, index);
    this.dissolveEmptySmartGroups();
  }

  /**
   * Delete a node. Empty SmartGroups dissolve (domain invariant).
   * @implements [SRS-IN-07] remove transmit op
   */
  private opRemove(p: Record<string, unknown>): void {
    const id = String(p.id);
    const detached = this.detachNode(id);
    if (!detached) throw new Error(`remove_missing:${id}`);
    this.dissolveEmptySmartGroups();
  }

  /**
   * Wholesale replace — how device undo publishes.
   * @implements [SRS-IN-07] restore_snapshot transmit op
   */
  private opRestoreSnapshot(p: Record<string, unknown>): void {
    const raw = p.document ?? p;
    let nodes: DocNode[] | undefined;
    if (Array.isArray(raw)) {
      nodes = raw as DocNode[];
    } else if (raw && typeof raw === "object" && Array.isArray((raw as VectorDocSnapshot).rootChildren)) {
      nodes = (raw as VectorDocSnapshot).rootChildren;
    } else if (Array.isArray(p.rootChildren)) {
      nodes = p.rootChildren as DocNode[];
    }
    if (!nodes) throw new Error("restore_snapshot_missing_document");
    this.rootChildren = cloneJson(nodes);
  }

  /**
   * Detach any node (root / frame / group / SmartGroup child).
   * @implements [SRS-IN-07] reparent/remove detach
   */
  detachNode(id: Id): DocNode | null {
    const removeFrom = (nodes: DocNode[]): DocNode | null => {
      for (let i = 0; i < nodes.length; i++) {
        const n = nodes[i];
        if (n.id === id) {
          nodes.splice(i, 1);
          return n;
        }
        if (n.kind === "frame" || n.kind === "group") {
          const found = removeFrom(n.children);
          if (found) return found;
        }
        if (n.kind === "smart_group") {
          const idx = n.children.findIndex((c) => c.id === id);
          if (idx >= 0) {
            const [ink] = n.children.splice(idx, 1);
            const free: InkNode = cloneJson(ink);
            delete free.role;
            delete free.layoutOffset;
            return free;
          }
        }
      }
      return null;
    };
    return removeFrom(this.rootChildren);
  }

  private isUnder(ancestorId: Id, maybeDescendantId: Id): boolean {
    const ancestor = this.indexById().get(ancestorId);
    if (!ancestor) return false;
    const walk = (nodes: DocNode[]): boolean => {
      for (const n of nodes) {
        if (n.id === maybeDescendantId) return true;
        if (n.kind === "frame" || n.kind === "group") {
          if (walk(n.children)) return true;
        }
        if (n.kind === "smart_group") {
          if (n.children.some((c) => c.id === maybeDescendantId)) return true;
        }
      }
      return false;
    };
    if (ancestor.kind === "frame" || ancestor.kind === "group") return walk(ancestor.children);
    if (ancestor.kind === "smart_group") {
      return ancestor.children.some((c) => c.id === maybeDescendantId);
    }
    return false;
  }

  private insertAt(parentId: Id | undefined, node: DocNode, index?: number): void {
    const spliceInto = (nodes: DocNode[]) => {
      if (index == null || index >= nodes.length) {
        nodes.push(node);
      } else {
        nodes.splice(Math.max(0, index), 0, node);
      }
    };
    if (!parentId) {
      spliceInto(this.rootChildren);
      return;
    }
    const parent = this.indexById().get(parentId);
    if (!parent) throw new Error(`bad_parent:${parentId}`);
    if (node.kind === "frame") throw new Error("frame_not_under_container");
    if (parent.kind === "smart_group") {
      if (node.kind !== "ink") throw new Error("smart_group_ink_only");
      const ink: InkNode = cloneJson(node);
      ink.role = "content";
      ink.layoutOffset = seedLayoutOffset(ink.samples, parent.bounds);
      if (index == null || index >= parent.children.length) {
        parent.children.push(ink);
      } else {
        parent.children.splice(Math.max(0, index), 0, ink);
      }
      return;
    }
    if (!isContainer(parent)) throw new Error(`bad_parent:${parentId}`);
    spliceInto(parent.children);
  }

  /** Domain: a SmartGroup with zero children is removed. */
  private dissolveEmptySmartGroups(): void {
    const prune = (nodes: DocNode[]): void => {
      for (let i = nodes.length - 1; i >= 0; i--) {
        const n = nodes[i];
        if (n.kind === "smart_group" && n.children.length === 0) {
          nodes.splice(i, 1);
          continue;
        }
        if (n.kind === "frame" || n.kind === "group") prune(n.children);
      }
    };
    prune(this.rootChildren);
  }

  /** Re-resolve connector validity + cached straight path after moves. */
  private refreshConnectors(): void {
    const byId = this.indexById();
    const visit = (nodes: DocNode[]) => {
      for (const n of nodes) {
        if (n.kind === "connector") {
          const from = resolveAnchor(n.from, byId);
          const to = resolveAnchor(n.to, byId);
          n.invalid = !from || !to;
          n.path = from && to ? [from, to] : undefined;
        } else if (n.kind === "frame" || n.kind === "group") {
          visit(n.children);
        }
      }
    };
    visit(this.rootChildren);
  }

  /** Frames only at root — for invariant checks. */
  framesOnlyAtRoot(): boolean {
    const nested: string[] = [];
    const walk = (nodes: DocNode[], underRoot: boolean) => {
      for (const n of nodes) {
        if (n.kind === "frame" && !underRoot) nested.push(n.id);
        if (n.kind === "frame" || n.kind === "group") {
          walk(n.children, false);
        }
      }
    };
    walk(this.rootChildren, true);
    return nested.length === 0;
  }

  groupsExcludeFrame(): boolean {
    const bad: string[] = [];
    const walk = (nodes: DocNode[]) => {
      for (const n of nodes) {
        if (n.kind === "group") {
          for (const c of n.children) {
            if (c.kind === "frame") bad.push(c.id);
          }
          walk(n.children);
        } else if (n.kind === "frame") {
          walk(n.children);
        }
      }
    };
    walk(this.rootChildren);
    return bad.length === 0;
  }
}
