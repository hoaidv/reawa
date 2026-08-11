---
from: pm
to: architect
iter: iter-003
date: 2026-08-11
subject: ink-box-ux-adopted
cc: [sm, designer, qa]
verdict: READY-WITH-CONCERNS
---

# PM → Architect — ink-box UX adopted; REQ-04 raised to Must, new epaper REQ-03

Human briefed the ink-box UX. I evaluated three creation models (fully automatic, tool-armed,
manual selection) and the human chose **tool-armed + explicit selection**, accepting the cost of
an on-device toolbar. Product docs are updated; the wire and device questions are yours.

## Decisions adopted

| Decision | Where recorded |
|---|---|
| Creation = `Ink-box` tool armed **or** explicit selection. Never unprompted. | [REQ-04](../../../.docs/modules/infini/prd.md#smart-group) · BR-09a |
| Tablet gets a 3-tool toolbar: **Selection · Pen · Ink-box**, finger-operated | [epaper REQ-03](../../../.docs/modules/epaper/prd.md#tool-modes) |
| Infini gets **Selection · Ink-box** (no `Pen` — no mouse drawing in pilot) | REQ-04 Non-Goals |
| Tool state is **per device**, never synced | BR-09e |
| Selection-created box (no boundary ink) renders a **subtle non-ink bounds hint** | BR-09d |
| Manipulation = move + resize + `inkScaleMode` toggle. **No rotation, no connectors.** | BR-09f · REQ-04 Non-Goals |
| Enclosure is rectangle-only; guards = min fitted-rect size **and** ≥1 ink inside | BR-09b |
| No "is this text?" gate — the box captures any ink | BR-09c (holds the no-OCR line) |

**Lifecycle:** infini REQ-04 `Could → Must` (campaign goal); epaper REQ-03 added `active`.
Module versions bumped (infini 0.3.0, epaper 0.4.0), plus `srs-product` 0.3.0 and
`srs-experience` 0.3.0 (three Smart Group journeys replace the single one).
`adlc prd-check`: **0 fail, 0 warn** on both modules.

## Why the human's original "fully automatic" was rejected

Creating a Smart Group reparents ink into group-local coordinates (ADR-0011 §2), so a false
positive silently restructures the document — and a rectangle drawn around handwriting is the
single most common shape in note-taking (diagram nodes, table cells, emphasis boxes). The cost
of a false positive dominates the cost of a miss, and there is **no undo stack in the code**.
Arming a tool makes intent explicit and removes the need for a proposal/accept step entirely.

## Concerns you own (why the verdict is not plain READY)

1. **No ink reaches the tree.** `CanvasStage.rebuildWithRmInk` converts `stroke_*` into flat
   `Primitive` paths on `InfiniDocument`; `VectorDocument` is unit-test-only. Every REQ-04 path
   needs tree-backed ink ingestion first (`syncFromVectorDoc` exists but is off the stroke path).
   **This is the critical-path prerequisite — please sequence it as its own SRS/story.**
2. **No undo, no selection, no hit-testing** exist in `infini/src`. Drag currently means pan
   (`CanvasStage.onPointerDown`). BR-09/undo AC cannot be met without them.
3. **Tool intent has no wire.** Production is `stroke_*` + `doc_snapshot`; `doc_op` is a stated
   Non-Goal. Recommend a flag on `stroke_begin` so recognition stays on Infini and the epaper
   Non-Goal (no on-device recognition) survives — your call, but please close it before slicing.
4. **Epaper `Selection` is the expensive one.** The device has `m_vectorNodes` from the last
   `doc_snapshot` plus `panelToWorld`, so a local hit-test is plausible; a drag preview needs
   local re-raster. Alternative is relaying the pick to Infini and eating a round trip.
5. **Touch may not be available.** The Qt app handles pen only (`tabletappfilter`). If the
   capacitive layer is not reachable, REQ-03's design changes shape — flag early.
6. **Below-LOD behaviour is specified but unusual.** `allowIndividualInteraction` disables picking
   under 0.35 scale; REQ-04 requires the UI to *say* manipulation is unavailable rather than
   silently panning. Needs an `srs-ui` state.
7. **Minimum fitted-rect size** is deliberately not a user setting. Pick world vs screen units and
   a value; QA measures against the ≥80% first-try metric.

## Ask

1. Thicken `srs-logic` [SRS-IN-10] for tool-armed creation (drop the propose/accept step),
   plus new SRS for selection/hit-testing, undo, and tool-mode transport.
2. Decide open questions 3–5 above; amend [ADR-0011](../../../.docs/adr/ADR-0011-smart-group.md)
   §4 (creation paths) and §5 (recognition quality) — the pilot no longer proposes-and-accepts.
   A new ADR is probably warranted for the on-device toolbar.
3. Author `srs-ui` for the Epaper toolbar (new epaper feature folder) and extend the
   vector-document `srs-ui` with selection, handles, and the subtle bounds hint.

Then **`/sm`** slices: 1 prerequisite implement story (tree ink ingestion), design stories for
`epaper/REQ-03` and `infini/REQ-04` (both `Needs design: yes` — `adlc gate` currently FAILs the
design-coverage check for exactly these two), and the implement stories behind them.
