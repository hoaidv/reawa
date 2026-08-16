/**
 * Vector document tree types — session SoT for Infini.
 * @implements [SRS-IN-04] tree node kinds and anchors
 */

export type Id = string;

export interface Vec2 {
  x: number;
  y: number;
}

export interface Aabb {
  minX: number;
  minY: number;
  maxX: number;
  maxY: number;
}

export interface Style {
  stroke: string;
  strokeWidth: number;
  fill?: string;
}

export interface InkSample {
  x: number;
  y: number;
  pressure?: number;
  tiltX?: number;
  tiltY?: number;
  t?: number;
  timestamp?: number;
  distance?: number;
  extras?: Record<string, number | boolean | string>;
}

export type RectPort = "north" | "east" | "south" | "west";
export type EllipsePort = "top" | "right" | "bottom" | "left";
export type LinePort = "start" | "end" | "mid";
export type PortId = RectPort | EllipsePort | LinePort;

export type BoundaryParam =
  | { edge: RectPort; t: number }
  | { angle: number }
  | { t: number };

/** Device create_connector from/to. Legacy port | boundary still resolve for old trees. */
export interface Anchor {
  nodeId: Id;
  /** Preferred discrete ports (legacy Infini SVG / primitives). */
  port?: PortId;
  boundary?: BoundaryParam;
  /** Device envelope: edge | centre (ADR-0020). */
  kind?: "edge" | "centre";
  edge?: number;
  t?: number;
  drawnN?: number;
  drawnE?: number;
  drawnBoxX?: number;
  drawnBoxY?: number;
  drawnEdgeLocal?: { n: number; e: number };
  drawnBoxLocal?: { x: number; y: number };
  local?: Vec2;
  hasLocal?: boolean;
}

export interface RestOffset {
  s: number;
  d: number;
}

export interface RestShape {
  spine: Vec2[];
  offsets: RestOffset[];
  warpStyle?: string;
}

/** @implements [SRS-IN-09] last live world pose cache (D39) */
export interface ConnectorEndPose {
  x: number;
  y: number;
  fx: number;
  fy: number;
  valid?: boolean;
}

export interface InkNode {
  id: Id;
  kind: "ink";
  samples: InkSample[];
  style: Style;
  role?: "content" | "boundary";
  /** Content-only UV in SmartGroup.bounds (SRS-IN-09). */
  layoutOffset?: { u: number; v: number };
}

export interface TextRun {
  text: string;
  bold?: boolean;
}

export interface TextNode {
  id: Id;
  kind: "text";
  box: Aabb;
  runs: TextRun[];
  style: Style;
}

export interface LineGeom {
  kind: "line";
  x1: number;
  y1: number;
  x2: number;
  y2: number;
}

export interface RectGeom {
  kind: "rect";
  x: number;
  y: number;
  w: number;
  h: number;
}

export interface EllipseGeom {
  kind: "ellipse";
  cx: number;
  cy: number;
  rx: number;
  ry: number;
}

export type PrimitiveGeom = LineGeom | RectGeom | EllipseGeom;

export interface PrimitiveNode {
  id: Id;
  kind: "primitive";
  geom: PrimitiveGeom;
  style: Style;
}

export interface GroupNode {
  id: Id;
  kind: "group";
  children: DocNode[];
}

export interface SmartBounds {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface SmartTransform {
  x: number;
  y: number;
  rotation: number;
  scaleX: number;
  scaleY: number;
}

export interface SmartGroupNode {
  id: Id;
  kind: "smart_group";
  bounds: SmartBounds;
  transform: SmartTransform;
  inkScaleMode: "withBounds" | "fixedInk";
  children: InkNode[];
}

export interface FrameNode {
  id: Id;
  kind: "frame";
  bounds: Aabb;
  children: DocNode[];
}

/** @implements [SRS-IN-09] create_connector envelope; geometry is derived */
export interface ConnectorNode {
  id: Id;
  kind: "connector";
  from: Anchor;
  to: Anchor;
  warpStyle?: string;
  restSpine?: Vec2[];
  restOffsets?: RestOffset[];
  /** Body ink (captureIds / body); not a spatial parent. */
  children?: InkNode[];
  fromPose?: ConnectorEndPose;
  toPose?: ConnectorEndPose;
  /** Derived warped samples — never streamed on the wire. */
  warpedSamples?: Vec2[];
  /** Paint cache (legacy straight path or warped samples). */
  path?: Vec2[];
  /** D39: always false on create; missing node uses last live pose. */
  invalid?: boolean;
}

export type DocNode =
  | InkNode
  | TextNode
  | PrimitiveNode
  | GroupNode
  | SmartGroupNode
  | FrameNode
  | ConnectorNode;

export type DocStatus = "none" | "open" | "dirty" | "error";

export interface VectorDocSnapshot {
  version: 1;
  status: DocStatus;
  title?: string;
  path?: string;
  errorMessage?: string;
  rootChildren: DocNode[];
}

export type OpSource = "epaper" | "infini";

export interface DocOp {
  opId: string;
  type: string;
  payload: Record<string, unknown>;
  ts?: number;
  source?: OpSource;
}

export interface DrawableBase {
  id: Id;
  bounds: Aabb;
}

export interface InkDrawable extends DrawableBase {
  kind: "ink";
  samples: InkSample[];
  style: Style;
}

export interface TextDrawable extends DrawableBase {
  kind: "text";
  box: Aabb;
  runs: TextRun[];
  style: Style;
}

export interface PrimitiveDrawable extends DrawableBase {
  kind: "primitive";
  geom: PrimitiveGeom;
  style: Style;
}

export interface ConnectorDrawable extends DrawableBase {
  kind: "connector";
  from: Vec2;
  to: Vec2;
  path: Vec2[];
  invalid?: boolean;
}

export type Drawable =
  | InkDrawable
  | TextDrawable
  | PrimitiveDrawable
  | ConnectorDrawable;

export const IDENTITY_SMART_TRANSFORM: SmartTransform = {
  x: 0,
  y: 0,
  rotation: 0,
  scaleX: 1,
  scaleY: 1,
};

export const DEFAULT_STYLE: Style = {
  stroke: "#1C2430",
  strokeWidth: 2,
};
