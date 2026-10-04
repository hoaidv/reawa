---
title: Document forest — connector
lifecycle: active
owner: architect
source: ADR-0041
---

# Connector

<a id="connector"></a>

Part of the [document forest](./index.md). Decision: [ADR-0041](../../adr/ADR-0041-document-forest.md).

This file reorganizes the connector’s **data**. The algorithms that produce today’s
behavior stay as they are: rest shape, anchors and warp from
[ADR-0020](../../adr/ADR-0020-connector-ink-geometry.md) (Connector-ink geometry: rest
shape, cubic and morph warps); end decoration from
[ADR-0038](../../adr/ADR-0038-endpoint-ink-face-frame.md) (Endpoint-ink on
ConnectorAnchor); and attachment `t` from
[ADR-0027](../../adr/ADR-0027-attachment-t-rest-spine.md) (Attachment parameter t on
connector rest spine). The [mapping table](#mapping) shows where each current field
goes.

## The idea

A connector is a **line between two ink boxes, drawn by hand**. The creator’s stroke
is remembered once, at rest, as an offset from a smooth centre line. When either box
moves, the centre line is re-routed between the two ends, and the remembered stroke
is laid back along it. Decorations at the ends ride the end they were drawn on.
Nodes hung on the line ride a fixed position along it.

Names follow common diagram and graph vocabulary: *source* and *target* ends, an
*anchor* on a box *side*, *markers* at the ends, *labels* along the line, and a
*rest path* that a *route* is derived from.

## Payload

<a id="payload"></a>

```text
Connector                                   a leaf; a Document child; transform is identity
  source:  ConnectorEnd
  target:  ConnectorEnd
  style:   Curve | Ink                      how the route is formed; wire: cubic | morph
  rest:    RestPath                         fixed at creation; never rebuilt (ADR-0020 I1)
  stroke:  { color, width }                 width in world units
  labels:  [EdgeLabel]
  route:   DerivedPath                      derived by the writer; not a document fact

RestPath
  spine:   [Pt]                             S: the stroke smoothed, resampled every 2 u, world space
  body:    [PathPt]                         the creator's ink, in path coordinates of S

PathPt     = { s, d }                       s ∈ [0,1]: normalized arc length along S
                                            d: signed perpendicular offset, world units, never scaled

ConnectorEnd
  node:      NodeRef                        the bound InkBox (handles.md §6)
  anchor:    SideAnchor | CenterAnchor
  departure: FacePt                         unit; the direction the ink left the box (side: face frame; centre: box frame)
  marker:    EndMarker
  lastPose:  Pose                           last resolved world point and facing; derived, persisted, not an op

SideAnchor   = { side: Top | Right | Bottom | Left, t ∈ [0,1] }
CenterAnchor = { }

EndMarker
  shape:     None | Arrow | ArrowOpen | Star | One | Many     closed set; Path A (not on Epaper yet)
  strokes:   [[FacePt]]                     hand-drawn decoration (Path B); erasable stroke by stroke
FacePt     = { n, e }                       in the end's face frame

EdgeLabel                                   "attachment" in current docs
  node:      NodeRef                        the hung node
  t:         [0,1]                          on rest spine S, never on the route
  offset:    d                              signed perpendicular offset, world units

DerivedPath                                 derived; recomputed by the writer, read by readers
  spine:     [WorldPt]                      V
  body:      [WorldPt]                      the ink laid along V; this is what paints
  markers:   [[WorldPt]]
```

Connectors are `Document` children with identity transform, because `S` and `d`
are in world units. A connector is not a container. Its markers are payload, not
child nodes. Its labels are separate nodes elsewhere in the forest.

## Frames on a box

<a id="anchor"></a>

Corners of the bound box’s `bounds`, in order `0 = top-left`, `1 = top-right`,
`2 = bottom-right`, `3 = bottom-left`. `side` is the side from `corner[side]` to
`corner[side + 1]` (`Top = 0` … `Left = 3`). Sides are box sides, not world sides: if
a later transform rotates the box, `Top` is still the same side.

- **Attach point** of a side anchor: `corner[side] + (corner[side+1] − corner[side]) · t`
  on the box’s **current** `bounds`, mapped to world. A centre anchor attaches at the
  centre of `bounds`.
- **Face frame** of a side: `e` is the unit vector along the side’s world image, from
  `corner[side]` to `corner[side+1]`. `n` is the unit vector perpendicular to `e`,
  pointing out of the box. With `y` down and no mirror, `n = (e.y, −e.x)`. The frame
  is orthonormal by construction: it is built from the side’s direction alone, never
  by mapping two stored axes through the box’s scale.
- **Box frame**, for a centre end: `x` is the unit vector along the world image of the
  top side, and `y` is perpendicular to it, pointing toward the bottom side.

<a id="departure"></a>

### Departure stays fixed relative to the node

**Requirement.** The angle at which the line leaves its node, measured from that
node, does not change when the node or any of its ancestors moves, resizes, is
reparented, or later gets a transform. The connector keeps the same look whatever
happens to the boxes.

**Rule.** `departure` is stored once, in the end’s face frame, as `{n, e}`: the same
frame the hand-drawn markers use ([ADR-0038](../../adr/ADR-0038-endpoint-ink-face-frame.md)).
In world, the facing is `departure.n · n′ + departure.e · e′`, where `{n′, e′}` is the
face frame built from the side’s current world image.

Why this holds in every case:

| Change | Effect on the side’s world image | Departure in world |
|---|---|---|
| Node or ancestor moves; reparenting | translated | unchanged |
| Node resized by a knob, either `manipMode` | stays the same side of an axis-aligned rectangle, only longer or shorter | unchanged: the frame takes only the side’s direction, not its length |
| Ancestor resized in `All` mode | the nested box’s `bounds` is scaled ([index.md](./index.md#inkbox-manipulation)), same as above | unchanged |
| Later: the node or an ancestor rotates | the side rotates | rotates with the side, so the angle to the side is kept |
| Later: uneven scale above a rotation (the side’s image is one side of a parallelogram) | the side is sheared | the angle to **this side** is kept; it is the only angle the frame measures |

The last row is why the stored frame is the face frame and not the box axes. If the
departure were stored in box axes and mapped through the box’s linear map, an uneven
ancestor scale would skew it. The box-axes form only appears to agree because, today,
every box’s world image is an axis-aligned rectangle.

**Same output as today.** Today the live warp reads only the box-axes copy
(`drawnBoxX`, `drawnBoxY`): `facingAtAttach` in `epaper_old/document/connector_warp.hpp`
and `drawnBoxLocal` in `infini/src/document/connectorWarp.ts`. For an axis-aligned
rectangle, the two frames are related by a fixed rotation per side:

| side | `e` in box axes | `n` in box axes |
|---|---|---|
| Top | (1, 0) | (0, −1) |
| Right | (0, 1) | (1, 0) |
| Bottom | (−1, 0) | (0, 1) |
| Left | (0, −1) | (−1, 0) |

So the face-frame rule gives today’s facing for every box the forest can hold while
`transform` is reserved. Import converts `drawnBoxX/Y` through this table and ignores
the stored `drawnN/drawnE`. Their only reader, `facingOnBox`, has no caller.

**Centre end.** A centre anchor has no side, and the requirement does not apply to
it. Its facing is the ray toward the other end ([decided](#adr-0020-drift)). Its
`departure` is still stored, in the box frame, for the marker rotation.

Today’s stored box-local attach point (`localX`, `localY`) is dropped. The attach
point is found from `side + t` on the current `bounds`, so it stays at the same
fraction along the side through any resize. Import projects the stored point onto its
side to get `t`.

## Deriving the route

<a id="derive"></a>

The current algorithm, written with these names. The equations are in
[ADR-0020 §4](../../adr/ADR-0020-connector-ink-geometry.md).

```text
derive(c):
  e0 = resolve(c.source, peer = c.target)       world point P0, facing f0, isCenter
  e1 = resolve(c.target, peer = c.source)
  U  = similarity map of c.rest.spine onto the chord P0 → P1
  C  = cubic Hermite from P0 (handle f0·L') to P1 (handle −f1·L'), sampled at U's parameters
  if c.style == Curve:  V = C
  else (Ink):           turn = max(end turns); m = versine(turn, saturating at 90°)
                        V = (m == 0) ? U : (1−m)·U + m·C     m = 0 is a true skip
  body    = each PathPt (s, d) located on U, positioned and oriented by V
  markers = each FacePt → origin + n·N' + e·E', rotated by α
            α = signed angle from the stored departure to V's tangent at that end
  for each label: place its node at V(t) + d · normal(V, t)
  c.route = { V, body, markers }; for each live end: lastPose = (P, f)
```

**Facing at an end.** For a side anchor, it is `departure` mapped to world through
the side’s face frame ([Departure](#departure)). For a centre anchor, it is the
unit ray from this end’s attach point toward the other end’s attach point, as
shipped.

<a id="adr-0020-drift"></a>

**Code and ADR-0020 differ on centre ends.** ADR-0020 §2 specifies a centre facing
that is the drawn departure clamped to a 60° cone, plus clipping the body at the box
boundary. In the shipped code on both peers, the live path uses the plain ray to the
peer. The cone exists only in the uncalled `facingOnBox`. Every branch of
`resolveConnectorEnds` sets `hasClip = false`, so the centre clip never runs. This
file follows the code. For a centre end, `departure` is still stored and still sets
the marker rotation `α`.

**Decided (human, 2026-10-04): keep the ray.** A centre end turns toward its peer as
the peer moves; only side ends hold a fixed angle to their node. The alternatives
were the drawn direction fixed in the box frame, and ADR-0020’s cone and clip.
ADR-0020 §2 still describes the cone and the clip, so it disagrees with both the
code and this model. Fixing that text is part of the supersession pass.

<a id="missing-end"></a>

**Missing end.** Delete keeps the connector:

| Source | Target | Route from |
|---|---|---|
| live | live | both resolved ends |
| live | `Gone` | source resolved; target from its `lastPose` |
| `Gone` | `Gone` | both `lastPose`s: a frozen stroke between two poses |
| `Gone` with no `lastPose` | — | not drawable; kept |

When undo restores a deleted box, it comes back with the same id. The writer updates
`NodeRef.cached` and the connector resolves live again
([handles.md](./handles.md#use-noderef)).

## Who computes what, and when

`route` is derived data. It is not a document fact, it does not change `lastOpId`,
and it is not on the wire. The **writer** recomputes it in the same commit that
changes an input: either end box’s world placement or `bounds` (move, resize, an
ancestor’s resize, reparent), `style`, an anchor, or markers.
The `dependents` index finds the affected connectors. The new `route` is published
as one granule ([concurrency.md](./concurrency.md#granules), Rule 2). Readers never
re-derive; they paint `route.body` and `route.markers`.

`lastPose` is also written by the writer during that commit. It is persisted, so a
reload after a delete still draws, but writing it is not an op.

During a live drag of a bound box, the document is not written. The tool overlay
warps a preview with the same `derive` function. The commit at pen-up must give the
same pixels as the last preview frame (0 px jump, as today).

<a id="labels"></a>

**Labels.** A labelled node keeps its own transform. Its position along the line is
added by its parent link’s placement, `Follow { offset }`, set by the writer to the
translation that moves the node’s bounds centre onto `V(t) + d · normal`. The visit
composes placement for every child anyway, so labels need no special case. In this
version, labelled nodes are `Document` children, like connectors.

<a id="bounds"></a>

**Bounds.** `bounds` is the hull of `route.body` and `route.markers`. The paint
extent pads it by half the stroke width. Both change whenever `route` does, which
updates the connector’s one entry in the `Document` R-tree.

## Behavior kept as is

| Behavior | Rule lives in |
|---|---|
| Recognition guards and chain | [SRS-EP-17](../../modules/epaper/features/connector-ink/srs-logic.md#srs-ep-17-connector-recognition) |
| Style auto-pick from inflections of `S` | ADR-0020 §3 |
| Hand-drawn markers: steal at pen-up, 5 mm, 80% length | ADR-0038 |
| Erase of markers and of the connector | ADR-0038 §5 |
| Label `t` on `S`, never on `V`; no rebake | ADR-0027 |
| Tap selects only on the stroke, not on the box | [SRS-EP-79](../../modules/epaper/features/device-document/srs-logic.md#srs-ep-79-geometry-queries) |

## Mapping from current fields

<a id="mapping"></a>

From `DocNode` in `epaper_old/document/doc_model.hpp`:

| Current | Forest | Note |
|---|---|---|
| `fromNodeId`, `toNodeId` | `source.node`, `target.node` | `NodeRef` (id plus cached handle) |
| `ConnectorAnchor.kind` `edge` / `centre` | `SideAnchor` / `CenterAnchor` | A variant instead of a string |
| `edge` 0..3 (N, E, S, W) | `side` Top, Right, Bottom, Left | Same order |
| `t` | `SideAnchor.t` | Same meaning |
| `drawnBoxX`, `drawnBoxY` | `departure` | Side end: converted to the face frame by the [per-side table](#departure). Centre end: kept in the box frame. |
| `drawnN`, `drawnE` | dropped | Ignored on import. No warp reads them today. The face-frame `departure` takes their role. |
| `localX`, `localY`, `hasLocal` | dropped | Equal to `side + t` on the current `bounds` |
| `styleInk` | `marker.strokes` | Same face-frame `{n, e}` |
| Path A end `style` | `marker.shape` | Same closed set |
| `warpStyle` `cubic` / `morph` | `style` `Curve` / `Ink` | The creator-facing names |
| `restSpine` | `rest.spine` | |
| `restOffsets` | `rest.body` | `(s, d)` unchanged |
| `warpedSamples` | `route.body` | Derived |
| `warpedStyleInk` | `route.markers` | Derived |
| `fromPose`, `toPose` (with `valid`) | `source.lastPose`, `target.lastPose` | Missing means not valid |
| `connectorInvalid` | dropped | Delete keeps the connector (ADR-0020 §6.7) |
| `attachments[] {nodeId, t, offset.d}` | `labels[] {node, t, offset}` | |
| `style` (stroke, width) on the header | `stroke` on the payload | Header carries no style |

## Verification

<a id="verification"></a>

| Claim | Status |
|---|---|
| Same rest path, ends and style give byte-identical `route.body` to today’s `connector_warp` on the shared fixtures | **Pending** |
| Face-frame `departure` gives today’s facing on every side end | Checked by reading the code: the live path reads only the box-axes copy, and the per-side table is exact for axis-aligned boxes (2026-10-04). Fixture run **pending**. |
| Departure angle unchanged after a knob resize, an `All`-mode ancestor resize and a reparent | **Pending** test |
| Centre-end facing | Decided: the ray to the peer, as shipped ([drift](#adr-0020-drift)) |
| `side + t` reproduces every stored `localX/Y` in current fixtures | **Pending** |