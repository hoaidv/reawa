/**
 * STORY-IN-028 / @SRS-IN-07 @SRS-IN-08 — handshake-gated doc_load.
 */
import { describe, expect, it } from "vitest";
import { InfiniDocument } from "../src/canvas/Document";
import { identityViewport } from "../src/canvas/Viewport";
import { VectorDocument } from "../src/document";
import {
  MemoryTransport,
  TabletSession,
  type DocChangeMessage,
  type SessionOutbound,
} from "../src/session";

const style = { stroke: "#1C2430", strokeWidth: 2 };

function liveSession() {
  const tree = new VectorDocument();
  const world = new InfiniDocument();
  const transport = new MemoryTransport();
  const session = new TabletSession({
    tree,
    world,
    transport,
    cssWidth: 800,
    cssHeight: 600,
  });
  session.connect();
  return { tree, world, transport, session };
}

function change(
  seq: number,
  opId: string,
  payload: Record<string, unknown>,
  baseSeq = seq - 1,
): DocChangeMessage {
  return {
    type: "doc_change",
    seq,
    opId,
    baseSeq,
    op: { type: "append_ink", payload: { ...payload, style }, opId, source: "epaper" },
  };
}

function documentTypes(outbound: SessionOutbound[]): string[] {
  return outbound.filter((m) => m.type !== "viewport").map((m) => m.type);
}

describe("STORY-IN-028 drain then doc_load when queued is greater than zero", () => {
  it("sends drain_ack, applies doc_change in seq, then doc_load after queue_empty", () => {
    const { session, tree, transport } = liveSession();
    tree.applyOp({
      opId: "seed",
      type: "append_ink",
      payload: {
        id: "seed_ink",
        samples: [
          { x: 0, y: 0 },
          { x: 1, y: 1 },
        ],
        style,
      },
    });

    session.receiveHello({ type: "hello", lastSeq: 4, queued: 2 });
    expect(transport.drainAcks).toEqual([{ type: "drain_ack" }]);
    expect(transport.docLoads).toHaveLength(0);
    expect(session.handshakePhase).toBe("draining");

    const first = session.receiveDocChange(
      change(3, "ink_a", {
        id: "ink_a",
        samples: [
          { x: 10, y: 20 },
          { x: 12, y: 24 },
        ],
      }),
    );
    expect(first.applied).toBe(true);
    expect(transport.docLoads).toHaveLength(0);

    const second = session.receiveDocChange(
      change(4, "ink_b", {
        id: "ink_b",
        samples: [
          { x: 30, y: 40 },
          { x: 32, y: 44 },
        ],
      }),
    );
    expect(second.applied).toBe(true);
    expect(tree.indexById().has("ink_a")).toBe(true);
    expect(tree.indexById().has("ink_b")).toBe(true);
    expect(transport.docLoads).toHaveLength(0);

    session.receiveQueueEmpty({ type: "queue_empty" });
    expect(transport.docLoads).toHaveLength(1);
    const load = transport.docLoads[0];
    expect(load.type).toBe("doc_load");
    expect(load.seq).toBe(0);
    expect(load.document).toMatchObject({ version: 1, rootChildren: expect.any(Array) });
    expect("pickables" in load).toBe(false);

    const types = documentTypes(transport.outbound);
    expect(types[0]).toBe("drain_ack");
    expect(types.filter((t) => t === "doc_load")).toEqual(["doc_load"]);
    expect(types.indexOf("drain_ack")).toBeLessThan(types.indexOf("doc_load"));
  });
});

describe("STORY-IN-028 zero outbound document messages after the load", () => {
  it("after handshake, only viewport may flow down", () => {
    const { session, transport } = liveSession();
    session.receiveHello({ type: "hello", lastSeq: 0, queued: 0 });
    session.receiveLoadAck({ type: "load_ack" });
    expect(transport.docLoads).toHaveLength(1);

    session.publishViewport(identityViewport(), { force: true });
    session.flushViewport(identityViewport());
    session.setOrientation("gutOnTop");
    session.noteInfiniSideAction();
    session.emitLocalStructureOp({
      opId: "nope",
      type: "append_ink",
      payload: { id: "x", samples: [] },
    });

    const after = transport.outbound.slice(1);
    expect(after.every((m) => m.type === "viewport")).toBe(true);
    expect(after.filter((m) => m.type === "doc_load")).toHaveLength(0);
    expect(documentTypes(after)).toHaveLength(0);
  });
});

describe("STORY-IN-028 reconnect runs the handshake instead of a reflexive document push", () => {
  it("waits for hello and drains before one doc_load", () => {
    const { session, transport } = liveSession();
    session.receiveHello({ type: "hello", lastSeq: 0, queued: 0 });
    session.receiveLoadAck({ type: "load_ack" });
    expect(transport.docLoads).toHaveLength(1);

    session.disconnect();
    session.connect();
    expect(session.handshakePhase).toBe("awaiting_hello");
    expect(transport.docLoads).toHaveLength(1);

    session.receiveHello({ type: "hello", lastSeq: 4, queued: 2 });
    expect(transport.drainAcks).toHaveLength(1);
    expect(transport.docLoads).toHaveLength(1);

    session.receiveDocChange(
      change(3, "ink_a", {
        id: "ink_a",
        samples: [
          { x: 1, y: 1 },
          { x: 2, y: 2 },
        ],
      }),
    );
    session.receiveDocChange(
      change(4, "ink_b", {
        id: "ink_b",
        samples: [
          { x: 3, y: 3 },
          { x: 4, y: 4 },
        ],
      }),
    );
    expect(transport.docLoads).toHaveLength(1);
    session.receiveQueueEmpty({ type: "queue_empty" });
    expect(transport.docLoads).toHaveLength(2);
    expect(transport.docLoads[1]).toMatchObject({ type: "doc_load", seq: 0 });
  });
});

describe("STORY-IN-028 orientation and Infini-side actions send zero doc_load", () => {
  it("does not emit doc_load or doc_snapshot", () => {
    const { session, transport } = liveSession();
    session.receiveHello({ type: "hello", lastSeq: 0, queued: 0 });
    session.receiveLoadAck({ type: "load_ack" });
    const loads = transport.docLoads.length;

    session.setOrientation("gutToRight");
    session.noteInfiniSideAction();
    session.publishViewport({ translate: { x: 10, y: 10 }, scale: 1.2 }, { force: true });

    expect(transport.docLoads).toHaveLength(loads);
    expect(transport.outbound.some((m) => m.type === "doc_snapshot")).toBe(false);
  });
});

describe("STORY-IN-028 hello retry resends doc_load while awaiting load_ack", () => {
  it("does not emit a second drain, only another doc_load", () => {
    const { session, transport } = liveSession();
    session.receiveHello({ type: "hello", lastSeq: 0, queued: 0 });
    expect(transport.docLoads).toHaveLength(1);
    expect(session.handshakePhase).toBe("awaiting_load_ack");

    session.receiveHello({ type: "hello", lastSeq: 0, queued: 0 });
    expect(transport.docLoads).toHaveLength(2);
    expect(transport.drainAcks).toHaveLength(0);
    expect(session.handshakePhase).toBe("awaiting_load_ack");
  });
});

describe("STORY-IN-028 missed hello still applies a live doc_change stream", () => {
  it("adopts baseSeq and applies create_smart_group then set_smart_transform", () => {
    const { session, tree, world } = liveSession();
    expect(session.handshakePhase).toBe("awaiting_hello");

    const created = session.receiveDocChange({
      type: "doc_change",
      seq: 24,
      opId: "csg",
      baseSeq: 23,
      op: {
        type: "create_smart_group",
        opId: "csg",
        source: "epaper",
        payload: {
          id: "sg_1",
          bounds: { x: 0, y: 0, width: 40, height: 20 },
          transform: { x: 10, y: 20, rotation: 0, scaleX: 1, scaleY: 1 },
          inkScaleMode: "fixedInk",
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
        },
      },
    });
    expect(created.applied).toBe(true);
    expect(session.handshakePhase).toBe("live");
    expect(session.lastAppliedSeq).toBe(24);

    const moved = session.receiveDocChange({
      type: "doc_change",
      seq: 25,
      opId: "sst",
      baseSeq: 24,
      op: {
        type: "set_smart_transform",
        opId: "sst",
        source: "epaper",
        payload: {
          id: "sg_1",
          transform: { x: 80, y: 90, rotation: 0, scaleX: 1, scaleY: 1 },
        },
      },
    });
    expect(moved.applied).toBe(true);
    const sg = tree.indexById().get("sg_1");
    expect(sg?.kind).toBe("smart_group");
    if (sg?.kind === "smart_group") expect(sg.transform.x).toBe(80);
    expect(world.all().some((p) => p.id === "ink_b")).toBe(true);
  });
});

describe("STORY-IN-028 retired snapshot pickables and tool_intent are not emitted", () => {
  it("emits doc_load and never retired wire names", () => {
    const { session, transport } = liveSession();
    session.receiveHello({ type: "hello", lastSeq: 0, queued: 0 });

    const types = transport.outbound.map((m) => m.type);
    expect(types).toContain("doc_load");
    expect(types).not.toContain("doc_snapshot");
    expect(types).not.toContain("tool_intent");
    expect(types).not.toContain("doc_op");
    expect(transport.docLoads[0]).not.toHaveProperty("pickables");
    expect(JSON.stringify(transport.outbound)).not.toMatch(/"intent"/);
  });
});
