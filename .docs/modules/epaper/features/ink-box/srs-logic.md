---
feature: ink-box
parent_req: [REQ-05, REQ-06]
version: 0.2.0
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
| `pen` + closure-gated dispatch | 1 `recog.connector` → endpoint-ink ([SRS-EP-35](../connector-ink/srs-logic.md#srs-ep-35-endpoint-ink)) · 2 draw-into membership (boundary-ink length) · 3 closed-ish **and** `recog.ink_box` → enclose (below); **guards fail → fall through** · 4 open **and** `recog.connector` → [SRS-EP-17](../connector-ink/srs-logic.md) · 5 ordinary ink |
| `sel_rect` / `sel_freeform` | No ink was produced; not an ingestion path at all |

Latching at pen-down matters: switching tools or toggles mid-stroke must not retroactively change
what the stroke means.

### Enclose recognition — inherited from [SRS-IN-10]

| Step | Rule |
|---|---|
| Trigger | Pen-up of a closed-ish stroke with `recog.ink_box` armed (**changed** — was exclusive `ink_box` tool) |
| Closed-ish | First–last sample gap ≤ **max**(48 world, 0.15 × polyline length). A handwritten close (start near end) on a large box **is** closed. **Changed 2026-08-15** — was AND of cap and fraction, which rejected near-closes on large boxes |
| Candidate shape | Closed or near-closed polyline; fitted bounds are the sample AABB |
| Fitted bounds | AABB of the enclose stroke samples → `(x, y, width, height)` |
| Guard — size (adaptive) | **With ≥1 capturable content ink:** shorter side ≥ **28** world (`kMinEncloseWithContent`). **Empty boundary:** shorter side ≥ **36** (`kMinEncloseEmpty`). PM 2026-08-15 — supersedes the single 48 from ADR-0013 §6 on device |
| Guard — empty shape | Empty boundary must also match a near-primitive: circle, ellipse, triangle, square, rectangle, parallelogram, diamond, pentagon, hexagon, or octagon (Douglas–Peucker + circularity in `enclose_shape.hpp`). Content enclose skips this gate |
| Guard — content | Prefer ≥1 capturable: **free Ink** with ≥80% of samples inside **or** a **SmartGroup** with ≥80% of **natural area** inside. **0 content is allowed** when size ≥ 36 and the empty-shape gate passes. Nested capture + flatten: [SRS-EP-75](#srs-ep-75-nested-membership). Draw-into membership is **step 2** of [ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md) and runs **before** this step — a stroke that already qualifies as content of an existing Smart Group never reaches enclose |
| Guard — already grouped | Ink whose parent is already a `SmartGroup` is **skipped as free capture**; the **parent box** may be captured if ≥80% of its natural area is inside ([SRS-EP-75](#srs-ep-75-nested-membership)) |
| Commit | `create_smart_group` immediately — no proposal, no accept step. The enclose stroke becomes `role: boundary` ink; captured ink becomes `role: content` in group-local coordinates; `bounds` = fitted rect; **each content ink seeded with its own `layoutOffset` UV** |
| Guard fails | **Fall through** to new-connector (if open) then ordinary ink ([ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md) steps 4–5). No error state, no banner ([SRS-EP-12](./srs-ui.md)) |
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
| Select (rect) | `sel_rect` armed. Pen-down + move draws a thin dotted **axis-aligned rectangle** (rubber-band from down to tip). On pen-up: Ink **and Connector** if **≥80% of path samples** lie inside the rectangle ([BR-C11](../connector-ink/srs-product.md) / [REQ-09](../../prd.md#device-connectors)); other pickables if **≥80% of their world AABB area** lies inside. A grazing AABB intersect is **not** enough. |
| Select (freeform) | `sel_freeform` armed. Pen-down + move appends samples to a thin dotted **polyline**. On pen-up the polyline **closes** (edge last→first for the test; stored samples stay as drawn). Membership: Ink **and Connector** — ≥80% of path samples inside the closed polyline (even-odd); other nodes — ≥80% of a 5×5 grid on the world AABB inside. **Not** AABB-intersect of the gesture. Settled chrome is **not** the polyline — see next row. |
| Select (feedback, settled) | After **either** gesture: thin dotted **selection rect** = **tight** union AABB of selected nodes (**0** extra padding); **6 square anchors** (visual only this campaign). The freeform polyline is **gone** once settled. |
| Input for create | Free `Ink` in the selection (≥2, or ≥1 content-role ink + 1 candidate surround), **plus** selected Smart Groups. Nested boxes are **in** ([SRS-EP-75](#srs-ep-75-nested-membership)). Non-ink non-SG selected nodes are ignored by the surround algorithm (not captured). |
| Surround candidate | For each selected **free** ink `S`, build an **artificial closed path** if `S` is open (append edge first→last **for the test only** — never mutate stored samples). Point-in-polygon uses the **even-odd** fill rule. A candidate qualifies when ≥80% of the samples of **every other** selected free ink lie inside |
| Winner | The qualifying candidate; if several qualify, the highest paint/z order (later sibling) |
| Commit | Winner → `role: boundary`; other free ink → `role: content` in group-local coords; selected **non-empty** Smart Groups reparent as nested children (own-transform remapped into the new group’s local space); selected **empty** Smart Groups flatten ([SRS-EP-75](#srs-ep-75-nested-membership)); `bounds` = fitted AABB of the winner; each content ink seeded with its `layoutOffset` UV |
| Refuse | No qualifying surround → **do not create**; selection unchanged; reason visible ([SRS-EP-12](./srs-ui.md) refuse state). **SmartGroup in selection is not a refuse reason** |
| Undo | One entry |

There is no AABB-only Smart Group from a selection: a box always has boundary ink the creator drew.
Self-intersecting surrounds are accepted as-is under even-odd — no geometry clean-up this iter.

### Draw-into membership — inherited from [SRS-IN-15]

Runs at pen-up as **step 2** of [ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md)
(after endpoint-ink, **before** enclose). Not skipped merely because the stroke was closed-ish.

| Step | Rule |
|---|---|
| Trigger | A new `Ink` node after its samples are committed in world space |
| Candidates | Every `SmartGroup` whose **boundary ink** (even-odd interior of the world boundary polyline) contains ≥80% of the stroke’s **polyline length**. **Not** ≥80% of ink samples inside the group AABB. No boundary ink → the group does not qualify |
| None | Leave the ink at its ordinary parent (document root) |
| One | Reparent as `role: content` (samples → group-local); **seed that ink's `layoutOffset` UV** from its AABB centroid within the current bounds |
| Several (incl. nested) | Highest paint/z order wins — tree sibling order, later siblings win. **No dual parent**, no z-index field. Resolve via [SRS-EP-79](../device-document/srs-logic.md#srs-ep-79-geometry-queries) (nested SmartGroups are candidates; do not copy a top-level-only walk) |
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
| Pickable set (single press) | `SmartGroup` at **any nesting level**, resolved against world `bounds` after the composed outcome ([SRS-EP-76](#srs-ep-76-nested-render) / [SRS-EP-77](#srs-ep-77-nested-hit-reparent)) |
| Pickable set (rect marquee) | **Top-level** nodes only ([SRS-EP-77](#srs-ep-77-nested-hit-reparent) Rule 4). Hit = **≥80% inside** the rubber-band: Ink **and Connector** by **path samples** ([BR-C11](../connector-ink/srs-product.md)); other nodes by AABB-area overlap. Grazing AABB intersect does **not** select. **Not** ToolChip chrome. Nested SmartGroup children are **not** independently marquee-selected |
| Pickable set (freeform) | **Top-level** nodes only. Hit = **≥80% inside** the closed polyline (even-odd): Ink **and Connector** by path samples; other nodes by 5×5 AABB grid. Gesture AABB is **not** the hit-test. Connector AABB-only overlap does **not** select |
| Resolution order (tap) | **Children before ancestors.** Among siblings, later paint first. Deepest qualifying SmartGroup wins |
| Hit region | Inside `bounds`, plus a handle tolerance band when selected: visual **28 du**, hit **56 du** (14 du pad beyond visual). 1 du = 1 panel pixel @ 226 dpi. **Not** 8 CSS px |
| Source | The **local document** — never a peer-supplied list |
| Query implementation | Named API in [SRS-EP-79](../device-document/srs-logic.md#srs-ep-79-geometry-queries) ([ADR-0040](../../../../adr/ADR-0040-logarithmic-hit-test.md)). Product rules in this table **unchanged** |
| LOD cutoff | Applies **only when the viewport is zoomed out** (min panel/world scale **< 1.0**). Then picking is disabled if the box's **smaller on-panel axis < 96 du** (world→panel AABB). At scale **≥ 1.0** (identity map / 100% zoom), every SmartGroup is manipulable — enclose min-axis is 48 world, which is a legal box. **Not** `TILE_LOD_SCALE = 0.35` |
| Below cutoff | The press does nothing to the document, and the UI states that manipulation is unavailable ([SRS-EP-12](./srs-ui.md)) |

**Device constants (closed 2026-08-13).** Handle 28/56 du and LOD 96 du min-axis are accepted from
the [EP-012 Spec spike](../../../../../.plan/iter-003/design/device-selection-chrome/ui-spec.md#spike)
against known RM2 panel DPI. They are implement locks for EP-019. Do not fall back to 8 CSS px or
`TILE_LOD_SCALE = 0.35`. First-device miss-rate may file a `CHL-*` with a new **du** number — never
a desktop constant.

Below the cutoff the press **falls through to nothing** — not to pan **from this section**.
Finger empty-canvas pan is [SRS-EP-21](#srs-ep-21-one-finger) (**20 mm** / **178 du** @ 226 dpi), not this parent; **two-finger** pan is [SRS-EP-24](../region-sync/srs-logic.md#srs-ep-24-two-finger-viewport) ([REQ-10](../../prd.md#hand-touch)).
<!-- CHL-0022: do not treat this paragraph as “no on-device pan forever”; REQ-10 one-finger empty pan + two-finger are a later id. -->

### Gestures

| Gesture | Precondition | Result |
|---|---|---|
| Press inside SmartGroup bounds + drag | at/above LOD cutoff; not starting a marquee | **Move** — the real ink follows the pen; `set_smart_transform` translate on release |
| Press, no drag | at/above LOD cutoff on a SmartGroup | **Select** — handles appear on geometric `bounds` |
| Pen-down + move (marquee) | `selection` tool; press not on a SmartGroup handle / not claiming a move | **Rubber-band** — thin dotted AABB follows pen tip; on pen-up, select intersecting document nodes; show selection rect + **6** anchors |
| Drag a handle | SmartGroup selected (manipulation chrome) | **Resize** — bounds follow the handle; `withBounds` scales content, `fixedInk` preserves each content ink's UV (draw rule above) |
| Toggle `inkScaleMode` | SmartGroup selected | `set_ink_scale_mode` |
| Tap `cta.enclose` | Selection non-empty | Run selection-create ([SRS-EP-10](./srs-logic.md#srs-ep-10-device-recognition)); refuse path if no surround (**not** if a SmartGroup is in the set) |
| Press empty canvas | — | Deselect; **0** residual chrome on the next settled frame (CHL-0007) |
| Press another pickable | — | Selection moves to that node |

### Tap vs travel (SelectionMode) {#srs-ep-11-hold-still}

**Parent also:** [REQ-12](../../prd.md#clipboard) / [SRS-EP-32](./srs-ui.md#srs-ep-32-clipboard-ui). **Decision:** [ADR-0037](../../../../adr/ADR-0037-device-clipboard-singleton.md). Applies in **SelectionMode** (`sel_rect` / `sel_freeform`) for Primary **and** Secondary. Does not apply in InkMode / EraserMode.

A down may **lock** Move / Lasso / Marquee / Resize as today, but the locked op **must not mutate the document** until classified. **No** 500 ms hold menu ([CHL-0031](../../../../../.plan/iter-005/challenges/CHL-0031-clipboard-tap-paste.md)).

| Classifier | Panel travel | Result |
|---|---|---|
| Travel | **> 1 mm** (Euclidean, panel px; 1 mm ≈ 8.9 du @ 226 dpi) | Begin the locked Move / lasso / marquee (or resize if a knob). This is what stops tap-nudge. |
| Tap | **≤ 1 mm**, then lift | **Select** the hit. If empty: **deselect only** when a selection existed; record paste origin **only** when selection was already idle. Primary (stylus) **and** Secondary (finger). Pose of the hit node is **unchanged**. |

Camera pan/zoom and a real lasso/marquee clear the paste origin ([SRS-EP-32](./srs-ui.md#srs-ep-32-clipboard-ui)).

**Out of scope:** rotation handles and connector attachment to a `SmartGroup`. Anchor **events** on
the 6 selection anchors (drag-to-resize the marquee selection) — later; chrome is visual only for
EP-018. Anchor resolution for connectors lands in [REQ-08](../../prd.md#node-manipulation).

### Live manipulation — the rule the pilot did not have

| Rule | Value |
|---|---|
| What moves | The **document's** ink, transformed live. There is no advisory outline that a peer later corrects |
| Paint during the gesture | **ToolCanvasLayer** draws the live node **and bound connector spines**; CanvasLayer hides the origin box **and those spines** ([CHL-0018](../../../../../.plan/iter-003/challenges/CHL-0018-live-node-tool-canvas.md), [BR-B19](./srs-product.md)). Suppress ids without punching the spine AABB leaves origin connector pixels. Not a second document, not a ghost |
| Commit | On release, the committed transform **equals** the last previewed transform. 0 px jump, 0 snap-back. Settled raster returns to CanvasLayer: InPlaceDirty of **origin ∪ live box ∪ origin and live connector spines**. A box-only dirty rect misses the middle of the spine and leaves the origin connector. |
| Op granularity | **One** `set_smart_transform` per completed gesture, not per frame — intermediate frames are local paint only |
| Refresh | Partial refresh only during the gesture; **0** full-panel invalidations ([SRS-EP-02](../region-sync/srs-logic.md)) |
| Feedback rate | ≥5 Hz; no stall >200 ms. Slow is acceptable, wrong is not (CHL-0006) |
| Interruption | An inbound `doc_load` never interrupts a gesture — it defers ([SRS-EP-08](../device-document/srs-logic.md)) |
| Undo | One entry per gesture. Undo/redo of a bound-node drag **re-warps** attached connectors (derived; not an undo target) and InPlaceDirty **pre ∪ post box ∪ both spines**. Box-only dirty leaves the post-move spine and misses the restored spine |

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

## [SRS-EP-21] One-finger pick and move {#srs-ep-21-one-finger}

<!-- lifecycle: active -->

**Parent:** [REQ-10](../../prd.md#hand-touch). **Links (not parents):** [SRS-EP-11](#srs-ep-11-device-manipulation) live-direct move, [SRS-EP-04](../tool-modes/srs-logic.md) exclusive tools, [SRS-EP-23](../tool-modes/srs-logic.md#srs-ep-23-finger-tool-switch), [SRS-EP-49](../region-sync/srs-logic.md#srs-ep-49-viewport-follow). **Decision:** [ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md) (one-finger does **not** publish unless Infini is following; box-move never pans).

Does **not** overload SRS-EP-11. Pen pick/move stays there. This section is **finger** only.

### Hit rules (closed)

| Pointer | Target | Eligible? |
|---|---|---|
| Finger | SmartGroup world `bounds` at/above LOD ([SRS-EP-11](#srs-ep-11-device-manipulation) 96 du min-axis when scale &lt; 1) | **Yes** — pick + move |
| Finger | Resize knobs (hit ≥ primary ToolChip tile) | **Yes** — resize with [SRS-EP-11](#srs-ep-11-device-manipulation) live-direct; knob wins over box-move |
| Finger | Any other control whose **hit target &lt; 64 du** | **No** — 0 transform, 0 scale-mode, 0 end-kind. Pen still may |
| Finger | ToolChip primary tile (64 du) | Chip wins — this REQ does not steal ([SRS-EP-05](../tool-modes/srs-ui.md)) |
| Finger | Empty canvas (no box, knob, or chip hit), one finger | Palm vs pan by travel — not this table’s job. See Gesture: ≤ **20 mm** tap deselects; > **20 mm** local pan |
| Two fingers | — | Not this section — [SRS-EP-24](../region-sync/srs-logic.md#srs-ep-24-two-finger-viewport) |

1 du = 1 panel pixel @ 226 dpi. Finger-eligible floor = primary ToolChip tile **64×64 du** ([CHL-0019](../../../../../.plan/iter-004/challenges/CHL-0019-toolchip-tile-size.md)).

**Pan threshold (architect bind, [REQ-10](../../prd.md#hand-touch)):** **20 mm** Euclidean panel travel from finger-down (**178 du** @ 226 dpi). At/below = tap: **deselect**, **0** pan, **0** tool switch. Past = **local** one-finger pan. Box / knob / chip hit on the same down **wins** (0 empty-canvas pan). **≥3** simultaneous contacts = palm (0 pan, 0 pinch).

**Palm rejection (architect bind, [REQ-10](../../prd.md#hand-touch)):** `handTouch.enabled = toggle.on and not (pen.near or pen.contact) and contactCount < 3`. Near = digitizer proximity (hover before contact). Contact = pen down on the panel. Toggle default **on**. While disabled: 0 canvas pick / move / empty pan / pinch. Chrome taps (≥64 du) still route. Enable again when the pen leaves proximity (or after a short idle if the stack omits leave-proximity) **and** the toggle is on.

### Gesture

| Step | Rule |
|---|---|
| Finger-down inside a hittable SmartGroup | Select that box; exclusive tool → `sel_freeform` ([SRS-EP-23](../tool-modes/srs-logic.md#srs-ep-23-finger-tool-switch)); chip updates with p95 ≤300 ms |
| Same down, or following finger drag, inside the selected box | **Move** with [REQ-06](../../prd.md#device-manipulation) live-direct contract: 0 px jump on lift; ≥5 Hz partial refresh; ToolCanvasLayer live node ([SRS-EP-11](#srs-ep-11-device-manipulation)) |
| Finger-down on a resize knob of the selected box | **Resize** with the same live-direct contract as pen on that knob (ink-scale mode applies). Knob hit wins over box-move. |
| During this move or resize | **0** viewport pan; two-finger does not start; follow unchanged (box-move is **not** local-nav) |
| Finger-down on a &lt;64 du control that is **not** a resize knob | No-op (table above) |
| Already selected box, finger-down inside bounds (not on a knob) | May start the move on that same down |
| Empty canvas, travel ≤ 20 mm, lift | Tap: **deselect** (0 nodes, 0 residual chrome); **0** pan; **0** tool switch |
| Pen near or in contact, or hand-touch toggle off | Hand-touch **off** — ignore canvas fingers until the pen leaves proximity and the toggle is on |
| ≥3 capacitive contacts | Palm: **0** pan, **0** pinch |
| Empty canvas, first sample with travel > 20 mm | **Local pan** (exclusive tool unchanged; 0 nodes selected; 0 lasso) **unless** Epaper follow is on (`infini_to_epaper`): then **0** pan, follow stays on ([SRS-EP-49](../region-sync/srs-logic.md#srs-ep-49-viewport-follow)). Publish `viewport` up **only if** Infini follow is on |
| Empty canvas, box/knob/chip hit on the same down | Hit wins — **0** empty-canvas pan |

### UI-driving fields

| Field | Drives |
|---|---|
| `touch.fingerCount` | 1 vs 2 vs ≥3 (2 → not this section; ≥3 → palm) |
| `pen.near` / `pen.contact` / `handTouch.toggle` | Any disable → hand-touch off |
| `toolMode` | Must become `sel_freeform` on box hit; **unchanged** on empty pan |
| `follow.direction` | Pan/pinch while following Infini → **blocked** (follow stays on) |
| `sel.moving` | Live-direct chrome ([SRS-EP-22](./srs-ui.md#srs-ep-22-hand-touch-ui)) |
| `sel.resizing.*` | Live-direct resize chrome when `hit.kind=anchor` |

### Out of scope

Finger rotation, connector re-anchor ([REQ-08](../../prd.md#node-manipulation)). Two-finger pan. REQ-15 tables.

---

## [SRS-EP-75] Nested membership, flatten, enclose capture {#srs-ep-75-nested-membership}

<!-- lifecycle: active -->

**Parent:** [REQ-05](../../prd.md#device-ink-box). **Product:** [BR-B20, BR-B21](./srs-product.md). **Decision:** [ADR-0039](../../../../adr/ADR-0039-nested-ink-box-rendering.md). **Quality:** [SRS-EP-14](./srs-quality.md).

Applies to **Creation A** (recognizer) and **Creation B** (`cta.enclose`). Clipboard paste-into-box uses the same flatten rule.

### Empty

A SmartGroup is **empty** iff every child is `Ink` with `role: boundary` (no `role: content` Ink, no nested SmartGroup).

### Flatten (Rule 1)

When an empty SmartGroup would become a child of another SmartGroup (enclose capture, Enclose CTA, or paste parent):

1. Let `own` be that child’s own-transform affine ([SRS-EP-76](#srs-ep-76-nested-render)).
2. Apply `own` to its boundary-ink samples (and boundary polyline). If the box was resized/rotated, that bake **is** the transform — do not keep a wrapper.
3. Insert the resulting Ink as `role: content` of the **parent**, seed `layoutOffset` from the baked AABB centroid vs parent bounds.
4. Remove the empty SmartGroup in the **same** gesture (one undo).

Do **not** flatten a non-empty box. Do **not** flatten a top-level empty box that is not being captured (letter-as-box stays a box until enclose/paste-into-parent).

### Capture set (Creation A)

Walk **top-level** siblings of the enclose stroke (same as today for free ink). Candidates:

| Kind | Capturable when |
|---|---|
| Free `Ink` (parent is not SmartGroup) | ≥80% of **samples** inside the enclose fitted AABB (unchanged) |
| Top-level `SmartGroup` | ≥80% of **natural area** inside the enclose fitted AABB (or inside the enclose stroke’s even-odd region — same region as the ink test; AABB is the fitted rect for Creation A) |

Ink already inside a SmartGroup is **not** free-captured; capturing the parent box takes the subtree.

Captured non-empty SmartGroup: `detachAny`, remap its **own-transform translate** into the new group’s local space (`tx' = tx − newWorldX`, same for y; scale/rotation unchanged), append as a child. Captured empty SmartGroup: flatten into the new group.

### Capture set (Creation B)

Surround test stays **free ink only**. Selected Smart Groups are additional capture (not surround candidates). Same flatten / remap as Creation A. `smartgroup_in_selection` is **retired**.

### CreateSmartGroupEdit

`captureIds` may name Ink **or** SmartGroup. Apply uses `detachAny`. Children array may include nested SmartGroup nodes. `insertUnder` still rejects SmartGroup as the **new group’s** parent (new groups stay document-root unless a later reparent). Nested **content** is in `children`, not via `insertUnder`.

---

## [SRS-EP-76] Nested rendering (RenderingContext) {#srs-ep-76-nested-render}

<!-- lifecycle: active -->

**Parent:** [REQ-06](../../prd.md#device-manipulation). **Product:** [BR-B22](./srs-product.md). **Decision:** [ADR-0039](../../../../adr/ADR-0039-nested-ink-box-rendering.md).

### Affine

`Affine { a, b, c, d, e, f }` maps `(x,y) → (a x + c y + e, b x + d y + f)`. `compose(P, Q)` applies **Q then P**. Identity `(1,0, 0,1, 0,0)`.

`own` of a SmartGroup, local origin = bounds origin (0,0) as shipped:

1. Scale `(scaleX, scaleY)`
2. Rotate `rotation` (radians; usually 0 this campaign)
3. Translate `(transform.x, transform.y)`

### Walk

```
paint(node, ctx):
  if SmartGroup:
    outcome = compose(ctx.transform, own(node))
    emit each boundary-role Ink with outcome
    contentCtx = inkScaleMode == withBounds ? outcome
                : compose(ctx.transform, TR_without_S(node))
    for child in children (paint order):
      paint(child, { transform: contentCtx })
  else if Ink / Primitive:
    emit with ctx.transform (and fixedInk UV on content Ink as [SRS-EP-10])
  else:
    emit world as today; recurse Frame/Group with identity (they stay world-space)
```

World call: `paint(rootChildren, { identity })`.

`smartLocalToWorld` for a **top-level** group is `paint` with identity ctx (bit-compatible). Nested groups **must** receive the parent’s `contentCtx` — passing only the immediate node (status quo `walkFlat(..., &node)`) is a defect ([CHL-0032](../../../../../.plan/iter-005/challenges/CHL-0032-nested-ink-box.md)).

Camera / viewport mapping is **after** world: composed world points × panel projector. Nested children stay correct when the camera changes **and** when an ancestor moves.

### Content clip (natural world AABB)

Entering a SmartGroup, **content** (content-role Ink / Primitive, nested SmartGroups, and their descendants) is clipped to the intersection of:

1. ancestor content clips, and
2. this group’s **natural world AABB** — axis-aligned hull of the four local `bounds` corners after `outcome` (`smartGroupWorldBounds`).

This is **not** even-odd of boundary ink. Boundary-role ink of **this** group is clipped only by ancestor clips (not by its own AABB). Live overlay uses the same clip, including ancestor AABBs when painting a nested subtree.

Overflow past that AABB is **not painted**. Dirty punch and hierarchy cull stay on the natural AABB; do **not** union descendant ink AABBs.

Live overlay (ToolCanvasLayer) uses the same compose for the selected node’s subtree.

---

## [SRS-EP-77] Nested tap-hit and move reparent {#srs-ep-77-nested-hit-reparent}

<!-- lifecycle: active -->

**Parent:** [REQ-06](../../prd.md#device-manipulation) (finger: [REQ-10](../../prd.md#hand-touch) uses the same tap walk). **Product:** [BR-B23, BR-B24, BR-B25](./srs-product.md). **Decision:** [ADR-0039](../../../../adr/ADR-0039-nested-ink-box-rendering.md). **Chrome:** [SRS-EP-12](./srs-ui.md) — no new inventory.

### Tap (Rule 3)

Walk SmartGroups **depth-first**. At each level, test later siblings first. If the press is **outside** a group’s natural world AABB, **skip that subtree** (overflow is not hittable — same clip as paint, [SRS-EP-76](#srs-ep-76-nested-render)). If the press is inside, a child’s world `bounds` (composed outcome, LOD as [SRS-EP-11](#srs-ep-11-device-manipulation)) that contains the press **wins over** its ancestors. Child ink of a SmartGroup is still **not** independently selected — the containing SmartGroup at that level is.

Hit-test of nested bounds uses the same affine as paint ([SRS-EP-76](#srs-ep-76-nested-render)).
Resolve through [SRS-EP-79](../device-document/srs-logic.md#srs-ep-79-geometry-queries) (index
cull + paint-rank among candidates). Children-before-ancestors is **not** AABB-max.

Selected node’s move/resize writes **that node’s own-transform / bounds only**. Context toolbar (`cta.copy` / `cta.cut` / `cta.paste` / `cta.enclose` / `tgl.ink_scale_mode`) is for **that** id.

### Marquee / freeform (Rule 4)

`collectPickable` stays **top-level** (do not recurse into SmartGroup). Nested boxes are tap-only. Child ink under a SmartGroup remains not independently lassoed. **Connector** is a top-level pickable: marquee/freeform uses ≥80% **path samples**, not AABB ([BR-C11](../connector-ink/srs-product.md)); pen-down selects on the **stroke** (AABB press does not).

### Reparent at move commit (Rule 5)

After a **move** (not resize) commits:

1. Compute the moving node’s **natural area** in world (SmartGroup: area of local `bounds` after full outcome — hull of the four corners; Ink: sample AABB area).
2. Candidates = SmartGroup / Frame / Group **excluding** the moving node and its descendants.
3. A candidate qualifies if ≥**80%** of that natural area lies in:
   - SmartGroup: even-odd interior of **world** boundary ink (same as draw-into)
   - Frame / Group: world AABB
4. Winner = highest **paint order** (later sibling among the deepest-vs-later walk: prefer the qualifying node that paints above). Else parent = document root (`parentId` empty).
5. If winner ≠ current parent: `reparent` + remap own-transform into the new parent’s **content** local space (inverse of that parent’s content-outcome). One undo with the move.

Not during the drag. Not on resize. Nested finger-move uses the same commit hook.
Resolve through [SRS-EP-79](../device-document/srs-logic.md#srs-ep-79-geometry-queries)
highest-paint-container query; exact ≥80% on the candidate set.

### Finger

[SRS-EP-21](#srs-ep-21-one-finger) “finger-down inside a hittable SmartGroup” uses this tap walk (deepest box), not top-level `collectPickable`.

---

## Superseded

New sections. They inherit, on the device, infini
[SRS-IN-10](../../../infini/features/vector-document/srs-logic.md#srs-in-10),
[SRS-IN-15](../../../infini/features/vector-document/srs-logic.md#srs-in-15-draw-into-membership),
[SRS-IN-16](../../../infini/features/vector-document/srs-logic.md#srs-in-16-selection-create-surround),
and [SRS-IN-11](../../../infini/features/vector-document/srs-logic.md#srs-in-11-selection-manipulation)
— all deprecated 2026-08-13 under CHL-0008. See the
[lifecycle map](../../../../../.plan/iter-003/lifecycle-map-2026-08-13.md).
