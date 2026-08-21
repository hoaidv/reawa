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
  | { type: "stroke_point"; id: string; x: number; y: number; p?: number; space?: "world" }
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

/** @implements [SRS-IN-07] hello lastSeq queued */
export type RmHelloMsg = {
  type: "hello";
  lastSeq: number;
  queued: number;
  document?: Record<string, unknown>;
};
export type RmQueueEmptyMsg = { type: "queue_empty" };
export type RmLoadAckMsg = { type: "load_ack" };

export type RmManipPreviewMsg = {
  type: "manip_preview";
  id: string;
  transform: { x: number; y: number; rotation: number; scaleX: number; scaleY: number };
  bounds?: { x: number; y: number; width: number; height: number };
};

export type RmViewportMsg = {
  type: "viewport";
  translate: { x: number; y: number };
  scale: number;
  drawingRegion?: { minX: number; minY: number; maxX: number; maxY: number };
  seq: number;
  orientation?: string;
  settle?: boolean;
  source?: "infini" | "epaper";
};

/** @implements [SRS-IN-26] inbound viewport_follow */
export type RmViewportFollowMsg = {
  type: "viewport_follow";
  direction: "none" | "infini_to_epaper" | "epaper_to_infini";
  seq: number;
};

export type RmInboundMsg =
  | RmStrokeMsg
  | RmToolIntentMsg
  | RmDocChangeMsg
  | RmHelloMsg
  | RmQueueEmptyMsg
  | RmLoadAckMsg
  | RmManipPreviewMsg
  | RmViewportMsg
  | RmViewportFollowMsg;

/** @implements [SRS-IN-17] inbound debug_log from Epaper on :9878 */
export type DebugLogRecord = {
  type: "debug_log";
  ts: number;
  level: string;
  logger: string;
  msg: string;
  dropped: number;
};

export interface InfiniNative {
  onRmStroke: (cb: (msg: RmInboundMsg) => void) => () => void;
  onRmToolIntent?: (cb: (msg: RmToolIntentMsg) => void) => () => void;
  strokeIngestPort: () => Promise<number>;
  sendToRm?: (obj: unknown) => Promise<number>;
  rmClientCount?: () => Promise<number>;
  onRmClient?: (cb: (ev: { type: "connected" | "closed" | "sync"; n: number }) => void) => () => void;
  onDebugLog?: (cb: (msg: DebugLogRecord) => void) => () => void;
  onDebugClient?: (cb: (ev: { type: "connected" | "closed" | "sync"; n: number }) => void) => () => void;
  debugPort?: () => Promise<number>;
  debugClientCount?: () => Promise<number>;
  debugLogSnapshot?: () => Promise<DebugLogRecord[]>;
  clearDebugLog?: () => Promise<boolean>;
  sendDebugControl?: (type: "debug_request" | "debug_start" | "debug_stop") => Promise<number>;
  setDebugPanelOpen?: (open: boolean) => Promise<boolean>;
}

declare global {
  interface Window {
    infiniNative?: InfiniNative;
  }
}

export {};
