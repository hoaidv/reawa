/**
 * @implements [SRS-IN-04] vector document public API
 */

export * from "./types";
export { VectorDocument } from "./VectorDocument";
export { flattenDrawables, drawableBoundsUnion } from "./flatten";
export { resolveAnchor, smartGroupWorldAabb, smartLocalToWorld, inkSamplesCentroid, seedLayoutOffset } from "./anchors";
export {
  warpConnector,
  restShapeReconstruction,
  refreshConnectorWarp,
  connectorWirePayload,
} from "./connectorWarp";
export {
  pickSmartGroupAt,
  pickingAllowed,
  handleSelectionPointer,
  createSelectionSession,
  selectionOverlayScreenRect,
  smartGroupWithPreview,
  hitResizeHandle,
  HANDLE_TOLERANCE_CSS_PX,
  resizeWorldAabbFromHandle,
  smartTransformFromWorldAabb,
} from "./selection";
export type {
  InfiniTool,
  SelectionSession,
  ResizeHandle,
  PointerResult,
} from "./selection";
export { drawablesToPrimitives } from "./toPrimitives";
export { commitLiveStrokeToTree } from "./ingestLiveStroke";
export type { LiveStrokeCommit } from "./ingestLiveStroke";
export {
  commitStrokeWithEncloseRecognition,
  fractionSamplesInside,
  MIN_ENCLOSE_WORLD,
} from "./recognizeEnclose";
export type { EncloseStrokeInput, EncloseResult, StrokeIntent } from "./recognizeEnclose";
export { tryDrawIntoMembership, smartGroupsInPaintOrder } from "./membership";
export type { MembershipResult } from "./membership";
export {
  createSmartGroupFromSelection,
  qualifiesAsSurround,
  pointInPolygonEvenOdd,
  closedPathForTest,
} from "./surroundCreate";
export {
  buildPickables,
  applyToolIntent,
  normalizeStrokeIntent,
  messageCarriesToolMode,
} from "./toolIntent";
export type { Pickable, ToolIntentMessage, ToolIntentAction } from "./toolIntent";
export { UndoRing, UNDO_RING_DEPTH, isStructuralOp } from "./UndoRing";
export {
  serializeInfiniSvg,
  parseInfiniSvg,
  loadInfiniSvgReplacing,
} from "./svgProfile";
export type { SvgLoadResult, SvgLoadOk, SvgLoadErr } from "./svgProfile";
