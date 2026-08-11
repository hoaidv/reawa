export type RmStrokeMsg =
  | { type: "stroke_begin"; id: string; brush?: { width?: number }; cw?: number; ch?: number }
  | { type: "stroke_point"; id: string; x: number; y: number; p?: number }
  | { type: "stroke_end"; id: string };

export interface InfiniNative {
  onRmStroke: (cb: (msg: RmStrokeMsg) => void) => () => void;
  strokeIngestPort: () => Promise<number>;
}

declare global {
  interface Window {
    infiniNative?: InfiniNative;
  }
}

export {};
