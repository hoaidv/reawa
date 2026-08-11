---
title: PRD — Infini
module: infini
version: 0.2.0
lifecycle: active
parent_brd: [BRD-07]
owner: pm
source: EXP-0001
---

# PRD — Infini

Desktop **infinity canvas** viewer/editor paired with the on-device [Epaper](../epaper/prd.md)
drawing tablet. Code home: repo-root `infini/` ([ADR-0008](../../adr/ADR-0008-electron-react-infini.md)).

**Docs rule (2026-08-11):** product docs describe **what ships in code today**. Aspirational
tree-op sync / Smart Group UI stay in Non-Goals or Could until implemented.

## Problem & Job-to-be-Done

Artists want reMarkable’s writing feel **and** a large, pan/zoomable canvas on the
desktop. Epaper inks locally; Infini owns pan/zoom and shows the shared drawing region.
Without region sync and stroke parity, the tablet cannot act as a true drawing tablet for
an infinite desktop canvas.

## Target Users

- **Primary:** Creators who draw on RM2 and review/navigate the full canvas on desktop.
- **Secondary:** Developers validating sync latency and document interchange.

## Success Metrics

| Metric | Baseline | Target | By when | Source |
|---|---|---|---|---|
| Pan / pinch / zoom feel smooth on trackpad | N/A | No visible stutter at 60 Hz display during continuous gesture | 2026-Q3 | Manual QA |
| Document round-trip (library) | N/A | SVG profile reopen → same geometry (±1 px @ 100% zoom) on fixture set | 2026-Q3 | Automated + Manual |
| Stroke RM → Infini visible | EXP open | p95 ≤ 50 ms after RM sample | 2026-Q3 | Manual / trace |
| Viewport Infini → RM drawing region | EXP open | Next pen sample uses new region (map ahead of full e-paper refresh) | 2026-Q3 | Manual |
| Stroke thickness parity under zoom | N/A | World width × scale on both peers (old + new strokes match after zoom) | 2026-Q3 | Manual |

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
  progress; window resize mid-gesture; open-document CTA path (chrome deferred).

## [REQ-02] Vector document model {#vector-document}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- Infini maintains a **tree-of-vectors** library (Ink, Text, Primitive, Group, Frame,
  Connector, SmartGroup) with SVG profile serialize/parse and idempotent ops
  ([ADR-0010](../../adr/ADR-0010-tree-of-vectors.md)). **Live paint today** uses the
  WorldLayer primitive list (`InfiniDocument`) projected on the canvas; the tree library
  is exercised by unit tests and is the target SoT for structure edits / persistence UI.

**Acceptance**
- Given a tree fixture with ink, text, primitives, groups, frames, and connectors, When
  Infini serializes and re-parses the SVG profile, Then ids, parenting, and geometry match
  (±1 px @ 100% zoom on the fixture set).
- Given the in-memory tree, When ops are applied by `opId`, Then duplicate `opId`s are
  idempotent and unknown types do not crash.
- Given Infini’s live WorldLayer, When demo or RM ink is shown, Then figures paint under
  the current viewport with world stroke widths ([ADR-0012](../../adr/ADR-0012-world-stroke-viewport-parity.md)).
- **UI states (target):** doc.none / doc.open / doc.dirty / doc.error — open/save chrome
  not required for tablet-sync Must wave.
- **Structure:** Groups may nest; Frames only at document root; handwriting remains
  polyline samples (not Bézier-fitted) in v0.

## [REQ-04] Smart Group / ink-box (pilot) {#smart-group}
- **Priority:** Could · **Traces:** [BRD-07]
- Needs design: yes
- **Pilot (library only today):** When handwriting is enclosed by a hand-drawn rectangle
  (or selected ink is promoted), Infini creates a **Smart Group**: ink stays ink (no OCR),
  an explicit rectangular **boundary** frames it, the unit moves/scales/rotates together,
  is a connector target, and supports `withBounds` vs `fixedInk`
  ([ADR-0011](../../adr/ADR-0011-smart-group.md)).
- **Shipped status:** `create_smart_group` / `set_smart_transform` apply in
  `VectorDocument` unit tests. **No** live enclose UI, handles, or wire emit yet.

**Acceptance** (library / future UI — not blocking tablet-sync Must)
- Given ink strokes and a create-smart-group op, When applied, Then children + bounds +
  transform match the fixture.
- Given `inkScaleMode=withBounds` vs `fixedInk`, When `set_smart_transform` runs, Then
  content ink behaves per [ADR-0011](../../adr/ADR-0011-smart-group.md).
- Enclose recognition / undo UX remain pilot quality goals once UI ships.

## [REQ-03] Tablet drawing-region sync {#tablet-sync}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: no
- Epaper is the drawing tablet: local ink on its panel **and** stroke samples to Infini.
  Infini pan/zoom updates the drawing region (`viewport`) and pushes a one-shot vector
  **`doc_snapshot`** of WorldLayer figures for the region. Ongoing pan/zoom is viewport-only;
  Epaper re-rasterizes locally. Stroke width is **world units** on both peers. Sync-frame
  orientation uses Reawa-style **four gut poses** (default tall: gut to the left).

**Acceptance**
- Given a live session, When the user draws on Epaper, Then Infini shows the new stroke
  paths with p95 ≤50 ms after the RM sample (`stroke_begin|point|end`).
- Given a live session, When the user pans or zooms Infini, Then Epaper’s drawing-region
  map updates with p95 ≤100 ms so the next pen-down matches world coordinates (full panel
  redraw may trail; ghosting OK).
- Given Infini has WorldLayer content, When Epaper receives `doc_snapshot` (or settle
  viewport), Then vectors in the drawing region match Infini for that region (0 divergent
  figures after settle).
- Given zoom-out on Infini, When Epaper shows existing + new local ink, Then stroke
  thickness scales with the region (world × panel scale) — new strokes are not
  screen-constant thickness.
- Given the Sync orientation control, When the user cycles gut poses, Then tall/wide frame
  aspect and axis mapping match the chosen pose (vertical gut-to-left verified correct).

---

## Non-Goals

- On-device pan/zoom gestures on Epaper (deferred).
- Reawa pen-relay / mouse emulation features inside Infini.
- Multi-user collaborative editing; cloud sync; CRDT productization.
- Pressure-rich brushes, layers UI, or full illustration suite.
- **OCR / handwriting-to-Text** as part of Smart Group.
- Perfect enclose recognition — pilot is best-effort + undo + explicit fallback.
- **Live bidirectional `doc_op` / `append_ink` wire** — library + unit tests only; production
  path is `doc_snapshot` + `stroke_*` until a migration story lands.
- Bitmap / PNG `region_refresh` as the region picture (ignored on device).
- Doc open/save chrome and tree-driven live paint (deferred beyond Must sync wave).

## Assumptions & Dependencies

- Epaper [REQ-01] local ink remains correct (Round 19 digitizer map).
- USB network path RM2 ↔ desktop remains available (`RM_SYNC_HOST`, TCP `:9877`).
- Architect owns session contract ([ADR-0009](../../adr/ADR-0009-shared-document-viewport.md)
  + interim wire note) and stroke parity ([ADR-0012](../../adr/ADR-0012-world-stroke-viewport-parity.md)).

## Open Questions

- Migrate production wire from `stroke_*` + `doc_snapshot` → ADR-0009 `doc_op` /
  `append_ink` — **owner:** architect — **needed by:** next sync wave
- Exact SVG attribute grammar for Infini profile v1 — **owner:** architect — **needed by:** 2026-08-24
- Whether Infini ships macOS-first only in v0 — **owner:** pm — **needed by:** 2026-08-24
- Smart Group UI wave sequencing — **owner:** pm — **needed by:** after Must sync stable

## Linked Modules

- [epaper](../epaper/prd.md) — tablet ink + region-sync consumer/producer
- [reawa](../reawa/prd.md) — sibling pen-driver product; not in this campaign scope
- Exploration: [EXP-0001](../../../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md)
