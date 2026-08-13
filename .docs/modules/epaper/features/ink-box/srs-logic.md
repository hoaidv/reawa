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
armed tool ([SRS-EP-04](../tool-modes/srs-logic.md)) at **pen-down**, latches it for the stroke, and
dispatches at pen-up.

| Armed at pen-down | Pen-up path |
|---|---|
| `ink_box` | Enclose evaluation (below). Guards fail → the stroke stays ordinary ink |
| `pen` | Draw-into membership evaluation (below) |
| `selection` | No ink was produced; not an ingestion path at all |

Latching at pen-down matters: switching tools mid-stroke must not retroactively change what the
stroke means.

### Enclose recognition — inherited from [SRS-IN-10]

| Step | Rule |
|---|---|
| Trigger | Pen-up of a stroke drawn with `ink_box` armed (**changed** — was `intent: enclose` on the wire) |
| Candidate shape | Closed or near-closed polyline fitting an axis-aligned rect; **rectangle only** |
| Fitted bounds | AABB of the enclose stroke samples → `(x, y, width, height)` |
| Guard — size | Shorter side ≥ `MIN_ENCLOSE_WORLD` = **48 world units** ([ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) §6, kept by ADR-0014 §7) |
| Guard — content | ≥1 ink with **≥80%** of its samples inside the fitted rect |
| Guard — already grouped | Ink whose parent is already a `SmartGroup` is **skipped**; remaining ink still captures |
| Commit | `create_smart_group` immediately — no proposal, no accept step. The enclose stroke becomes `role: boundary` ink; captured ink becomes `role: content` in group-local coordinates; `bounds` = fitted rect; **each content ink seeded with its own `layoutOffset` UV** |
| Guard fails | **No-op** — the stroke stays ordinary ink. No error state, no banner ([SRS-EP-12](./srs-ui.md)) |
| Undo | One entry; one undo restores the pre-op tree exactly ([SRS-EP-07](../device-document/srs-logic.md)) |
| Visibility | The box is on the panel **p95 ≤500 ms after pen-up**, with 0 peer messages required ([SRS-EP-14](./srs-quality.md)) |

Recognition is internal: `recognize_enclose` is not an op and never appears on the wire. Only its
outcome — `create_smart_group` — is published.

### Selection create, surround required — inherited from [SRS-IN-16]

| Step | Rule |
|---|---|
| Input | ≥2 selected `Ink` nodes (or ≥1 content + 1 candidate surround) |
| Surround candidate | For each selected stroke `S`, build an **artificial closed path** if `S` is open (append edge first→last **for the test only** — never mutate stored samples). Point-in-polygon uses the **even-odd** fill rule. A candidate qualifies when ≥80% of the samples of **every other** selected ink lie inside |
| Winner | The qualifying candidate; if several qualify, the highest paint/z order (later sibling) |
| Commit | Winner → `role: boundary`; others → `role: content` in group-local coords; `bounds` = fitted AABB of the winner; each content ink seeded with its `layoutOffset` UV |
| Refuse | No qualifying surround → **do not create**; selection unchanged; the reason is visible ([SRS-EP-12](./srs-ui.md) refuse state) |
| Undo | One entry |

There is no AABB-only Smart Group from a selection: a box always has boundary ink the creator drew.
Self-intersecting surrounds are accepted as-is under even-odd — no geometry clean-up this iter.

### Draw-into membership — inherited from [SRS-IN-15]

Runs at pen-up for ordinary ink (`pen` armed). **Never** runs on an enclose stroke.

| Step | Rule |
|---|---|
| Trigger | A new `Ink` node after its samples are committed in world space |
| Candidates | Every `SmartGroup` whose **world** `bounds` contain ≥80% of the stroke's samples |
| None | Leave the ink at its ordinary parent (document root) |
| One | Reparent as `role: content` (samples → group-local); **seed that ink's `layoutOffset` UV** from its AABB centroid within the current bounds |
| Several (incl. nested) | Highest paint/z order wins — tree sibling order, later siblings win. **No dual parent**, no z-index field |
| Layout | Do **not** translate, scale, or reflow any existing content ink; new ink stays as drawn |
| Bounds | `SmartGroup` bounds are **not** expanded by membership |
| Undo | One entry |

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
| Pickable set | `SmartGroup` nodes, resolved against world `bounds` after transform |
| Resolution order | Topmost first — later siblings paint above, so they pick first |
| Hit region | Inside `bounds`, plus a handle tolerance band when selected (device value below) |
| Source | The **local document** — never a peer-supplied list |
| LOD cutoff | Picking disabled below a device scale threshold (**open**, below) |
| Below cutoff | The press does nothing to the document, and the UI states that manipulation is unavailable ([SRS-EP-12](./srs-ui.md)) |

**Open — device constants.** `TILE_LOD_SCALE = 0.35` and the 8 CSS px tolerance band are *desktop*
values from [SRS-IN-11]. The device has a fixed panel, no CSS pixel, and a pen with a different
pointing error than a mouse. Both must be re-derived in device units against the panel DPI before
the first manipulation story. Owner: architect + design spike
([SRS-EP-12](./srs-ui.md), handoff ask).

Below the cutoff the press **falls through to nothing** — not to pan, because there is no on-device
pan ([epaper Non-Goals](../../prd.md)). That is the one line of [SRS-IN-11] that could not be
inherited as written.

### Gestures

| Gesture | Precondition | Result |
|---|---|---|
| Press inside bounds + drag | at/above LOD cutoff | **Move** — the real ink follows the pen; `set_smart_transform` translate on release |
| Press, no drag | at/above LOD cutoff | **Select** — handles appear on geometric `bounds` |
| Drag a handle | node selected | **Resize** — bounds follow the handle; `withBounds` scales content, `fixedInk` preserves each content ink's UV (draw rule above) |
| Toggle `inkScaleMode` | node selected | `set_ink_scale_mode` |
| Press empty canvas | — | Deselect; **0** residual chrome on the next settled frame (CHL-0007) |
| Press another pickable | — | Selection moves to that node |

**Out of scope:** rotation handles and connector attachment to a `SmartGroup`. Anchor resolution
computes a world AABB for translate + scale only, so exposing rotation would resolve connector ports
incorrectly — the anchor math lands first, in [REQ-08](../../prd.md#node-manipulation).

### Live manipulation — the rule the pilot did not have

| Rule | Value |
|---|---|
| What moves | The **document's** ink, transformed live. There is no ghost, no outline stand-in, no advisory layer |
| Commit | On release, the committed transform **equals** the last previewed transform. 0 px jump, 0 snap-back |
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
| Selection model | One selection holder, node-kind agnostic; multi-select is absent, not architecturally excluded |
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
