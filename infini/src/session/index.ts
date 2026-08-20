/**
 * @implements [SRS-IN-07] Infini tablet session API
 */

export type {
  ViewportMessage,
  ViewportFollowMessage,
  DocOpMessage,
  DocChangeMessage,
  DocChangeOp,
  DocLoadMessage,
  DrainAckMessage,
  HelloMessage,
  QueueEmptyMessage,
  LoadAckMessage,
  SessionOutbound,
  SessionTransport,
} from "./types";
export {
  FOLLOW_COPY,
  FOLLOW_P95_MS,
  aabbToXywh,
  cloneViewport,
  dualFollowOn,
  followToggleView,
  identityWorldLayer,
  parseFollowDirection,
  shouldApplyInboundTabletViewport,
  shouldPublishViewportDown,
  worldLayerFromLocalViewport,
  worldLayerFromTabletViewport,
  xywhToAabb,
} from "./viewportFollow";
export type {
  DrawingRegionXywh,
  FollowDirection,
  FollowOffKind,
  FollowToggleView,
  FollowUiId,
  WorldLayerPose,
} from "./viewportFollow";
export {
  docOpToMessage,
  docChangeToOp,
  messageToDocOp,
  isInfiniStructureOp,
} from "./types";
export { MemoryTransport } from "./MemoryTransport";
export { IpcRmTransport } from "./IpcRmTransport";
export { TabletSession } from "./TabletSession";
export type { TabletSessionOptions, HandshakePhase } from "./TabletSession";
