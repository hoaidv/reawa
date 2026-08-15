---
feature: vector-document
parent_req: [REQ-02, REQ-04]
version: 0.3.0
lifecycle: active
owner: pm
---

# SRS — Vector document (Product)

PM feature depth for [REQ-02](../../prd.md#vector-document). Complements
[ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) (structure) without owning wire shapes.

## Intent / JTBD

When a creator draws mostly by hand on Epaper and organises ideas on Infini, they need **one
document** that holds ink, typed text, shapes, groups, frames, and connectors — openable,
saveable, and syncable — not a pile of disconnected strokes.

## In scope / out of scope (feature grain)

| In scope | Out of scope |
|---|---|
| Tree-of-vectors model (ink, text, primitives, groups, frames, connectors) | Full illustration suite / brush engine |
| **Smart Group / ink-box pilot** — tool-armed enclose **or** explicit selection; draw-into membership; free layout; move/resize; scale modes | OCR to Text; perfect recognition; unprompted grouping; rotation; connectors on a Smart Group; **in-box alignment** |
| Open / save / new / dirty / error chrome | Cloud multi-user CRDT productization |
| Persistence round-trip + transmit op round-trip | On-device document browser on RM |
| Group anywhere; frames at root | Nested frames; component instances / variants |
| Connector links between nodes | Auto-layout, constraints engines |

## Business rules / eligibility / policy

| Rule id | Statement (product language) | Notes |
|---|---|---|
| BR-01 | Handwriting is first-class: most content is freehand ink, not shapes. Each ink sample keeps **position plus any tablet-reported ink channels** (pressure, tilt, …). | Polyline of samples |
| BR-02 | Users may **group** related vectors into a Group that can sit anywhere in the tree. | Composite |
| BR-03 | Users may place **Frames** only at the document root to cluster work areas. | Like Figma frames, simpler |
| BR-04 | Users may add **paragraph text** via desktop text boxes in world space. | |
| BR-05 | Users may add or keep **primitive shapes** (line, rect, ellipse) alongside ink. | Recognition may feed this later |
| BR-06 | Users may add **connectors** between two existing nodes (shape↔shape, shape↔group, frame↔frame, text↔…). | |
| BR-06a | Connector endpoints attach to the **boundary** of a node. For rects/squares, prefer the **4 edge midpoints**; any point on the 4 edges is allowed. For circles/ellipses, prefer **top/right/bottom/left**; any point on the circumference is allowed. | Snap vs free |
| BR-09 | **Smart Group (pilot):** enclose (or explicit promote) → ink-box; content ink preserved (no OCR); **boundary ink preserved and always transforms** with the group; geometric `bounds` = initial (x,y,w,h); connector target; `inkScaleMode` `withBounds` \| `fixedInk` applies to **content ink only**. | [REQ-04](../../prd.md#smart-group) · ADR-0011 |
| BR-09a | **Ink-box is never created unprompted.** Creation requires either the `Ink-box` tool armed when the enclosing stroke is drawn, or an explicit Smart Group command on a selection. A rectangle drawn in `Pen` mode is ordinary ink. | Rejects auto-recognition |
| BR-09b | **Enclose guards.** An armed enclose creates a Smart Group only when the fitted rect is ≥ a fixed minimum size **and** contains ≥1 ink (≥80% of that ink's samples inside). Otherwise the stroke stays ordinary ink, with no error state. | Rect-only in pilot |
| BR-09c | **Ink-box captures any ink** inside the enclosure — handwriting, sketch, or shape. There is no "is this text?" test. | Guards the no-OCR line |
| BR-09d | **A box always reads as a box.** Successful create always has `role: boundary` ink (enclose stroke or the surround stroke found in a selection). Never a synthetic ink rectangle. | Supersedes “hint-only selection create” |
| BR-09e | **Tool state is per device**, never synced. **Infini this campaign has no editing ToolStrip** ([REQ-04](../../prd.md#smart-group) deprecated). Epaper: exclusive `sel_rect` · `sel_freeform` · `pen` plus recognizer toggles ([epaper REQ-03](../../../epaper/prd.md#tool-modes)). | Modal, device-local |
| BR-09f | **Manipulation pilot scope** is move + resize + `inkScaleMode` toggle. Rotation and connector attachment to a Smart Group are **out of pilot**. | Anchors are translate+scale only |
| BR-09g | **Draw-into membership.** A new `Pen` stroke whose samples are ≥80% inside one or more Smart Group world bounds becomes `role: content` of the qualifying box with the **highest paint/z order** (tree sibling order; later paints above). Nested boxes that each contain 100% of the stroke resolve the same way — no separate z-index field. | Runs on Infini at `stroke_end` |
| BR-09h | **Free layout inside the box.** Appending content ink never reflows, realigns, or shifts existing children. Placement is as drawn. In-box alignment is deferred. | |
| BR-09i | **`fixedInk` per-ink offset.** Each `role: content` ink tracks **its own** relative offset / UV in the box. On bounds/scale change under `fixedInk`, adjust each ink independently so its UV is preserved and sample size stays fixed — a newly drawn stroke never moves older content. `withBounds` scales content with the box. Boundary ink always transforms. | Preferred impl model for architect/dev |
| BR-09j | **Selection create needs a surround stroke.** Among the selected inks, one stroke must contain ≥80% of each other selected stroke’s samples (open stroke OK — test via artificial closed path). That stroke → `boundary`; others → `content`. If none qualify → **cannot create**. | Solution 3 guard |
| BR-07 | Unsaved edits mark the document **dirty**; failed open/parse must not wipe the on-screen tree. | doc.* states |
| BR-08 | Deleting a node that a connector references must not crash; the connector **stays** and that end uses last live pose until undo restores the node ([ADR-0020](../../../../adr/ADR-0020-connector-ink-geometry.md)). | REQ-09 D39 |

### Edge cases

| Case | Expected product behavior |
|---|---|
| Open corrupt / unsupported SVG | `doc.error`; previous tree unchanged if one was open; empty stays empty |
| Save with invalid connectors | Save still succeeds; invalid connectors flagged in chrome or stripped with user-visible notice (v0: flag, keep geometry path) |
| Empty new document | `doc.open` + clean; canvas may show empty world |
| Group with zero children | Disallowed on commit; empty group rejected |
| Frame with children moved out | Frame may remain empty (allowed) |
| Enclose stroke encloses nothing | No Smart Group; stroke stays ordinary ink; no error banner |
| Enclose stroke below minimum size | Same as above — treated as ordinary ink (protects small annotations) |
| Enclose stroke drawn in `Pen` mode | Ordinary ink, always — never grouped retroactively |
| Enclose encloses ink already inside another Smart Group | v0: capture is skipped for ink that already has a Smart Group parent; the rest is captured |
| New Pen stroke ≥80% inside one Smart Group | Stroke reparented as `content`; existing children unmoved |
| New Pen stroke ≥80% inside several nested Smart Groups | Parents under the highest paint/z-order qualifier; never dual-parented |
| New Pen stroke <80% inside every Smart Group | Remains root (or ordinary parent) ink — no membership |
| Selection create without a surround stroke | Refuse create; selection unchanged; clear reason (no AABB-only Smart Group) |
| Selection create with open surround stroke | Artificial closed path used for ≥80% test; open stroke preserved as `boundary` samples |
| `fixedInk` resize with multiple content inks | Each ink’s own UV/offset preserved; no cross-ink translation |
| Manipulation attempted below LOD (scale < 0.35) | Selection/move/resize unavailable; drag pans; UI states it is unavailable rather than silently panning |
| Smart Group left with zero children after an edit | Disallowed; last child removal removes the Smart Group and restores ink to the parent |

### Acceptance (drives BDD / stories)

- Given a document tree with ink, a group of ink, a root frame, a text box, a rect, and a connector, When the user saves and reopens, Then all nodes reappear with the same ids, parenting, and geometry (±1 px @ 100% zoom).
- Given `doc.open` with edits, When the user has not saved, Then chrome shows dirty; After save, dirty clears.
- Given open fails, When parse errors, Then `doc.error` and the prior document (if any) remains on canvas.
- Given two nodes, When the user creates a connector between them, Then the connector references both ids and renders a link path attached to each node’s **boundary** (preferred port or free edge/circumference point).
- Given a rect with a connector on `port: east`, When the rect moves or resizes, Then the connector endpoint stays on the east-edge midpoint of the new bounds.
- Given handwritten ink, When the user encloses it with a rectangle (or explicit Smart Group), Then a Smart Group holds that content ink unaltered, **keeps the enclose stroke as boundary ink that always transforms with the group**, stores geometric bounds from recognition, and can be connector-targeted on those bounds; `fixedInk` keeps **content** ink sample size fixed while bounds (and boundary ink) still transform.
- Given the `Pen` tool, When the user draws a rectangle around existing handwriting, Then **no** Smart Group is created and both strokes remain ordinary ink.
- Given the `Ink-box` tool and an enclosure that passes both guards, When the stroke ends, Then a Smart Group is created within 300 ms and a single undo restores the previous tree.
- Given a Smart Group created from a selection that includes a surround stroke, When create succeeds, Then that stroke is `boundary` and the others are `content`.
- Given a selection with no surround stroke at the ≥80% bar, When the user invokes Smart Group, Then nothing is created and the selection is unchanged.
- Given a Smart Group, When the user drags inside its bounds at scale ≥0.35, Then it moves and the canvas does not pan.
- Given a Smart Group and the `Pen` tool, When the user draws a stroke ≥80% inside the box, Then the stroke becomes content of that box and no existing content is shifted.
- Given nested Smart Groups both containing a new Pen stroke at ≥80%, When the stroke ends, Then membership goes to the highest paint/z-order box.
- Given `inkScaleMode=fixedInk` and two content inks with distinct UVs, When the box scales by `s`, Then each ink keeps its own UV (±1 px @ 100% zoom) and sample sizes are unchanged.

### Implemented via

| Concern | Pointer |
|---|---|
| Tree + ops + three representations | [srs-logic](./srs-logic.md) · [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) |
| Smart Group | [SRS-IN-10](./srs-logic.md) · [ADR-0011](../../../../adr/ADR-0011-smart-group.md) · [REQ-04](../../prd.md#smart-group) |
| Schema | [srs-data](./srs-data.md) |
| Open/save chrome | [srs-ui](./srs-ui.md) |
| Journeys | [srs-experience](./srs-experience.md) |
| Fidelity / dual-ask | [srs-quality](./srs-quality.md) |
