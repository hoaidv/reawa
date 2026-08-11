---
title: PRD — Infini
module: infini
version: 0.1.0
lifecycle: active
parent_brd: [BRD-07]
owner: pm
source: EXP-0001
---

# PRD — Infini

Desktop **infinity canvas** viewer/editor paired with the on-device [Epaper](../epaper/prd.md)
drawing tablet. Code home TBD under repo-root `infini/` (see [ADR-0008](../../adr/ADR-0008-electron-react-infini.md)).

## Problem & Job-to-be-Done

Artists want reMarkable’s writing feel **and** a large, pan/zoomable canvas on the
desktop. Today Epaper can ink locally (EXP-0001 S1) but there is no first-class desktop
infinity surface, shared vector document, or drawing-region sync. Without those, the
tablet cannot act as a true drawing tablet for an infinite desktop canvas.

## Target Users

- **Primary:** Creators who draw on RM2 and review/navigate the full canvas on desktop.
- **Secondary:** Developers validating sync latency and document interchange.

## Success Metrics

| Metric | Baseline | Target | By when | Source |
|---|---|---|---|---|
| Pan / pinch / zoom feel smooth on trackpad | N/A | No visible stutter at 60 Hz display during continuous gesture | 2026-Q3 | Manual QA |
| Document round-trip | N/A | Saved SVG reopen renders same geometry (±1 px @ 100% zoom) | 2026-Q3 | Manual QA |
| Stroke RM → Infini visible | EXP open | p95 ≤ 50 ms after RM sample (EXP S2) | 2026-Q3 | Manual / trace |
| Viewport Infini → RM drawing region | EXP open | Next pen sample uses new region (mapping latency ahead of full e-paper refresh) | 2026-Q3 | Manual / EXP S3 |

## [REQ-01] Infinity canvas navigation {#infinity-canvas}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- The desktop app (“Infini”) presents an infinite 2D canvas. Users pan and zoom/pinch so
  the visible window samples a drawing region of world space. Scene transform is
  **translate + uniform scale** only; primitive figures render with correct scale and
  translation under that transform.

**Acceptance**
- Given Infini is focused on a 60 Hz display, When the user pans with trackpad, mouse drag,
  wheel, or keyboard-modifier + wheel for ≥5 s, Then the canvas translates continuously
  with ≤2 dropped frames/s perceived.
- Given Infini is focused, When the user pinches on a trackpad or uses keyboard-modifier +
  wheel to zoom, Then uniform scale changes about the gesture focus (or documented
  fallback) with the same ≤2 dropped frames/s budget.
- Given a circle and a square in world space, When the user pans and zooms, Then both keep
  aspect (circle stays circular) and screen positions match `screen = (world + translate) * scale`.
- **UI states / journeys to design:** empty canvas; canvas with primitives; gesture in
  progress; window resize mid-gesture; open-document CTA path.

## [REQ-02] Vector document interchange {#vector-document}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- One **persistence** format (SVG profile), one **in-memory tree-of-vectors**, and one
  **transmit** op encoding so Epaper and Infini exchange scene data. The tree holds
  handwritten **ink** (dense polylines of tablet samples: position, pressure, tilt, and other
  reported ink channels), **text** paragraphs, **primitive** shapes,
  nestable **groups**, root-level **frames**, and **connectors** between nodes
  ([ADR-0010](../../adr/ADR-0010-tree-of-vectors.md)). Infini must render persistence.

**Acceptance**
- Given a document with ink, text, primitives, groups, frames, and connectors saved as the
  persistence format, When Infini opens it, Then nodes reappear with the same ids, parenting,
  and geometry (error ≤1 px at 100% zoom).
- Given the in-memory tree, When it is serialized for transmit and deserialized on the peer,
  Then a round-trip yields equivalent ops/tree (100% op equality on the fixture set).
- Given Infini has an open document, When the user exports persistence format, Then a
  standards-readable SVG (Infini profile) is written.
- **UI states:** doc.none / doc.open / doc.dirty / doc.error (open/save chrome).
- **Structure:** Groups may nest anywhere; Frames only at document root; handwriting remains
  polyline ink samples (not Bézier-fitted) in v0; sample channels from the tablet are preserved.

## [REQ-04] Smart Group / ink-box (pilot) {#smart-group}
- **Priority:** Could · **Traces:** [BRD-07]
- Needs design: yes
- **Pilot:** When the user handwriting is enclosed by a hand-drawn rectangle (or any selected
  ink is promoted), Infini creates a **Smart Group**: ink stays ink (no OCR), an explicit
  rectangular **boundary** frames it, the unit moves together, scales (independent axes and
  aspect-lock), rotates, is a **connector target**, and supports **ink scale mode**
  (`withBounds` vs `fixedInk` text-box feel). Not limited to the enclose gesture — any ink
  set may become a Smart Group ([ADR-0011](../../adr/ADR-0011-smart-group.md)).

**Acceptance**
- Given ink strokes on the canvas, When the user completes a rectangular enclose gesture
  around them (or explicit Smart Group), Then a Smart Group exists whose children include those
  content Ink nodes (unaltered samples) **and** the enclose stroke as boundary ink, and whose
  geometric bounds match the recognized (x, y, width, height) within ≤3 CSS px @ 100% zoom on
  the happy-path fixture set.
- Given a Smart Group, When the user translates, rotates, or scales the boundary (including
  non-uniform scale), Then the group moves as one connector-targetable node; **boundary ink
  always transforms** with the group; with `inkScaleMode=withBounds`, **content** ink
  transforms with the group; with `fixedInk`, content ink sample geometry stays fixed size
  while bounds (and boundary ink) change.
- Given a Smart Group, When a connector is attached to a preferred edge midpoint, Then the
  anchor tracks the Smart Group boundary after transform.
- Given enclose recognition false positive/negative, When the user undoes or uses explicit
  create, Then no data loss of ink samples (100% sample channel preservation).

## [REQ-03] Tablet drawing-region sync {#tablet-sync}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: no
- Epaper is the drawing tablet: local ink on its panel **and** vectors sent to Infini.
  Infini pan/zoom changes the drawing region and sends that region to Epaper. Epaper
  refresh may be laggy (ghosting OK), but the **drawing-area mapping** stays in sync with
  least latency so the next pen samples ink in the correct place. Epaper must not show a
  different document than Infini for the shared drawing region.

**Acceptance**
- Given a live session, When the user draws on Epaper, Then Infini shows the new vectors
  with p95 ≤50 ms after the RM sample.
- Given a live session, When the user pans or zooms Infini, Then Epaper’s drawing-region
  map updates with p95 ≤100 ms so the next pen-down matches world coordinates (full panel
  redraw may trail).
- Given both sides share the session document, When Epaper refreshes its panel for the
  drawing region, Then the vectors in that region match Infini’s document for the same
  region (0 divergent strokes).

---

## Non-Goals

- On-device pan/zoom gestures on Epaper (deferred — slow gesture detect + e-paper cost).
- Reawa pen-relay / mouse emulation features inside Infini.
- Multi-user collaborative editing; cloud sync; CRDT productization beyond what Architect
  needs for two-device consistency.
- Pressure-rich brushes, layers UI, or full illustration suite.
- **OCR / handwriting-to-Text** as part of Smart Group (ink stays ink).
- Perfect enclose recognition — pilot is best-effort + undo + explicit fallback.

## Assumptions & Dependencies

- Epaper [REQ-01] local ink remains correct (orientation / aspect / latency).
- USB network path RM2 ↔ desktop remains available (same as Reawa/Epaper deploy).
- Architect chooses shell ([ADR-0008](../../adr/ADR-0008-electron-react-infini.md)) and
  consistency mechanism for [REQ-03].

## Open Questions

- Exact SVG attribute grammar for Infini profile v1 (tighten [SRS-IN-09](./features/vector-document/srs-data.md)) — **owner:** architect — **needed by:** 2026-08-17
- Transport (reuse EXP TCP JSON-lines vs other) — **owner:** architect — **needed by:** 2026-08-17
- Whether Infini ships macOS-first only in v0 while Electron enables later Windows/Linux — **owner:** pm — **needed by:** 2026-08-24
- Local transforms for ordinary Groups/Frames — still deferred; **SmartGroup** has local TF per [ADR-0011](../../adr/ADR-0011-smart-group.md) — **owner:** architect

## Linked Modules

- [epaper](../epaper/prd.md) — tablet ink + region-sync consumer/producer
- [reawa](../reawa/prd.md) — sibling pen-driver product; not in this campaign scope
- Exploration: [EXP-0001](../../../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md)
