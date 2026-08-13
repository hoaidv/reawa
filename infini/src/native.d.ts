export type RmStrokeMsg =
  | {
      type: "stroke_begin";
      id: string;
      brush?: { width?: number };
      cw?: number;
      ch?: number;
      /** ADR-0013 / SRS-IN-13 — enclose when Ink-box armed. */
      intent?: "ink" | "enclose";
    }
  | { type: "stroke_point"; id: string; x: number; y: number; p?: number }
  | { type: "stroke_end"; id: string };

export type RmToolIntentMsg = {
  type: "tool_intent";
  action: "select" | "move" | "resize";
  nodeId: string;
  delta?: { dx: number; dy: number };
  bounds?: { x: number; y: number; width: number; height: number };
  seq?: number;
};

/** @implements [SRS-IN-07] inbound doc_change from Epaper */
export type RmDocChangeMsg = {
  type: "doc_change";
  seq: number;
  opId: string;
  op: {
    type: string;
    payload?: Record<string, unknown>;
    opId?: string;
    ts?: number;
    source?: "epaper" | "infini";
  };
  baseSeq: number;
};

export type RmInboundMsg = RmStrokeMsg | RmToolIntentMsg | RmDocChangeMsg;

export interface InfiniNative {
  onRmStroke: (cb: (msg: RmInboundMsg) => void) => () => void;
  onRmToolIntent?: (cb: (msg: RmToolIntentMsg) => void) => () => void;
  strokeIngestPort: () => Promise<number>;
  sendToRm?: (obj: unknown) => Promise<number>;
  rmClientCount?: () => Promise<number>;
  onRmClient?: (cb: (ev: { type: "connected" | "closed" | "sync"; n: number }) => void) => () => void;
}

declare global {
  interface Window {
    infiniNative?: InfiniNative;
  }
}

export {};
