/**
 * Morph (Ink) and Always-cubic (Curve) warp — pure function of rest + ends.
 * Port of epaper/document/connector_warp.hpp; do not copy EXP-0002 probe.
 * @implements [SRS-IN-09] derive connector geometry from rest + ends
 * @implements [ADR-0020] I1 never re-bake; I3 Morph identity at rest
 */

import { smartLocalToWorld } from "./anchors";
import {
  kEdgeFacingPerpendicular,
  kHandleModeRestSpeed,
  kMorphSatDeg,
} from "./connectorWarpParams";
import type {
  Aabb,
  Anchor,
  ConnectorEndPose,
  ConnectorNode,
  DocNode,
  RestOffset,
  RestShape,
  SmartGroupNode,
  Vec2,
} from "./types";

const kWarpPi = 3.14159265358979323846;

export interface WarpEnd {
  p: Vec2;
  f: Vec2;
  centre: boolean;
  clip: Aabb;
  hasClip: boolean;
}

export interface WarpResult {
  spine: Vec2[];
  samples: Vec2[];
  mixM: number;
  lPrime: number;
}

function vec(x: number, y: number): Vec2 {
  return { x, y };
}

function warpAdd(a: Vec2, b: Vec2): Vec2 {
  return vec(a.x + b.x, a.y + b.y);
}
function warpSub(a: Vec2, b: Vec2): Vec2 {
  return vec(a.x - b.x, a.y - b.y);
}
function warpMul(a: Vec2, k: number): Vec2 {
  return vec(a.x * k, a.y * k);
}
function warpDot(a: Vec2, b: Vec2): number {
  return a.x * b.x + a.y * b.y;
}
function warpLen(a: Vec2): number {
  return Math.hypot(a.x, a.y);
}
function warpUnit(a: Vec2): Vec2 {
  const l = warpLen(a);
  return l > 1e-12 ? vec(a.x / l, a.y / l) : vec(1, 0);
}
function warpLeft(t: Vec2): Vec2 {
  return vec(-t.y, t.x);
}
function warpRot(v: Vec2, th: number): Vec2 {
  const c = Math.cos(th);
  const s = Math.sin(th);
  return vec(v.x * c - v.y * s, v.x * s + v.y * c);
}
function warpAngleDeg(a: Vec2, b: Vec2): number {
  const ua = warpUnit(a);
  const ub = warpUnit(b);
  let c = warpDot(ua, ub);
  c = Math.max(-1.0, Math.min(1.0, c));
  return Math.acos(c) * 180.0 / kWarpPi;
}

function warpArcTable(p: readonly Vec2[]): number[] {
  const cum = new Array<number>(p.length).fill(0);
  for (let i = 1; i < p.length; ++i) {
    cum[i] = cum[i - 1] + warpLen(warpSub(p[i], p[i - 1]));
  }
  return cum;
}

function warpSegTangent(p: readonly Vec2[], i: number): Vec2 {
  const n = p.length;
  if (n < 2) return vec(1, 0);
  for (let j = i + 1; j < n; ++j) {
    const d = warpSub(p[j], p[i]);
    if (warpDot(d, d) > 1e-24) return warpUnit(d);
  }
  for (let j = i; j-- > 0; ) {
    const d = warpSub(p[i], p[j]);
    if (warpDot(d, d) > 1e-24) return warpUnit(d);
  }
  return vec(1, 0);
}

function warpHermite(p0: Vec2, m0: Vec2, p1: Vec2, m1: Vec2, t: number): Vec2 {
  const t2 = t * t;
  const t3 = t2 * t;
  return warpAdd(
    warpAdd(warpMul(p0, 2 * t3 - 3 * t2 + 1), warpMul(m0, t3 - 2 * t2 + t)),
    warpAdd(warpMul(p1, -2 * t3 + 3 * t2), warpMul(m1, t3 - t2)),
  );
}

function warpSimilarity(spine: readonly Vec2[], p0: Vec2, p1: Vec2): Vec2[] {
  const out: Vec2[] = new Array(spine.length);
  if (spine.length === 0) return out;
  const chord = warpSub(spine[spine.length - 1], spine[0]);
  const cNew = warpSub(p1, p0);
  const lOld = warpLen(chord);
  const lNew = warpLen(cNew);
  const scale = lOld > 1e-9 ? lNew / lOld : 1.0;
  const th =
    lNew > 1e-9 && lOld > 1e-9
      ? Math.atan2(cNew.y, cNew.x) - Math.atan2(chord.y, chord.x)
      : 0.0;
  const o = spine[0];
  for (let i = 0; i < spine.length; ++i) {
    out[i] = warpAdd(p0, warpRot(warpMul(warpSub(spine[i], o), scale), th));
  }
  return out;
}

function warpCubicAtU(
  U: readonly Vec2[],
  p0: Vec2,
  f0: Vec2,
  p1: Vec2,
  f1: Vec2,
  h0: number,
  h1: number,
): Vec2[] {
  if (U.length < 2) return U.slice();
  const cum = warpArcTable(U);
  const total = cum[cum.length - 1];
  const m0 = warpMul(f0, h0);
  const m1 = warpMul(f1, -h1);
  const C: Vec2[] = new Array(U.length);
  for (let i = 0; i < U.length; ++i) {
    const s = total > 1e-12 ? cum[i] / total : 0;
    C[i] = warpHermite(p0, m0, p1, m1, s);
  }
  C[0] = p0;
  C[C.length - 1] = p1;
  return C;
}

function placeOnSpine(
  param: readonly Vec2[],
  geom: readonly Vec2[],
  sd: readonly RestOffset[],
): Vec2[] {
  const cum = warpArcTable(param);
  const total = cum.length === 0 ? 0.0 : cum[cum.length - 1];
  const out: Vec2[] = new Array(sd.length);
  for (let i = 0; i < sd.length; ++i) {
    const arc = sd[i].s * total;
    let lo = 0;
    let u = 0;
    if (param.length >= 2 && total > 1e-12) {
      if (arc >= total) {
        lo = param.length - 2;
        u = 1;
      } else if (arc > 0) {
        let hi = param.length - 1;
        while (lo + 1 < hi) {
          const mid = (lo + hi) >> 1;
          if (cum[mid] <= arc) lo = mid;
          else hi = mid;
        }
        const segLen = cum[lo + 1] - cum[lo];
        u = segLen > 1e-12 ? (arc - cum[lo]) / segLen : 0;
      }
    }
    let p = geom.length === 0 ? vec(0, 0) : geom[0];
    let t = vec(1, 0);
    if (geom.length >= 2) {
      p = warpAdd(geom[lo], warpMul(warpSub(geom[lo + 1], geom[lo]), u));
      t = warpSegTangent(geom, lo);
    }
    out[i] = warpAdd(p, warpMul(warpLeft(t), sd[i].d));
  }
  return out;
}

function aabbContains(b: Aabb, p: Vec2): boolean {
  return p.x >= b.minX && p.x <= b.maxX && p.y >= b.minY && p.y <= b.maxY;
}

function aabbExit(inside: Vec2, outside: Vec2, b: Aabb): Vec2 {
  let lo = inside;
  let hi = outside;
  for (let i = 0; i < 48; ++i) {
    const mid = warpMul(warpAdd(lo, hi), 0.5);
    if (aabbContains(b, mid)) lo = mid;
    else hi = mid;
  }
  return hi;
}

function clipCentreEnds(pts: Vec2[], e0: WarpEnd, e1: WarpEnd): void {
  if (e0.centre && e0.hasClip) {
    let n = 0;
    while (n < pts.length && aabbContains(e0.clip, pts[n])) ++n;
    if (n > 0 && n < pts.length) {
      const hit = aabbExit(pts[n - 1], pts[n], e0.clip);
      pts.splice(0, n);
      pts[0] = hit;
    }
  }
  if (e1.centre && e1.hasClip) {
    let n = 0;
    while (n < pts.length && aabbContains(e1.clip, pts[pts.length - 1 - n])) ++n;
    if (n > 0 && n < pts.length) {
      const keep = pts.length - n;
      const hit = aabbExit(pts[keep], pts[keep - 1], e1.clip);
      pts.length = keep;
      pts[pts.length - 1] = hit;
    }
  }
}

function morphMixFromTurn(turnDeg: number): number {
  if (!(turnDeg > 0.0)) return 0.0;
  const sat = kMorphSatDeg > 1e-9 ? kMorphSatDeg : 90.0;
  const u = Math.min(1.0, turnDeg / sat);
  return 0.5 * (1.0 - Math.cos(kWarpPi * u));
}

/**
 * Reconstruct rest samples from S + (s,d). Morph at m=0 must match this bitwise.
 * @implements [SRS-IN-09] rest-shape reconstruction (I3)
 */
export function restShapeReconstruction(rs: RestShape): Vec2[] {
  return placeOnSpine(rs.spine, rs.spine, rs.offsets);
}

/** @implements [SRS-IN-09] Morph skip when m=0; Cubic V=C; d never scaled */
export function warpConnector(
  rs: RestShape,
  e0: WarpEnd,
  e1: WarpEnd,
  style: string,
): WarpResult {
  const out: WarpResult = { spine: [], samples: [], mixM: 0, lPrime: 0 };
  if (rs.spine.length < 2) return out;
  const U = warpSimilarity(rs.spine, e0.p, e1.p);
  out.lPrime = warpArcTable(U)[U.length - 1] ?? 0;
  const chord = warpLen(warpSub(e1.p, e0.p));
  const h = kHandleModeRestSpeed
    ? out.lPrime > 1e-12
      ? out.lPrime
      : chord
    : chord / 3.0;
  const C = warpCubicAtU(U, e0.p, e0.f, e1.p, e1.f, h, h);
  const t0 = warpSegTangent(U, 0);
  const t1 = warpSegTangent(U, U.length >= 2 ? U.length - 2 : 0);
  const turn = Math.max(warpAngleDeg(e0.f, t0), warpAngleDeg(warpMul(e1.f, -1.0), t1));
  out.mixM = morphMixFromTurn(turn);
  const cubic = style === "cubic";
  if (cubic) {
    out.spine = C;
  } else if (!(out.mixM > 0.0)) {
    // I3: true skip, not a zero-parameter mix.
    out.spine = U;
  } else if (out.mixM >= 1.0) {
    out.spine = C;
  } else {
    out.spine = new Array(U.length);
    const om = 1.0 - out.mixM;
    for (let i = 0; i < U.length; ++i) {
      out.spine[i] = warpAdd(warpMul(U[i], om), warpMul(C[i], out.mixM));
    }
    if (out.spine.length > 0) {
      out.spine[0] = e0.p;
      out.spine[out.spine.length - 1] = e1.p;
    }
  }
  out.samples = placeOnSpine(U, out.spine, rs.offsets);
  clipCentreEnds(out.samples, e0, e1);
  return out;
}

export function restShapeFromNode(n: ConnectorNode): RestShape {
  return {
    spine: n.restSpine ?? [],
    offsets: n.restOffsets ?? [],
    warpStyle: n.warpStyle && n.warpStyle.length > 0 ? n.warpStyle : "morph",
  };
}

interface WarpBox {
  corners: [Vec2, Vec2, Vec2, Vec2];
  c: Vec2;
  aabb: Aabb;
  ok: boolean;
}

function emptyWarpBox(): WarpBox {
  return {
    corners: [vec(0, 0), vec(0, 0), vec(0, 0), vec(0, 0)],
    c: vec(0, 0),
    aabb: { minX: 0, minY: 0, maxX: 0, maxY: 0 },
    ok: false,
  };
}

function warpBoxFromSmart(sg: SmartGroupNode): WarpBox {
  const sb = sg.bounds;
  const local: Vec2[] = [
    vec(sb.x, sb.y),
    vec(sb.x + sb.width, sb.y),
    vec(sb.x + sb.width, sb.y + sb.height),
    vec(sb.x, sb.y + sb.height),
  ];
  const b = emptyWarpBox();
  b.aabb = { minX: 1e300, minY: 1e300, maxX: -1e300, maxY: -1e300 };
  let acc = vec(0, 0);
  for (let i = 0; i < 4; ++i) {
    const w = smartLocalToWorld(local[i], sg, "boundary");
    b.corners[i] = w;
    acc = warpAdd(acc, w);
    b.aabb.minX = Math.min(b.aabb.minX, w.x);
    b.aabb.minY = Math.min(b.aabb.minY, w.y);
    b.aabb.maxX = Math.max(b.aabb.maxX, w.x);
    b.aabb.maxY = Math.max(b.aabb.maxY, w.y);
  }
  b.c = vec(acc.x * 0.25, acc.y * 0.25);
  b.ok = true;
  return b;
}

function edgeFrameFromBox(b: WarpBox, edge: number): { n: Vec2; along: Vec2 } {
  const e = ((edge % 4) + 4) % 4;
  const p = b.corners[e];
  const q = b.corners[(e + 1) % 4];
  const along = warpUnit(warpSub(q, p));
  const n = vec(along.y, -along.x);
  return { n, along };
}

function anchorKind(a: Anchor): string {
  return a.kind ?? "edge";
}

function anchorEdge(a: Anchor): number {
  return a.edge ?? 0;
}

function anchorT(a: Anchor): number {
  return a.t ?? 0;
}

function drawnBoxX(a: Anchor): number {
  return a.drawnBoxLocal?.x ?? a.drawnBoxX ?? 1;
}

function drawnBoxY(a: Anchor): number {
  return a.drawnBoxLocal?.y ?? a.drawnBoxY ?? 0;
}

function hasLocalAttach(a: Anchor): boolean {
  return a.hasLocal === true || a.local != null;
}

function anchorPointOnBox(b: WarpBox, a: Anchor): Vec2 {
  if (anchorKind(a) === "centre") return b.c;
  const e = ((anchorEdge(a) % 4) + 4) % 4;
  const p = b.corners[e];
  const q = b.corners[(e + 1) % 4];
  const t = Math.max(0.0, Math.min(1.0, anchorT(a)));
  return warpAdd(p, warpMul(warpSub(q, p), t));
}

function attachWorld(sg: SmartGroupNode, a: Anchor, b: WarpBox): Vec2 {
  if (hasLocalAttach(a)) {
    return smartLocalToWorld(
      { x: a.local?.x ?? 0, y: a.local?.y ?? 0 },
      sg,
      "boundary",
    );
  }
  return anchorPointOnBox(b, a);
}

function facingAtAttach(b: WarpBox, a: Anchor, thisP: Vec2, peerP: Vec2): Vec2 {
  if (anchorKind(a) === "centre") return warpUnit(warpSub(peerP, thisP));
  const { n } = edgeFrameFromBox(b, anchorEdge(a));
  if (kEdgeFacingPerpendicular) return n;
  const ex = warpUnit(warpSub(b.corners[1], b.corners[0]));
  const ey = warpUnit(warpSub(b.corners[3], b.corners[0]));
  return warpUnit(warpAdd(warpMul(ex, drawnBoxX(a)), warpMul(ey, drawnBoxY(a))));
}

function emptyWarpEnd(): WarpEnd {
  return {
    p: vec(0, 0),
    f: vec(1, 0),
    centre: false,
    clip: { minX: 0, minY: 0, maxX: 0, maxY: 0 },
    hasClip: false,
  };
}

export function poseFromWarpEnd(e: WarpEnd): ConnectorEndPose {
  return { x: e.p.x, y: e.p.y, fx: e.f.x, fy: e.f.y, valid: true };
}

export function endFromPose(p: ConnectorEndPose): WarpEnd {
  const e = emptyWarpEnd();
  e.p = vec(p.x, p.y);
  e.f = warpUnit(vec(p.fx, p.fy));
  return e;
}

function asSmart(n: DocNode | undefined): SmartGroupNode | undefined {
  return n?.kind === "smart_group" ? n : undefined;
}

/**
 * Resolve live SmartGroup attach, else last live pose. Never marks invalid (D39).
 * @implements [SRS-IN-09] last-live pose when bound node missing
 */
export function resolveConnectorEnds(
  conn: ConnectorNode,
  find: (id: string) => DocNode | undefined,
): { from: WarpEnd; to: WarpEnd } | null {
  const sg0 = asSmart(find(conn.from.nodeId));
  const sg1 = asSmart(find(conn.to.nodeId));
  const b0 = sg0 ? warpBoxFromSmart(sg0) : emptyWarpBox();
  const b1 = sg1 ? warpBoxFromSmart(sg1) : emptyWarpBox();
  let e0 = emptyWarpEnd();
  let e1 = emptyWarpEnd();
  let ok0 = false;
  let ok1 = false;
  if (b0.ok && b1.ok && sg0 && sg1) {
    e0.p = attachWorld(sg0, conn.from, b0);
    e1.p = attachWorld(sg1, conn.to, b1);
    e0.f = facingAtAttach(b0, conn.from, e0.p, e1.p);
    e1.f = facingAtAttach(b1, conn.to, e1.p, e0.p);
    e0.centre = anchorKind(conn.from) === "centre";
    e1.centre = anchorKind(conn.to) === "centre";
    e0.hasClip = false;
    e1.hasClip = false;
    ok0 = ok1 = true;
  } else if (b0.ok && sg0 && conn.toPose?.valid) {
    e0.p = attachWorld(sg0, conn.from, b0);
    e1 = endFromPose(conn.toPose);
    e0.f = facingAtAttach(b0, conn.from, e0.p, e1.p);
    e0.centre = anchorKind(conn.from) === "centre";
    e0.hasClip = false;
    ok0 = ok1 = true;
  } else if (b1.ok && sg1 && conn.fromPose?.valid) {
    e1.p = attachWorld(sg1, conn.to, b1);
    e0 = endFromPose(conn.fromPose);
    e1.f = facingAtAttach(b1, conn.to, e1.p, e0.p);
    e1.centre = anchorKind(conn.to) === "centre";
    e1.hasClip = false;
    ok0 = ok1 = true;
  } else if (conn.fromPose?.valid && conn.toPose?.valid) {
    e0 = endFromPose(conn.fromPose);
    e1 = endFromPose(conn.toPose);
    ok0 = ok1 = true;
  }
  if (!ok0 || !ok1) return null;
  return { from: e0, to: e1 };
}

/**
 * Re-derive warped samples. Does not rewrite restSpine (I1).
 * @implements [SRS-IN-09] refresh derived connector geometry
 */
export function refreshConnectorWarp(
  conn: ConnectorNode,
  find: (id: string) => DocNode | undefined,
): boolean {
  const ends = resolveConnectorEnds(conn, find);
  if (!ends) return false;
  const rs = restShapeFromNode(conn);
  const w = warpConnector(rs, ends.from, ends.to, conn.warpStyle ?? "morph");
  conn.warpedSamples = w.samples;
  conn.path = w.samples;
  conn.fromPose = poseFromWarpEnd(ends.from);
  conn.toPose = poseFromWarpEnd(ends.to);
  conn.invalid = false;
  return true;
}

/** Wire envelope: derived samples are never streamed. */
export function connectorWirePayload(n: ConnectorNode): Record<string, unknown> {
  const payload: Record<string, unknown> = {
    id: n.id,
    from: n.from,
    to: n.to,
    warpStyle: n.warpStyle ?? "morph",
  };
  if ((n.restSpine?.length ?? 0) > 0 || (n.restOffsets?.length ?? 0) > 0) {
    payload.restShape = { spine: n.restSpine ?? [], offsets: n.restOffsets ?? [] };
  }
  if (n.children && n.children.length > 0) payload.body = n.children;
  if (n.fromPose?.valid) payload.fromPose = { ...n.fromPose };
  if (n.toPose?.valid) payload.toPose = { ...n.toPose };
  return payload;
}
