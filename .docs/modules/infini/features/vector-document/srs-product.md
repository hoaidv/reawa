---
feature: vector-document
parent_req: [REQ-02]
version: 0.2.0
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
| **Smart Group / ink-box pilot** (enclose or explicit; ink preserved; scale modes) | OCR to Text; perfect recognition |
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
| BR-07 | Unsaved edits mark the document **dirty**; failed open/parse must not wipe the on-screen tree. | doc.* states |
| BR-08 | Deleting a node that a connector references must not crash; connector becomes invalid and is shown as error or removed per edge policy below. | |

### Edge cases

| Case | Expected product behavior |
|---|---|
| Open corrupt / unsupported SVG | `doc.error`; previous tree unchanged if one was open; empty stays empty |
| Save with invalid connectors | Save still succeeds; invalid connectors flagged in chrome or stripped with user-visible notice (v0: flag, keep geometry path) |
| Empty new document | `doc.open` + clean; canvas may show empty world |
| Group with zero children | Disallowed on commit; empty group rejected |
| Frame with children moved out | Frame may remain empty (allowed) |

### Acceptance (drives BDD / stories)

- Given a document tree with ink, a group of ink, a root frame, a text box, a rect, and a connector, When the user saves and reopens, Then all nodes reappear with the same ids, parenting, and geometry (±1 px @ 100% zoom).
- Given `doc.open` with edits, When the user has not saved, Then chrome shows dirty; After save, dirty clears.
- Given open fails, When parse errors, Then `doc.error` and the prior document (if any) remains on canvas.
- Given two nodes, When the user creates a connector between them, Then the connector references both ids and renders a link path attached to each node’s **boundary** (preferred port or free edge/circumference point).
- Given a rect with a connector on `port: east`, When the rect moves or resizes, Then the connector endpoint stays on the east-edge midpoint of the new bounds.
- Given handwritten ink, When the user encloses it with a rectangle (or explicit Smart Group), Then a Smart Group holds that content ink unaltered, **keeps the enclose stroke as boundary ink that always transforms with the group**, stores geometric bounds from recognition, and can be connector-targeted on those bounds; `fixedInk` keeps **content** ink sample size fixed while bounds (and boundary ink) still transform.

### Implemented via

| Concern | Pointer |
|---|---|
| Tree + ops + three representations | [srs-logic](./srs-logic.md) · [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) |
| Smart Group | [SRS-IN-10](./srs-logic.md) · [ADR-0011](../../../../adr/ADR-0011-smart-group.md) · [REQ-04](../../prd.md#smart-group) |
| Schema | [srs-data](./srs-data.md) |
| Open/save chrome | [srs-ui](./srs-ui.md) |
| Journeys | [srs-experience](./srs-experience.md) |
| Fidelity / dual-ask | [srs-quality](./srs-quality.md) |
