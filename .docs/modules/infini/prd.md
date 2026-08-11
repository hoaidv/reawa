---
title: PRD — Infini
module: infini
version: 0.3.0
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
| Ink-box gesture lands first try | N/A | ≥80% of a 20-gesture scripted enclose set create the intended Smart Group on the first attempt; 0 creations on the negative set | 2026-Q3 | Manual QA |
| Ink-box result reaches the tablet | N/A | p95 ≤500 ms from op to settled panel for the synced region | 2026-Q3 | Manual / trace |

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
- **Priority:** Must (iter-003 campaign; raised from Could 2026-08-11) · **Traces:** [BRD-07]
- Needs design: yes
- **Outcome:** a creator who has handwritten a thought can make that cluster behave as **one
  object** — move it, resize it, keep the ink as ink — from either device, without OCR
  ([ADR-0011](../../adr/ADR-0011-smart-group.md)).
- **Tool modes.** The creator picks a tool before acting. Infini offers **Selection · Ink-box**;
  Epaper offers **Selection · Pen · Ink-box** ([epaper REQ-03](../epaper/prd.md#tool-modes)).
  Tool state is **per device** — it is not synced between peers.
- **Creation A — ink-box tool (enclose).** In `Ink-box` mode the creator draws a shape around
  ink. Infini **recognizes** a roughly rectangular stroke (recognize, do **not** convert to a
  Primitive), captures every ink whose samples are ≥80% inside, and creates a Smart Group: the
  enclose stroke is kept as `role: boundary` ink, contained ink is reparented as `role: content`,
  and `bounds` is the fitted rect. Guards: the fitted rect must be ≥ a fixed minimum size **and**
  contain ≥1 ink — otherwise the stroke stays ordinary ink. Enclosure is **rectangle-only** in
  the pilot.
- **Creation B — selection (explicit).** In `Selection` mode the creator selects ink and invokes
  Smart Group. Infini must find **one stroke among the selection** that surrounds almost all of
  the other selected ink (≥80% of each other stroke’s samples inside that surround stroke’s
  region). The surround stroke may be **open**; Infini builds an **artificial closed path** from
  it for the containment test only. That stroke becomes `role: boundary`; the rest become
  `role: content`; `bounds` = fitted rect of the surround stroke. **If no such surround stroke
  exists, creation is refused** (command disabled or no-op with a clear reason — no AABB-only
  Smart Group).
- **Appearance.** A Smart Group always has boundary ink after a successful create (enclose path
  or selection-with-surround). The creator’s surround stroke is the visual frame — never a
  synthetic ink rectangle. (A subtle non-ink hint remains available only as chrome for selection
  handles / hit-testing, not as a substitute for a missing boundary.)
- **Manipulation (pilot scope).** Move by dragging inside the bounds (no prior selection needed);
  resize via handles after explicit selection. Tapping empty canvas deselects; tapping another
  node selects that node. `inkScaleMode` (`withBounds` | `fixedInk`) is toggleable on a selected
  Smart Group. **Rotation and connector attachment are out of pilot** (see Non-Goals).
- **Draw into an existing box.** When the creator draws with `Pen` (ordinary ink) and ≥80% of
  the new stroke’s samples fall inside one or more Smart Groups’ world bounds, Infini parents
  that stroke as `role: content` of the matching box. If several boxes qualify (including nested
  boxes that each contain 100% of the stroke), pick the one with the **highest paint / z order**
  (tree sibling order — later siblings paint above; no separate z-index field). Adding ink never
  reflows, realigns, or shifts existing content inside the box — freehand placement as drawn.
- **`inkScaleMode` feel.** `withBounds`: content scales with the box. `fixedInk`: each content
  ink keeps its sample size fixed and tracks the box via **its own** relative offset / UV inside
  the box (so a newly drawn stroke never moves older content). Boundary ink always transforms
  with the frame. **In-box text-style alignment is deferred** (Non-Goals).
- **Cross-device.** Both creation paths and manipulation are available on both devices, and the
  document result syncs to the peer. Recognition and all tree ops run on **Infini**; Epaper
  contributes tool intent plus pen samples ([epaper REQ-03](../epaper/prd.md#tool-modes)).
- **Shipped status (2026-08-11):** `create_smart_group` / `set_smart_transform` apply in
  `VectorDocument` unit tests only. Live ink is still the flat WorldLayer primitive list, so **no
  ink reaches the tree yet** — tree-backed ink ingestion, selection/hit-testing, and an undo
  stack are prerequisites for every path above and none exist in code today.

**Acceptance**
- Given the `Ink-box` tool and ink on the canvas, When the creator draws a closed roughly
  rectangular stroke whose fitted rect is ≥ the minimum size and contains ≥80% of that ink's
  samples, Then a Smart Group appears within 300 ms holding the enclose stroke as `boundary` and
  the contained ink as `content`, and one undo restores the previous tree exactly.
- Given the same gesture over empty canvas, or a fitted rect below the minimum size, When the
  stroke ends, Then no Smart Group is created and the stroke remains ordinary ink (0 creations
  on the negative fixture set).
- Given the `Selection` tool and selected ink that includes a surround stroke containing ≥80% of
  each other selected stroke’s samples (open stroke OK — tested via an artificial closed path),
  When the creator invokes Smart Group, Then a Smart Group is created with that stroke as
  `boundary`, the others as `content`, and `bounds` from the surround stroke’s fitted rect
  (±1 px @ 100% zoom).
- Given the `Selection` tool and selected ink with **no** stroke that surrounds the rest at the
  ≥80% bar, When the creator invokes Smart Group, Then **no** Smart Group is created (0 creations
  on the negative fixture set) and the selection is unchanged.
- Given a Smart Group and viewport scale ≥0.35, When the creator drags inside its bounds, Then
  the box moves with the pointer and the canvas does **not** pan; below 0.35 the drag pans and
  the UI shows that manipulation is unavailable.
- Given a selected Smart Group, When the creator drags a resize handle, Then `bounds` follow the
  handle; under `withBounds` content ink scales with them; under `fixedInk` each content ink’s
  sample size changes ≤1 px and **each** content ink’s own UV/offset in the box is preserved
  (±1 px @ 100% zoom), without moving unrelated content inks; boundary ink still transforms.
- Given the `Pen` tool and an existing Smart Group, When the creator draws a stroke with ≥80% of
  samples inside that box’s world bounds, Then the stroke becomes `role: content` of that box
  within 300 ms and no other content ink is translated or reflowed.
- Given nested Smart Groups that each contain ≥80% of a new `Pen` stroke, When the stroke ends,
  Then the stroke parents under the qualifying box with the highest paint/z order (later sibling
  wins; 0 dual-parent outcomes).
- Given a live session and a Smart Group created or moved on Infini, When Epaper settles, Then
  the panel shows the same result for that region with p95 ≤500 ms after the op (0 divergent
  figures).
- Given a Smart Group in the tree, When the document is saved and reopened, Then bounds,
  transform, `inkScaleMode`, child roles, and ink samples match (geometry ±1 px @ 100% zoom).
- **UI states / journeys to design:** tool switching; `Ink-box` armed; enclose accepted; enclose
  rejected (too small / no content); `Selection` → Smart Group with surround stroke; selection
  create refused (no surround); Smart Group selected with handles; `inkScaleMode` toggle;
  draw-into-box; nested z-order membership; manipulation unavailable below LOD.

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
- **OCR / handwriting-to-Text** as part of Smart Group; no "is this text?" gate on recognition —
  the ink-box captures **any** ink inside the enclosure.
- Perfect enclose recognition — pilot is best-effort + undo + explicit fallback.
- **Automatic (unprompted) ink-box creation** — every Smart Group is created under an explicit
  tool mode or an explicit selection command; drawing a rectangle in `Pen` mode never groups.
- **Rotating a Smart Group and attaching connectors to its bounds** — deferred past the pilot;
  anchor resolution is translate + scale only today.
- Non-rectangular enclosure shapes (ellipse, lasso) — `bounds` is axis-aligned in the pilot.
- **In-box content alignment / reflow** (left/center/right, baseline snap, auto-padding when
  appending ink) — deferred; pilot keeps freehand placement and `fixedInk` centroid tracking only.
- Drawing ink on Infini with a mouse — `Pen` is an Epaper-only tool in the pilot.
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

- Migrate production wire to ADR-0009 `doc_op` / `append_ink` — **owner:** architect —
  **needed by:** next sync wave (today: `stroke_*` + `doc_snapshot`)
- Exact SVG attribute grammar for Infini profile v1 — **owner:** architect — **needed by:** 2026-08-24
- Whether Infini ships macOS-first only in v0 — **owner:** pm — **needed by:** 2026-08-24
- How Epaper tool intent reaches Infini — **owner:** architect — **needed by:** before the first
  REQ-04 implement story. Flag on `stroke_begin` vs a new tool message; and how Smart Group
  results return to the tablet given `doc_op` is not on the production wire.
- Whether Epaper `Selection` picks locally — **owner:** architect — **needed by:** before the
  first REQ-04 implement story. Hit-test against the last `doc_snapshot` on device, or relay the
  pick to Infini and wait for a snapshot.
- Minimum fitted-rect size for enclose — **owner:** architect with qa evidence — **needed by:**
  first pilot build. World units vs screen px, and the value itself.

## Linked Modules

- [epaper](../epaper/prd.md) — tablet ink + region-sync consumer/producer
- [reawa](../reawa/prd.md) — sibling pen-driver product; not in this campaign scope
- Exploration: [EXP-0001](../../../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md)
