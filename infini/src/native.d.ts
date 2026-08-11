export type RmStrokeMsg =
  | { type: "stroke_begin"; id: string; brush?: { width?: number }; cw?: number; ch?: number }
  | { type: "stroke_point"; id: string; x: number; y: number; p?: number }
  | { type: "stroke_end"; id: string };

export interface InfiniNative {
  onRmStroke: (cb: (msg: RmStrokeMsg) => void) => () => void;
  strokeIngestPort: () => Promise<number>;
  sendToRm?: (obj: unknown) => Promise<number>;
  rmClientCount?: () => Promise<number>;
  onRmClient?: (cb: (ev: { type: "connected" | "closed"; n: number }) => void) => () => void;
}

declare global {
  interface Window {
    infiniNative?: InfiniNative;
  }
}

export {};
