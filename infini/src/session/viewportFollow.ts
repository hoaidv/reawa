/**
 * Viewport-follow session enum + FollowToggle view model.
 * @implements [SRS-IN-26] viewport-follow Epaper enum
 * @implements [SRS-IN-27] FollowToggle states and copy
 * @implements [SRS-IN-28] exclusive follow quality
 */

import type { Aabb, Viewport } from "../canvas/Viewport";

/** Closed follow enum. Exactly one value — 0 dual-on by construction. */
export type FollowDirection = "none" | "infini_to_epaper" | "epaper_to_infini";

export type FollowUiId =
  | "follow.off"
  | "follow.following_epaper"
  | "follow.peer_following_you"
  | "follow.local_nav_turns_off"
  | "follow.connection_lost"
  | "follow.reconnect_stays_off";

/** Why `direction` is `none` while a session is (or was) live. */
export type FollowOffKind = "default" | "local_nav" | "reconnect";

/** UI-IN-04 copy table (Spec-drafted). */
export const FOLLOW_COPY = {
  ariaOff: "Follow Epaper",
  ariaOn: "Following Epaper",
  ariaPeer: "Follow Epaper",
  ariaPeerDesc: "Epaper is following you. Click to follow the tablet instead.",
  ariaUnavailable: "Follow Epaper unavailable",
  captionOff: "Follow off",
  captionOn: "Following Epaper",
  captionPeer: "Epaper is following you",
  captionLost: "No session — follow off",
  captionReconnect: "Reconnected — follow stays off",
  captionLocalNav: "Local pan turned follow off",
} as const;

export const FOLLOW_P95_MS = 300;

export interface FollowToggleView {
  ui: FollowUiId;
  direction: FollowDirection;
  pressed: boolean;
  disabled: boolean;
  caption: string;
  ariaLabel: string;
  ariaDescription?: string;
}

export function parseFollowDirection(raw: unknown): FollowDirection {
  if (raw === "infini_to_epaper" || raw === "epaper_to_infini") return raw;
  return "none";
}

export function infiniIsFollowing(direction: FollowDirection): boolean {
  return direction === "epaper_to_infini";
}

export function epaperIsFollowing(direction: FollowDirection): boolean {
  return direction === "infini_to_epaper";
}

/** Dual-on is impossible: the enum holds one direction. */
export function dualFollowOn(direction: FollowDirection): boolean {
  return infiniIsFollowing(direction) && epaperIsFollowing(direction);
}

export function shouldPublishViewportDown(direction: FollowDirection): boolean {
  return direction === "infini_to_epaper";
}

export function shouldApplyInboundTabletViewport(direction: FollowDirection): boolean {
  return direction === "epaper_to_infini";
}

export function followToggleView(input: {
  connected: boolean;
  direction: FollowDirection;
  offKind: FollowOffKind;
}): FollowToggleView {
  const { connected, direction, offKind } = input;
  if (!connected) {
    return {
      ui: "follow.connection_lost",
      direction: "none",
      pressed: false,
      disabled: true,
      caption: FOLLOW_COPY.captionLost,
      ariaLabel: FOLLOW_COPY.ariaUnavailable,
    };
  }
  if (direction === "epaper_to_infini") {
    return {
      ui: "follow.following_epaper",
      direction,
      pressed: true,
      disabled: false,
      caption: FOLLOW_COPY.captionOn,
      ariaLabel: FOLLOW_COPY.ariaOn,
    };
  }
  if (direction === "infini_to_epaper") {
    return {
      ui: "follow.peer_following_you",
      direction,
      pressed: false,
      disabled: false,
      caption: FOLLOW_COPY.captionPeer,
      ariaLabel: FOLLOW_COPY.ariaPeer,
      ariaDescription: FOLLOW_COPY.ariaPeerDesc,
    };
  }
  if (offKind === "local_nav") {
    return {
      ui: "follow.local_nav_turns_off",
      direction: "none",
      pressed: false,
      disabled: false,
      caption: FOLLOW_COPY.captionLocalNav,
      ariaLabel: FOLLOW_COPY.ariaOff,
    };
  }
  if (offKind === "reconnect") {
    return {
      ui: "follow.reconnect_stays_off",
      direction: "none",
      pressed: false,
      disabled: false,
      caption: FOLLOW_COPY.captionReconnect,
      ariaLabel: FOLLOW_COPY.ariaOff,
    };
  }
  return {
    ui: "follow.off",
    direction: "none",
    pressed: false,
    disabled: false,
    caption: FOLLOW_COPY.captionOff,
    ariaLabel: FOLLOW_COPY.ariaOff,
  };
}

export function cloneViewport(vp: Viewport): Viewport {
  return { translate: { x: vp.translate.x, y: vp.translate.y }, scale: vp.scale };
}

export function viewportsMatch(a: Viewport, b: Viewport, eps = 1e-9): boolean {
  return (
    Math.abs(a.translate.x - b.translate.x) <= eps &&
    Math.abs(a.translate.y - b.translate.y) <= eps &&
    Math.abs(a.scale - b.scale) <= eps
  );
}

/** Gherkin drawingRegion { x, y, w, h } — WorldLayer crop, not CSS frame. */
export interface DrawingRegionXywh {
  x: number;
  y: number;
  w: number;
  h: number;
}

/**
 * Infini WorldLayer camera after follow apply or local-nav.
 * Uniform scale only — 0 rotation / 0 skew by construction ([SRS-IN-01]).
 * @implements [SRS-IN-20] apply tablet viewport to WorldLayer
 */
export interface WorldLayerPose {
  translate: { x: number; y: number };
  scale: number;
  scaleX: number;
  scaleY: number;
  rotation: number;
  skew: number;
  drawingRegion: DrawingRegionXywh;
}

export function aabbToXywh(a: Aabb): DrawingRegionXywh {
  return { x: a.minX, y: a.minY, w: a.maxX - a.minX, h: a.maxY - a.minY };
}

export function xywhToAabb(r: DrawingRegionXywh): Aabb {
  return { minX: r.x, minY: r.y, maxX: r.x + r.w, maxY: r.y + r.h };
}

export function cloneAabb(a: Aabb): Aabb {
  return { minX: a.minX, minY: a.minY, maxX: a.maxX, maxY: a.maxY };
}

export function identityWorldLayer(): WorldLayerPose {
  return {
    translate: { x: 0, y: 0 },
    scale: 1,
    scaleX: 1,
    scaleY: 1,
    rotation: 0,
    skew: 0,
    drawingRegion: { x: 0, y: 0, w: 0, h: 0 },
  };
}

/**
 * Apply inbound tablet pose immediately: translate + uniform scale + drawingRegion.
 * @implements [SRS-IN-20] apply while epaper_to_infini
 * @implements [SRS-IN-22] 0 rotation or skew; scale_x equals scale_y
 */
export function worldLayerFromTabletViewport(input: {
  translate: { x: number; y: number };
  scale: number;
  drawingRegion: Aabb | DrawingRegionXywh;
}): WorldLayerPose {
  const drawingRegion =
    "minX" in input.drawingRegion ? aabbToXywh(input.drawingRegion) : { ...input.drawingRegion };
  const scale = input.scale;
  return {
    translate: { x: input.translate.x, y: input.translate.y },
    scale,
    scaleX: scale,
    scaleY: scale,
    rotation: 0,
    skew: 0,
    drawingRegion,
  };
}

export function worldLayerFromLocalViewport(
  vp: Viewport,
  drawingRegion: DrawingRegionXywh,
): WorldLayerPose {
  return worldLayerFromTabletViewport({
    translate: vp.translate,
    scale: vp.scale,
    drawingRegion,
  });
}
