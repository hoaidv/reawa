---
feature: ink-box
parent_req: [REQ-05, REQ-06]
version: 0.1.0
lifecycle: active
---

# SRS — Ink-box on the device (Logic)

Architect-owned recognition and manipulation rules for
[REQ-05](../../prd.md#device-ink-box) and [REQ-06](../../prd.md#device-manipulation).
Ownership: [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md).
Smart Group semantics: [ADR-0011](../../../../adr/ADR-0011-smart-group.md) (unchanged) and
[domain/vector-document](../../../../domain/vector-document.md) §SmartGroup.
Document it writes: [SRS-EP-07](../device-document/srs-logic.md). Tool arming:
[SRS-EP-04](../tool-modes/srs-logic.md). Chrome: [SRS-EP-12](./srs-ui.md).
Product depth: [srs-product](./srs-product.md).

**Inheritance, stated once.** [SRS-EP-10](#srs-ep-10-device-recognition) inherits infini
[SRS-IN-10], [SRS-IN-15], [SRS-IN-16]; [SRS-EP-11](#srs-ep-11-device-manipulation) inherits
[SRS-IN-11]. The rules were correct — the **host** was wrong. Where a rule changes, it is marked
**changed**; everything else is verbatim and must stay bit-compatible with the desktop's reader
([shared fixtures](../device-document/srs-data.md)).

---

## [SRS-EP-10] Recognition, guards, and membership {#srs-ep-10-device-recognition}

Parent REQ: [REQ-05](../../prd.md#device-ink-box).

### Endpoint(s)

N/A — device-local, synchronous at pen-up. **No message is sent or awaited to create a box**
([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §1). The resulting
`create_smart_group` publishes afterwards ([SRS-EP-08](../device-document/srs-logic.md)) as a
consequence, never as a precondition.

### Trigger dispatch at pen-up

**Changed:** there is no `intent` flag on the wire and no `pickables` cache. The device reads its own
exclusive tool **and** recognizer toggles ([SRS-EP-04](../tool-modes/srs-logic.md)) at **pen-down**,
latches them for the stroke, and dispatches at pen-up per
[ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md).

| Armed at pen-down | Pen-up path |
|---|---|
| `pen` + closure-first dispatch | 1 closed-ish **and** `recog.ink_box` → enclose (below); **guards fail → fall through** · 2 draw-into membership · 3 open **and** `recog.connector` → [SRS-EP-17](../connector-ink/srs-logic.md) · 4 ordinary ink |
| `sel_rect` / `sel_freeform` | No ink was produced; not an ingestion path at all |

Latching at pen-down matters: switching tools or toggles mid-stroke must not retroactively change
what the stroke means.

### Enclose recognition — inherited from [SRS-IN-10]

| Step | Rule |
|---|---|
| Trigger | Pen-up of a closed-ish stroke with `recog.ink_box` armed (**changed** — was exclusive `ink_box` tool) |
| Closed-ish | First–last sample gap ≤ **max**(48 world, 0.15 × polyline length). A handwritten close (start near end) on a large box **is** closed. **Changed 2026-08-15** — was AND of cap and fraction, which rejected near-closes on large boxes |
| Candidate shape | Closed or near-closed polyline fitting an axis-aligned rect; **rectangle only** |
| Fitted bounds | AABB of the enclose stroke samples → `(x, y, width, height)` |
| Guard — size | Shorter side ≥ `MIN_ENCLOSE_WORLD` = **48 world units** ([ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) §6, kept by ADR-0014 §7) |
| Guard — content | Prefer ≥1 **free top-level Ink** with **≥80%** of samples inside. **0 content is allowed**: a closed-ish stroke on empty canvas still creates a boundary-only Smart Group (**changed 2026-08-15**). If that stroke already qualifies as draw-into of an existing Smart Group (≥80% samples inside), **do not** create — fall through to membership (D21 / [CHL-0011](../../../../../.plan/iter-003/challenges/CHL-0011-nested-smartgroup-enclose.md)) |
| Guard — already grouped | Ink whose parent is already a `SmartGroup` is **skipped**; remaining free ink still captures |
| Commit | `create_smart_group` immediately — no proposal, no accept step. The enclose stroke becomes `role: boundary` ink; captured ink becomes `role: content` in group-local coordinates; `bounds` = fitted rect; **each content ink seeded with its own `layoutOffset` UV** |
| Guard fails | **Fall through** to draw-into membership ([ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md) step 2), then connector, then ordinary ink. No error state, no banner ([SRS-EP-12](./srs-ui.md)) |
| Undo | One entry; one undo restores the pre-op tree exactly ([SRS-EP-07](../device-document/srs-logic.md)) |
| Visibility | The box is on the panel **p95 ≤500 ms after pen-up**, with 0 peer messages required ([SRS-EP-14](./srs-quality.md)) |

Recognition is internal: `recognize_enclose` is not an op and never appears on the wire. Only its
outcome — `create_smart_group` — is published.

### Selection create, surround required — inherited from [SRS-IN-16]

**Invocation (adopted [CHL-0013](../../../../../.plan/iter-003/challenges/CHL-0013-selection-create-feedback-enclose-cta.md) /
[ADR-0016](../../../../adr/ADR-0016-selection-create-enclose-cta.md)):** runs only when the creator
activates `cta.enclose` on SelectionOverlay — never from pen-up alone.

| Step | Rule |
|---|---|
| Select (arm) | Exclusive ToolChip: `sel_rect` or `sel_freeform` ([ADR-0021](../../../../adr/ADR-0021-connector-toolchip.md)). Switching mid-gesture is ignored until pen-up. |
| Select (rect) | `sel_rect` armed. Pen-down + move draws a thin dotted **axis-aligned rectangle** (rubber-band from down to tip). On pen-up: Ink if **≥80% of samples** lie inside the rectangle; other pickables if **≥80% of their world AABB area** lies inside. A grazing AABB intersect is **not** enough. |
| Select (freeform) | `sel_freeform` armed. Pen-down + move appends samples to a thin dotted **polyline**. On pen-up the polyline **closes** (edge last→first for the test; stored samples stay as drawn). Membership: Ink — ≥80% of samples inside the closed polyline (even-odd); other nodes — ≥80% of a 5×5 grid on the world AABB inside. **Not** AABB-intersect of the gesture. Settled chrome is **not** the polyline — see next row. |
| Select (feedback, settled) | After **either** gesture: thin dotted **selection rect** = **tight** union AABB of selected nodes (**0** extra padding); **6 square anchors** (visual only this campaign). The freeform polyline is **gone** once settled. |
| Input for create | Free top-level `Ink` nodes in the selection (≥2), or ≥1 content-role ink + 1 candidate surround ink. **SmartGroup in selection → refuse** (no nesting — [CHL-0011](../../../../../.plan/iter-003/challenges/CHL-0011-nested-smartgroup-enclose.md)). Non-ink non-SG selected nodes are ignored by the surround algorithm (not captured). |
| Surround candidate | For each selected free ink `S`, build an **artificial closed path** if `S` is open (append edge first→last **for the test only** — never mutate stored samples). Point-in-polygon uses the **even-odd** fill rule. A candidate qualifies when ≥80% of the samples of **every other** selected free ink lie inside |
| Winner | The qualifying candidate; if several qualify, the highest paint/z order (later sibling) |
| Commit | Winner → `role: boundary`; others → `role: content` in group-local coords; `bounds` = fitted AABB of the winner; each content ink seeded with its `layoutOffset` UV |
| Refuse | No qualifying surround, or SmartGroup in selection → **do not create**; selection unchanged; reason visible ([SRS-EP-12](./srs-ui.md) refuse state) |
| Undo | One entry |

There is no AABB-only Smart Group from a selection: a box always has boundary ink the creator drew.
Self-intersecting surrounds are accepted as-is under even-odd — no geometry clean-up this iter.

### Draw-into membership — inherited from [SRS-IN-15]

Runs at pen-up as **step 2** of [ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md)
(including after a **failed** enclose). Not skipped merely because the stroke was closed-ish.

| Step | Rule |
|---|---|
| Trigger | A new `Ink` node after its samples are committed in world space |
| Candidates | Every `SmartGroup` whose **world** `bounds` contain ≥80% of the stroke's samples |
| None | Leave the ink at its ordinary parent (document root) |
| One | Reparent as `role: content` (samples → group-local); **seed that ink's `layoutOffset` UV** from its AABB centroid within the current bounds |
| Several (incl. nested) | Highest paint/z order wins — tree sibling order, later siblings win. **No dual parent**, no z-index field |
| Layout | Do **not** translate, scale, or reflow any existing content ink; new ink stays as drawn |
| Bounds | `SmartGroup` bounds are **not** expanded by membership (**this campaign**). Auto-expand on draw-into is **future** under sizing `WRAP_CONTENT` only — [CHL-0012](../../../../../.plan/iter-003/challenges/CHL-0012-inkbox-sizing-align.md) |
| Undo | One entry |

### Sizing / align (out of this campaign)

Shipping policy remains `inkScaleMode`: `withBounds` | `fixedInk` ([ADR-0011](../../../../adr/ADR-0011-smart-group.md)).
**Not** in this campaign: box sizing `FREE_FORM` | `WRAP_CONTENT`, or `align-content`
TOP|RIGHT|BOTTOM|LEFT for content-ink. See [CHL-0012](../../../../../.plan/iter-003/challenges/CHL-0012-inkbox-sizing-align.md).

### `layoutOffset` + `fixedInk` draw rule (locked, inherited)

Canonical field: `{ u, v }` ([SRS-EP-09](../device-document/srs-data.md) →
[SRS-IN-09](../../../infini/features/vector-document/srs-data.md)).

| Mode | Draw / resize |
|---|---|
| `withBounds` | Scale samples by `scaleX` / `scaleY`, then rotate, then translate. `layoutOffset` is stored but unused for placement |
| `fixedInk` | Do **not** scale content samples. Target local centroid = `(bounds.x + u·width, bounds.y + v·height)`. Translate samples so their AABB centroid matches that target, then apply rotation + group translate only |

Boundary ink is **not** governed by `inkScaleMode` — it always follows the group transform. This is
the exact clause CHL-0004 and CHL-0005 broke on; the device implementation must be fixture-verified
against it before any resize story is called done.

### Errors / partial failure

| Case | Behavior |
|---|---|
| Enclose passes guards but capture finds nothing after skipping grouped ink | No box; stroke stays ordinary ink |
| Membership target removed between commit and reparent | Fall back to the ordinary parent; log |
| Group left with zero children | The group is removed in the same op; its ink returns to the parent ([SRS-EP-07](../device-document/srs-logic.md)) |
| Guard evaluation exceeds its budget | Complete it — a late box is acceptable, a wrong box is not ([SRS-EP-14](./srs-quality.md)) |

---

## [SRS-EP-11] Selection, hit-testing, and manipulation {#srs-ep-11-device-manipulation}

Parent REQ: [REQ-06](../../prd.md#device-manipulation).

### Endpoint(s)

N/A — device-local, on the real document. **Changed from [SRS-IN-11]:** there is no `pickables`
array, no `tool_intent`, and no advisory ghost. The device hit-tests its own tree and transforms
real ink; `set_smart_transform` publishes at pen-up as a consequence
([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §4 of Amendments).

### Hit-testing

| Rule | Value |
|---|---|
| Pickable set (single press) | `SmartGroup` nodes, resolved against world `bounds` after transform |
| Pickable set (rect marquee) | Same node types. Hit = **≥80% inside** the rubber-band: Ink by sample count; other nodes by AABB-area overlap. Grazing AABB intersect does **not** select. **Not** ToolChip chrome. Child ink of a SmartGroup is **not** independently selected (parent SmartGroup may be) |
| Pickable set (freeform) | Same node types. Hit = **≥80% inside** the closed polyline (even-odd): Ink by samples; other nodes by 5×5 AABB grid. Gesture AABB is **not** the hit-test. Child ink of a SmartGroup is **not** independently selected |
| Resolution order | Topmost first — later siblings paint above, so they pick first |
| Hit region | Inside `bounds`, plus a handle tolerance band when selected: visual **28 du**, hit **56 du** (14 du pad beyond visual). 1 du = 1 panel pixel @ 226 dpi. **Not** 8 CSS px |
| Source | The **local document** — never a peer-supplied list |
| LOD cutoff | Applies **only when the viewport is zoomed out** (min panel/world scale **< 1.0**). Then picking is disabled if the box's **smaller on-panel axis < 96 du** (world→panel AABB). At scale **≥ 1.0** (identity map / 100% zoom), every SmartGroup is manipulable — enclose min-axis is 48 world, which is a legal box. **Not** `TILE_LOD_SCALE = 0.35` |
| Below cutoff | The press does nothing to the document, and the UI states that manipulation is unavailable ([SRS-EP-12](./srs-ui.md)) |

**Device constants (closed 2026-08-13).** Handle 28/56 du and LOD 96 du min-axis are accepted from
the [EP-012 Spec spike](../../../../../.plan/iter-003/design/device-selection-chrome/ui-spec.md#spike)
against known RM2 panel DPI. They are implement locks for EP-019. Do not fall back to 8 CSS px or
`TILE_LOD_SCALE = 0.35`. First-device miss-rate may file a `CHL-*` with a new **du** number — never
a desktop constant.

Below the cutoff the press **falls through to nothing** — not to pan, because there is no on-device
pan ([epaper Non-Goals](../../prd.md)). That is the one line of [SRS-IN-11] that could not be
inherited as written.

### Gestures

| Gesture | Precondition | Result |
|---|---|---|
| Press inside SmartGroup bounds + drag | at/above LOD cutoff; not starting a marquee | **Move** — the real ink follows the pen; `set_smart_transform` translate on release |
| Press, no drag | at/above LOD cutoff on a SmartGroup | **Select** — handles appear on geometric `bounds` |
| Pen-down + move (marquee) | `selection` tool; press not on a SmartGroup handle / not claiming a move | **Rubber-band** — thin dotted AABB follows pen tip; on pen-up, select intersecting document nodes; show selection rect + **6** anchors |
| Drag a handle | SmartGroup selected (manipulation chrome) | **Resize** — bounds follow the handle; `withBounds` scales content, `fixedInk` preserves each content ink's UV (draw rule above) |
| Toggle `inkScaleMode` | SmartGroup selected | `set_ink_scale_mode` |
| Tap `cta.enclose` | Selection non-empty | Run selection-create ([SRS-EP-10](./srs-logic.md#srs-ep-10-device-recognition)); refuse path if no surround / SmartGroup in set |
| Press empty canvas | — | Deselect; **0** residual chrome on the next settled frame (CHL-0007) |
| Press another pickable | — | Selection moves to that node |

**Out of scope:** rotation handles and connector attachment to a `SmartGroup`. Anchor **events** on
the 6 selection anchors (drag-to-resize the marquee selection) — later; chrome is visual only for
EP-018. Anchor resolution for connectors lands in [REQ-08](../../prd.md#node-manipulation).

### Live manipulation — the rule the pilot did not have

| Rule | Value |
|---|---|
| What moves | The **document's** ink, transformed live. There is no advisory outline that a peer later corrects |
| Paint during the gesture | **ToolCanvasLayer** draws the live node; CanvasLayer hides the origin box only ([CHL-0018](../../../../../.plan/iter-003/challenges/CHL-0018-live-node-tool-canvas.md)). Not a second document, not a ghost |
| Commit | On release, the committed transform **equals** the last previewed transform. 0 px jump, 0 snap-back. Settled raster returns to CanvasLayer |
| Op granularity | **One** `set_smart_transform` per completed gesture, not per frame — intermediate frames are local paint only |
| Refresh | Partial refresh only during the gesture; **0** full-panel invalidations ([SRS-EP-02](../region-sync/srs-logic.md)) |
| Feedback rate | ≥5 Hz; no stall >200 ms. Slow is acceptable, wrong is not (CHL-0006) |
| Interruption | An inbound `doc_load` never interrupts a gesture — it defers ([SRS-EP-08](../device-document/srs-logic.md)) |
| Undo | One entry per gesture |

"Committed equals previewed" is the single assertion that closes CHL-0005, CHL-0006, and CHL-0007.
It is only implementable because the preview *is* the document — which is the whole point of
ADR-0014.

### Bounds normalization

A handle dragged past the opposite edge normalizes **before** commit: `width` / `height` stay
non-negative, content and boundary follow the normalized rect, and no negative-size state ever
enters the document or the wire ([SRS-EP-09](../device-document/srs-data.md)).

### Conformance to the future model

[REQ-06](../../prd.md#device-manipulation) is a declared subset of
[REQ-08](../../prd.md#node-manipulation), so this section ships against the shared shapes rather
than bespoke ones ([node-manipulation srs-product](../node-manipulation/srs-product.md)):

| Contract | This iter's obligation |
|---|---|
| Capability descriptor | `SmartGroup` declares exactly `{select, move, resize, set-ink-scale-mode}`. **0** hard-coded "if SmartGroup" branches in the gesture router |
| Selection model | One selection holder, node-kind agnostic; **marquee multi-select adopted** for Creation B ([CHL-0013](../../../../../.plan/iter-003/challenges/CHL-0013-selection-create-feedback-enclose-cta.md)) — not full REQ-08 align/distribute |
| Gizmo geometry | Bounds + handles computed from a node-kind-agnostic bounds provider |
| Transform envelope | Carries a **reserved `rotation` field**, always unset this iter; nothing may assume it cannot be set |

A gesture router that switches on node kind would pass this iter's tests and fail REQ-08. That is why
the descriptor is a logic requirement here and not merely a product aspiration.

### Errors / partial failure

| Case | Behavior |
|---|---|
| Link drops mid-gesture | The gesture completes and commits locally; the change queues |
| Selected node removed by an undo | Selection clears; **0** orphaned chrome |
| Selected node not present after a `doc_load` | Selection clears silently — the load is a new epoch |
| Gesture starts while a refresh is in flight | The gesture wins; feedback uses partial refresh |
| Transform would produce a degenerate box (zero width/height) | Clamp to the minimum and commit the clamped value — never a zero-area node |

---

## Superseded

New sections. They inherit, on the device, infini
[SRS-IN-10](../../../infini/features/vector-document/srs-logic.md#srs-in-10),
[SRS-IN-15](../../../infini/features/vector-document/srs-logic.md#srs-in-15-draw-into-membership),
[SRS-IN-16](../../../infini/features/vector-document/srs-logic.md#srs-in-16-selection-create-surround),
and [SRS-IN-11](../../../infini/features/vector-document/srs-logic.md#srs-in-11-selection-manipulation)
— all deprecated 2026-08-13 under CHL-0008. See the
[lifecycle map](../../../../../.plan/iter-003/lifecycle-map-2026-08-13.md).
