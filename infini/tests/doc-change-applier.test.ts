/**
 * STORY-IN-027 / @SRS-IN-07 — inbound doc_change applier into VectorDocument.
 */
import { describe, expect, it } from "vitest";
import { InfiniDocument } from "../src/canvas/Document";
import { VectorDocument, serializeInfiniSvg } from "../src/document";
import {
  MemoryTransport,
  TabletSession,
  type DocChangeMessage,
} from "../src/session";

const style = { stroke: "#1C2430", strokeWidth: 2 };

function liveSession() {
  const tree = new VectorDocument();
  const world = new InfiniDocument();
  const transport = new MemoryTransport();
  const logs: Array<{ msg: string; detail?: unknown }> = [];
  const session = new TabletSession({
    tree,
    world,
    transport,
    cssWidth: 800,
    cssHeight: 600,
    log: (msg, detail) => logs.push({ msg, detail }),
  });
  session.connect();
  return { tree, world, transport, session, logs };
}

function change(
  seq: number,
  opId: string,
  type: string,
  payload: Record<string, unknown>,
  baseSeq = seq - 1,
): DocChangeMessage {
  return {
    type: "doc_change",
    seq,
    opId,
    baseSeq,
    op: { type, payload, opId, source: "epaper" },
  };
}

describe("SRS-IN-07 append_ink from Epaper updates tree and WorldLayer", () => {
  it("applies into VectorDocument and paints the mirror node", () => {
    const { tree, world, session, transport } = liveSession();
    const r = session.receiveDocChange(
      change(1, "ink_1", "append_ink", {
        id: "ink_1",
        samples: [
          { x: 10, y: 20 },
          { x: 12, y: 24 },
        ],
        style,
      }),
    );
    expect(r.applied).toBe(true);
    expect(tree.indexById().has("ink_1")).toBe(true);
    expect(world.all().some((p) => p.id === "ink_1")).toBe(true);
    expect(world.all().some((p) => p.id.startsWith("rm-live-"))).toBe(false);
    expect(transport.docOps).toHaveLength(0);
  });

  it("is idempotent on duplicate opId", () => {
    const { tree, session } = liveSession();
    const msg = change(1, "ink_1", "append_ink", {
      id: "ink_1",
      samples: [
        { x: 10, y: 20 },
        { x: 12, y: 24 },
      ],
      style,
    });
    expect(session.receiveDocChange(msg).applied).toBe(true);
    const snap = tree.snapshotString();
    const second = session.receiveDocChange(msg);
    expect(second.applied).toBe(false);
    expect(second.reason).toBe("duplicate_opId");
    expect(tree.snapshotString()).toBe(snap);
  });
});

describe("SRS-IN-07 transmit ops land in the mirror", () => {
  it("applies create_smart_group, set_smart_group_transform, set_ink_scale_mode", () => {
    const { tree, world, session } = liveSession();
    expect(
      session.receiveDocChange(
        change(1, "sg_1", "create_smart_group", {
          id: "sg_1",
          bounds: { x: 0, y: 0, width: 40, height: 20 },
          transform: { x: 10, y: 20, rotation: 0, scaleX: 1, scaleY: 1 },
          inkScaleMode: "withBounds",
          children: [
            {
              id: "ink_b",
              kind: "ink",
              role: "boundary",
              samples: [
                { x: 0, y: 0 },
                { x: 40, y: 0 },
              ],
              style,
            },
          ],
        }),
      ).applied,
    ).toBe(true);

    expect(
      session.receiveDocChange(
        change(2, "xf_1", "set_smart_group_transform", {
          id: "sg_1",
          transform: { x: 50, y: 60, rotation: 0, scaleX: 1, scaleY: 1 },
        }),
      ).applied,
    ).toBe(true);

    expect(
      session.receiveDocChange(
        change(3, "mode_1", "set_ink_scale_mode", {
          id: "sg_1",
          inkScaleMode: "fixedInk",
        }),
      ).applied,
    ).toBe(true);

    const sg = tree.indexById().get("sg_1");
    expect(sg?.kind).toBe("smart_group");
    if (sg?.kind === "smart_group") {
      expect(sg.transform.x).toBe(50);
      expect(sg.inkScaleMode).toBe("fixedInk");
    }
    expect(world.all().some((p) => p.id === "ink_b")).toBe(true);
  });

  it("applies reparent, remove, restore_snapshot", () => {
    const { tree, session } = liveSession();
    session.receiveDocChange(
      change(1, "g1", "create_group", { id: "grp_1" }),
    );
    session.receiveDocChange(
      change(2, "ink_a", "append_ink", {
        id: "ink_a",
        samples: [
          { x: 1, y: 1 },
          { x: 2, y: 2 },
        ],
        style,
      }),
    );
    expect(
      session.receiveDocChange(
        change(3, "rp", "reparent", { id: "ink_a", newParentId: "grp_1", index: 0 }),
      ).applied,
    ).toBe(true);
    const grp = tree.indexById().get("grp_1");
    expect(grp?.kind).toBe("group");
    if (grp?.kind === "group") {
      expect(grp.children.some((c) => c.id === "ink_a")).toBe(true);
    }

    expect(
      session.receiveDocChange(change(4, "rm", "remove", { id: "ink_a" })).applied,
    ).toBe(true);
    expect(tree.indexById().has("ink_a")).toBe(false);

    expect(
      session.receiveDocChange(
        change(5, "rst", "restore_snapshot", {
          document: {
            version: 1,
            rootChildren: [
              {
                id: "ink_restored",
                kind: "ink",
                samples: [
                  { x: 0, y: 0 },
                  { x: 3, y: 3 },
                ],
                style,
              },
            ],
          },
        }),
      ).applied,
    ).toBe(true);
    expect(tree.indexById().has("ink_restored")).toBe(true);
    expect(tree.indexById().has("grp_1")).toBe(false);
  });
});

describe("SRS-IN-07 sequence gap marks the mirror suspect", () => {
  it("does not apply, requests resync, and refuses save", () => {
    const { tree, session } = liveSession();
    session.lastAppliedSeq = 12;
    const r = session.receiveDocChange(
      change(
        16,
        "late",
        "append_ink",
        {
          id: "gap_ink",
          samples: [
            { x: 0, y: 0 },
            { x: 1, y: 1 },
          ],
          style,
        },
        15,
      ),
    );
    expect(r.applied).toBe(false);
    expect(r.reason).toBe("seq_gap");
    expect(r.resyncRequested).toBe(true);
    expect(session.mirrorSuspect).toBe(true);
    expect(session.resyncRequested).toBe(true);
    expect(tree.indexById().has("gap_ink")).toBe(false);
    expect(tree.canSave()).toBe(false);
    expect(session.trySerializeMirror(serializeInfiniSvg)).toBeNull();
    expect(() => serializeInfiniSvg(tree)).toThrow(/refuse_save_suspect_mirror/);
  });
});

describe("SRS-IN-07 applied change replaces the preview", () => {
  it("drops the preview path; the mirror node paints in its place", () => {
    const { world, session } = liveSession();
    session.previewBegin("s_9", 2.5);
    session.previewPoint("s_9", 10, 20);
    session.previewPoint("s_9", 12, 24);
    session.previewEnd("s_9");
    expect(session.previewIds()).toContain("s_9");
    expect(world.all().some((p) => p.id === "preview:s_9")).toBe(true);

    const r = session.receiveDocChange(
      change(1, "s_9", "append_ink", {
        id: "s_9",
        samples: [
          { x: 10, y: 20 },
          { x: 12, y: 24 },
        ],
        style,
      }),
    );
    expect(r.applied).toBe(true);
    expect(session.previewIds()).not.toContain("s_9");
    expect(world.all().some((p) => p.id === "preview:s_9")).toBe(false);
    expect(world.all().some((p) => p.id === "s_9")).toBe(true);
  });
});

describe("SRS-IN-07 Infini authors no document ops", () => {
  it("emits zero document ops for a viewer session", () => {
    const { session, transport } = liveSession();
    session.receiveDocChange(
      change(1, "ink_1", "append_ink", {
        id: "ink_1",
        samples: [
          { x: 0, y: 0 },
          { x: 1, y: 1 },
        ],
        style,
      }),
    );
    const emit = session.emitLocalStructureOp({
      opId: "local",
      type: "create_group",
      payload: { id: "grp_x" },
    });
    expect(emit.emitted).toBe(false);
    expect(emit.reason).toBe("viewer_only");
    expect(transport.docOps).toHaveLength(0);
    session.publishViewport({ translate: { x: 0, y: 0 }, scale: 1 });
    expect(transport.viewports.length).toBeGreaterThan(0);
    expect(transport.docOps).toHaveLength(0);
  });
});

describe("SRS-IN-08 device commit to mirror apply budget", () => {
  it("applies well under p95 300 ms", () => {
    const { session } = liveSession();
    const elapsed: number[] = [];
    for (let i = 0; i < 40; i++) {
      const r = session.receiveDocChange(
        change(i + 1, `ink_${i}`, "append_ink", {
          id: `ink_${i}`,
          samples: [
            { x: i, y: i },
            { x: i + 1, y: i + 1 },
          ],
          style,
        }),
      );
      expect(r.applied).toBe(true);
      elapsed.push(r.elapsedMs ?? Infinity);
    }
    elapsed.sort((a, b) => a - b);
    const p95 = elapsed[Math.floor(elapsed.length * 0.95)];
    expect(p95).toBeLessThanOrEqual(300);
  });
});

describe("SRS-IN-07 unknown op marks suspect", () => {
  it("logs, does not crash, and refuses save", () => {
    const { tree, session, logs } = liveSession();
    expect(() =>
      session.receiveDocChange(change(1, "weird", "teleport_ink", {})),
    ).not.toThrow();
    expect(session.mirrorSuspect).toBe(true);
    expect(session.resyncRequested).toBe(true);
    expect(logs.some((l) => l.msg.includes("unknown"))).toBe(true);
    expect(tree.canSave()).toBe(false);
  });
});
