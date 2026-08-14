---
feature: device-document
parent_req: [REQ-04, REQ-07]
version: 0.1.0
lifecycle: active
owner: pm
---

# SRS — On-device working document (Product)

PM feature depth for [REQ-04](../../prd.md#device-document) and
[REQ-07](../../prd.md#one-way-sync). Node semantics are shared with Infini
([ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md),
[ADR-0011](../../../../adr/ADR-0011-smart-group.md)) — this file does not restate them; it states
what changes now that the **device** is the writer.

## Intent / JTBD

When a creator edits on the tablet, they are not filing a request with a laptop — they are moving
their own paper. Everything that decides what the document *is* has to be within reach of the pen:
the tree, the recognition, the undo, the picture on the panel. The desktop's job is to keep the
document safe, show it at scale, and stay out of the gesture.

The 2026-08-11 pilot put that decision-making one network hop away, and every fix wave
(CHL-0004…0007) failed the same way — the creator saw their own hand corrected. This feature closes
that distance.

## In scope / out of scope (feature grain)

| In scope | Out of scope |
|---|---|
| Device holds the document tree in memory for the session | On-device persistence / file browser / documents surviving app restart |
| Device ingests its own finished strokes as document nodes | Bézier fitting of handwriting (samples stay polylines) |
| Panel paints from the device document | Painting from a peer-supplied picture |
| Undo/redo history for structural ops, on the device (linear, depth 20) | Branching history, per-sample undo |
| Publish document changes to the desktop; queue them when the link is down | Applying desktop-authored changes mid-session |
| Accept an initial full-document load at start / reconnect / explicit resync | Multi-directional sync, CRDT, conflict resolution |
| Multiple boxes and nodes in one document | Multiple documents / switching documents on the device |

## Business rules / eligibility / policy

| Rule id | Statement (product language) | Notes |
|---|---|---|
| BR-D01 | **The device is the only writer of its document during a session.** No peer message may alter the document except the initial full load. | The whole point of the rework |
| BR-D02 | **The panel paints the device's own document.** No repaint is sourced from an inbound peer picture. | Kills the "snapshot is truth" model |
| BR-D03 | **A finished stroke becomes a document node.** Ink is not a transient sample stream that only exists on the wire; it is a node the device can then select, group, and transform. | Prerequisite for every edit |
| BR-D04 | **Editing never depends on the link.** Create, group, move, resize, and undo behave identically with the session up or down. Only *publishing* waits. | Link down is a normal state |
| BR-D05 | **Every structural op is undoable, one entry per completed gesture.** A wrong recognition or a slipped drag costs one undo, never a stuck document. Floor: the last 20 structural ops. | Inherits infini SRS-IN-12 semantics |
| BR-D06 | **Ingestion costs no ink latency.** Turning a stroke into a node must not push pen-down → pixel past the [REQ-01](../../prd.md#local-pen-ink) budget. | The device is a notebook first |
| BR-D07 | **The document is in memory only.** If the app restarts, in-session work not yet published is gone; the desktop file is the durable copy. | Deliberate scope limit — see Non-Goals |
| BR-D08 | **Downward traffic is exactly two kinds:** an initial full-document load, and pan/zoom viewport. Anything else arriving is a defect, not a feature. | [REQ-07](../../prd.md#one-way-sync) |
| BR-D09 | **Upward traffic is document changes**, in the order the creator made them. Live stroke samples may also flow up as a **preview**; the authoritative node arrives with the change at pen-up. | Preserves desktop liveness |
| BR-D10 | **A full load replaces the local document**, so it is accepted only at session start or on explicit resync, and only after queued changes have drained. The desktop must not send one unsolicited. | Load safety |
| BR-D11 | **Queued changes are never silently dropped.** If they cannot be published, the creator can see that changes are pending ([REQ-03](../../prd.md#tool-modes) status affordance). | No invisible data loss |
| BR-D12 | **The device and desktop mean the same thing by a document.** Node kinds, roles, `inkScaleMode`, and UV offsets are shared vocabulary — a document authored on the device round-trips through desktop save/open without loss. | Shared domain doc |

## Edge cases

| Case | Expected product behavior |
|---|---|
| Session never connects | Full editing works; all changes queue; status shows unpublished work |
| Link drops mid-gesture | The gesture completes locally and commits normally; the change joins the queue |
| Link returns with a queue | Queue publishes in order first; only then may a load be accepted |
| Load arrives unsolicited mid-session | Rejected; treated as a protocol defect and surfaced in the status line, not applied |
| Load arrives while changes are queued | Deferred until the queue drains; never applied over unpublished work |
| App restarts | Local document is empty; the next session start load restores the desktop's copy (which contains everything previously published) |
| Undo past the history floor | Oldest entries are discarded; the creator cannot undo below the floor and the UI does not pretend otherwise |
| Undo of an op that was already published | Undo is itself a change and publishes like any other — the mirror follows |
| Document grows large enough to threaten ink latency | Ink latency wins: the device may degrade repaint fidelity or defer non-essential work, never pen responsiveness |
| Desktop is running an older document format | Out of scope this iter; single-version assumption, flagged as an architect risk |

## Acceptance (drives BDD / stories)

- Given no session has ever connected, When the creator draws 20 strokes and creates 2 ink-boxes,
  Then all work exists in the local document and paints on the panel (0 features unavailable).
- Given a stroke ends, When the device ingests it, Then the node exists with p95 ≤50 ms after pen-up
  and the next stroke still meets pen-down → pixel p95 ≤30 ms.
- Given any panel repaint, When it occurs, Then it renders the local document (0 repaints sourced
  from an inbound peer picture).
- Given 20 structural ops, When the creator undoes 20 times, Then the document returns through each
  intermediate state to the starting state (geometry ±1 px @ 100% zoom; 1 undo per gesture).
- Given the link drops and the creator performs 10 document operations, When the link returns, Then
  all 10 publish in order (0 lost, 0 reordered) before any load is accepted.
- Given a full load is offered while changes are queued, When the device handles it, Then the queue
  drains first and 0 queued changes are discarded.
- Given a completed initial load, When both ends settle, Then the device document equals the desktop
  document (0 divergent nodes).
- Given a live session after the initial load, When the session is traced for its whole lifetime,
  Then 0 inbound document messages are observed.
- Given a document authored entirely on the device, When the desktop saves and reopens it, Then
  bounds, transform, `inkScaleMode`, child roles, and ink samples match (±1 px @ 100% zoom).

## Implemented via

| Concern | Pointer |
|---|---|
| Tree, ingestion, undo ring, change queue | `srs-logic.md` — architect, ids from `SRS-EP-07` |
| Sync contract + message set | `srs-logic.md` + **ADR-0015** |
| Ownership decision | **ADR-0014** |
| Change envelope schema | [infini srs-data](../../../infini/features/vector-document/srs-data.md) — extend, do not fork |
| Node semantics | [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) · [ADR-0011](../../../../adr/ADR-0011-smart-group.md) |
| Budgets and parity | `srs-quality.md` — architect |
| What edits the document | [ink-box](../ink-box/srs-product.md) |
| Status affordances | [tool-modes srs-ui](../tool-modes/srs-ui.md) — [SRS-EP-05] |

---

## Superseded

Inherits, on the device, the semantics of infini [SRS-IN-12] (undo ring) and the document-authority
half of [SRS-IN-07] / [SRS-IN-13]. See the
[lifecycle map](../../../../../.plan/iter-003/lifecycle-map-2026-08-13.md).
