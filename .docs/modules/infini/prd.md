---
title: PRD — Infini
module: infini
version: 0.7.0-draft
lifecycle: active
parent_brd: [BRD-07]
owner: pm
source: EXP-0001
---

# PRD — Infini

Desktop **infinity canvas** paired with the on-device [Epaper](../epaper/prd.md)
drawing tablet. Code home: repo-root `infini/` ([ADR-0008](../../adr/ADR-0008-electron-react-infini.md)).

**Docs rule (2026-08-11):** product docs describe **what ships in code today**. Aspirational
tree-op sync / Smart Group UI stay in Non-Goals or Could until implemented.

**Ownership rework (2026-08-13, [CHL-0008](../../../.plan/iter-003/challenges/CHL-0008-architecture-rework.md)).**
Infini is now **viewer, navigator, and persistence home**. The tablet owns the working document
in-session ([epaper REQ-04](../epaper/prd.md#device-document)); Infini opens/saves it, holds a
mirror it rebuilds from published tablet changes, and **navigates its own canvas**. Desktop-side
ink-box authoring is **deprecated** ([REQ-04](#smart-group)) until multi-directional sync lands.

**Viewport follow (2026-08-20, human).** Infini and Epaper cameras are **independent by default**.
Always-on Infini→tablet viewport drive is **obsolete**. Optional one-way **follow**
([REQ-06](#viewport-follow) / [epaper REQ-19](../epaper/prd.md#viewport-follow)) is mutually
exclusive and off on disconnect. Document channel stays one-way (change later is a Non-Goal).

**Pen-button map (2026-08-20, human).** Infini does **not** host the barrel-button map editor.
The creator configures Click / Hold-move **on the tablet** ([epaper REQ-18](../epaper/prd.md#pen-buttons)).
This module’s [REQ-05](#pen-button-map) is persist/restore only — **not** a desktop settings surface.

## Problem & Job-to-be-Done

Artists want reMarkable’s writing feel **and** a large, pan/zoomable canvas on the
desktop. Epaper inks and edits locally; Infini navigates **its** canvas at scale and keeps the
file. Viewports are independent unless the creator turns on follow. Without optional region
follow and stroke parity, the tablet cannot act as a true drawing tablet for an infinite desktop
canvas — but always-on desktop drive of the tablet camera is the wrong default.

The 2026-08-11 pilot also showed the cost of making the desktop the sole tree writer: every tablet
gesture became a request the desktop answered, and the answer arrived late enough to visibly correct
the creator's own hand. Infini gives up that authority so the tablet can feel immediate.

## Target Users

- **Primary:** Creators who draw on RM2 and review/navigate the full canvas on desktop.
- **Secondary:** Developers validating sync latency and document interchange.

## Success Metrics

| Metric | Baseline | Target | By when | Source |
|---|---|---|---|---|
| Pan / pinch / zoom feel smooth on trackpad | N/A | No visible stutter at 60 Hz display during continuous gesture | 2026-Q3 | Manual QA |
| Document round-trip (library) | N/A | SVG profile reopen → same geometry (±1 px @ 100% zoom) on fixture set | 2026-Q3 | Automated + Manual |
| Stroke RM → Infini visible (preview) | EXP open | p95 ≤ 50 ms after RM sample | 2026-Q3 | Manual / trace |
| Viewport Infini → RM drawing region | EXP open | **While Epaper follow is on:** next pen sample uses new region (map ahead of full e-paper refresh). **Default (follow off):** tablet map unchanged by Infini pan | 2026-Q3 | Manual |
| Viewport Epaper → Infini (follow on) | N/A | Infini canvas matches tablet after settle when Infini follow is on; independent when off | 2026-Q3 | Manual QA |
| Dual-follow | N/A | 0 sessions with both follows on at once; disconnect forces both off | 2026-Q3 | Manual / trace |
| Stroke thickness parity under zoom | N/A | World width × scale on both peers (old + new strokes match after zoom) | 2026-Q3 | Manual |
| Tablet change → desktop mirror | N/A | p95 ≤300 ms after the op; 0 divergent figures for the region after settle | 2026-Q3 | Manual / trace |
| Document messages sent to the tablet after initial load | N/A | **0** per session | 2026-Q3 | Trace |
| Mirror convergence after reconnect | N/A | Desktop and tablet documents equal (0 divergent nodes) once the change queue drains | 2026-Q3 | Manual QA |

## [REQ-01] Infinity canvas navigation {#infinity-canvas}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- The desktop app (“Infini”) presents an infinite 2D canvas. Users pan and zoom/pinch so
  the visible window samples a drawing region of world space. Scene transform is
  **translate + uniform scale** only; primitive figures render with correct scale and
  translation under that transform.
- Infini always navigates **its** canvas. That navigation reaches the tablet **only** while
  Epaper follow is on ([epaper REQ-19](../epaper/prd.md#viewport-follow)). Default: independent.

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
  progress; window resize mid-gesture; open-document CTA path (chrome deferred). Viewport-follow
  toggle is [REQ-06](#viewport-follow), not this canvas package.

## [REQ-02] Vector document model, mirror, and persistence {#vector-document}
<!-- revised: 2026-08-13 — CHL-0008. Re-scoped from "the document Infini edits" to "the document
     Infini keeps": serialization, persistence, and applying inbound tablet changes to a mirror.
     Same id, content revised; no supersession. -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- Infini maintains a **tree-of-vectors** library (Ink, Text, Primitive, Group, Frame,
  Connector, SmartGroup) with SVG profile serialize/parse and idempotent ops
  ([ADR-0010](../../adr/ADR-0010-tree-of-vectors.md)). Node semantics are **shared** with Epaper so
  a document means the same thing on both ends.
- **Infini is the persistence home.** It opens and saves the file, sends the initial full-document
  load to the tablet, and **applies inbound document changes** from the tablet
  ([epaper REQ-07](../epaper/prd.md#one-way-sync)) to its mirror. It does not author document
  changes of its own this iter ([REQ-04](#smart-group) deprecated).
- **Live paint today** uses the WorldLayer primitive list (`InfiniDocument`) projected on the canvas;
  the tree library is exercised by unit tests and is the target SoT for the mirror and persistence.

**Acceptance**
- Given a tree fixture with ink, text, primitives, groups, frames, and connectors, When
  Infini serializes and re-parses the SVG profile, Then ids, parenting, and geometry match
  (±1 px @ 100% zoom on the fixture set).
- Given the in-memory tree, When ops are applied by `opId`, Then duplicate `opId`s are
  idempotent and unknown types do not crash.
- Given a stream of published tablet changes, When Infini applies them in order, Then the mirror
  equals the tablet document (0 divergent nodes) and replaying the same stream twice yields the same
  tree (idempotent by `opId`).
- Given Infini’s live WorldLayer, When demo or RM ink is shown, Then figures paint under
  the current viewport with world stroke widths ([ADR-0012](../../adr/ADR-0012-world-stroke-viewport-parity.md)).
- Given a document edited entirely on the tablet, When Infini saves and reopens it, Then bounds,
  transform, `inkScaleMode`, child roles, and ink samples match (geometry ±1 px @ 100% zoom).
- **UI states (target):** doc.none / doc.open / doc.dirty / doc.error — open/save chrome
  not required for tablet-sync Must wave.
- **Structure:** Groups may nest; Frames only at document root; handwriting remains
  polyline samples (not Bézier-fitted) in v0.

## [REQ-04] Smart Group / ink-box — desktop authoring {#smart-group}
<!-- lifecycle: deprecated -->
<!-- superseded-by: [epaper REQ-05], [epaper REQ-06] -->
<!-- deprecated: 2026-08-13 — CHL-0008. Desktop-side ink-box authoring is withdrawn; the tablet
     creates and manipulates ink-boxes locally. Still works as spec'd for the library ops, but no
     new desktop authoring work until multi-directional sync lands. -->
- **Priority:** Won't (this campaign) · **Traces:** [BRD-07]
- Needs design: no *(the desktop design package [ink-box-ui](../../../.plan/iter-003/design/ink-box-ui/)
  is deprecated with [SRS-IN-14])*
- **Deprecated 2026-08-13.** Making Infini the sole tree writer is exactly what made the pilot
  fragile: the tablet's own gesture was answered, late, by the desktop. Authoring moves to the
  device.
- **Where the outcome now lives.** The *product* outcome — a handwritten cluster behaving as one
  object, ink kept as ink, no OCR — is unchanged; only its home moved:
  - Creation (enclose + selection-with-surround), guards, boundary/content roles, draw-into
    membership → [epaper REQ-05](../epaper/prd.md#device-ink-box).
  - Selection, move, resize, `inkScaleMode`, undo → [epaper REQ-06](../epaper/prd.md#device-manipulation).
  - Node semantics, schema, persistence, and the desktop mirror → [REQ-02](#vector-document).
  - Result reaching the peer → [epaper REQ-07](../epaper/prd.md#one-way-sync).
- **What Infini keeps:** it renders Smart Groups published by the device, saves and reopens them
  without loss, and offers **no** desktop tool for creating or transforming them.
- **This campaign (TRACK-004, human 2026-08-15):** leftover Infini **ToolStrip** / selection overlay
  / transform handles from STORY-IN-013 / IN-024 **must be removed** from the shipping desktop app.
  Pan/zoom and open/save chrome stay. Do not park this as backlog.
- [ADR-0011](../../adr/ADR-0011-smart-group.md) node semantics survive; its "recognition runs on
  Infini" placement and [ADR-0013](../../adr/ADR-0013-ink-box-tool-modes.md) §3 sole-writer rule are
  superseded by the architect's rework ADRs.
- **Returns when** multi-directional sync lands and a desktop edit can reach the tablet safely; a
  new REQ id will carry it (this id is not reused for the new scope).

**Acceptance**
- Given the desktop UI this campaign, When the creator looks for an ink-box or selection tool, Then
  **0** desktop authoring affordances are offered (no ToolStrip, no selection overlay, no transform
  handles) — including after TRACK-004 ships; the leftover toolbar must not remain on screen.
- Given a Smart Group created on the tablet, When Infini receives the published change, Then it
  renders boundary and content ink correctly under the current viewport (0 divergent figures for
  that region after settle).
- Given a Smart Group in the mirror, When the document is saved and reopened, Then bounds,
  transform, `inkScaleMode`, child roles, and ink samples match (geometry ±1 px @ 100% zoom).

## [REQ-03] Tablet session — one-way sync contract {#tablet-sync}
<!-- revised: 2026-08-13 — CHL-0008. Downward traffic narrowed to initial full-document load +
     viewport; document changes now flow up from the tablet. Same id, content revised; no
     supersession. -->
<!-- revised: 2026-08-20 — Always-on `viewport` down is obsolete. Initial document load stays.
     Viewport messages flow only along the active follow direction. Mutual exclusion; off on
     disconnect. Same id; no supersession. -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: no *(follow-toggle chrome is [REQ-06](#viewport-follow))*
- **One-way per direction for documents.** Infini sends the tablet exactly one **document** thing: an
  **initial full-document load** (session start, reconnect, or explicit resync). It receives
  **document changes** from the tablet and applies them to its mirror
  ([REQ-02](#vector-document)). Nothing else crosses downward as document traffic — no post-edit
  picture push, no pickable list, no correction of a gesture in flight
  ([epaper REQ-07](../epaper/prd.md#one-way-sync)). Changing that document channel is a Non-Goal.
- **Viewport is optional and one-way along follow.** Viewport messages flow **only along the active
  follow direction**: Infini → Epaper while Epaper follow is on; Epaper → Infini while Infini follow
  is on ([REQ-06](#viewport-follow) / [epaper REQ-19](../epaper/prd.md#viewport-follow)). Exactly one
  direction at a time when connected; both off is the default. Follow is **automatically off** on
  connection lost. Continuous `viewport` down is **not** the session.
- **Reconnect is safe by construction.** The mirror is current because it applied every published
  tablet change, so re-sending the full document down is always the right recovery — but only after
  the tablet's queued changes have drained. Follow does **not** restore on reconnect.
- Epaper re-rasterizes locally from its own document; Infini's job downward is the **map when
  followed**, not the picture. Stroke width is **world units** on both peers. Sync-frame orientation
  uses Reawa-style **four gut poses** (default tall: gut to the left).
- Live in-progress stroke samples may still stream up as a **preview** so the desktop feels live;
  the authoritative node arrives with the document change at pen-up.

**Acceptance**
- Given a live session, When the user draws on Epaper, Then Infini shows the in-progress stroke
  paths with p95 ≤50 ms after the RM sample, and the published node replaces the preview at pen-up
  (0 duplicate or orphaned preview paths).
- Given a live session and Epaper follow **on**, When the user pans or zooms Infini for ≥5 s, Then
  Epaper’s drawing-region map updates with p95 ≤100 ms so the next pen-down matches world
  coordinates, and Infini sends **0** document messages during the gesture (full panel redraw may
  trail; ghosting OK).
- Given a live session and Epaper follow **off**, When the user pans or zooms Infini for ≥5 s, Then
  Infini sends **0** viewport messages from that gesture and **0** document messages; the tablet map
  is unchanged.
- Given Infini follow **on**, When the tablet viewport changes, Then Infini’s canvas matches after
  settle (0 divergent viewports) and Epaper follow is off.
- Given Infini follow **off**, When the tablet viewport changes, Then Infini’s canvas is unchanged
  by that gesture (independent cameras).
- Given a session that has completed its initial load, When anything changes on the desktop, Then
  Infini sends **0** further document messages for the rest of the session.
- Given the tablet publishes a document change, When Infini applies it, Then the mirror shows the
  same result with p95 ≤300 ms after the op and 0 divergent figures for that region after settle.
- Given a reconnect with queued tablet changes, When the link returns, Then Infini ingests the whole
  queue in order (0 lost, 0 reordered) before sending a full load, after which both documents are
  equal (0 divergent nodes) and both follows are **off**.
- Given zoom-out on Infini **while Epaper follow is on**, When Epaper shows existing + new local ink,
  Then stroke thickness scales with the region (world × panel scale) — new strokes are not
  screen-constant thickness.
- Given the Sync orientation control, When the user cycles gut poses, Then tall/wide frame
  aspect and axis mapping match the chosen pose (vertical gut-to-left verified correct).

## [REQ-05] Pen-button map persist (not the editor) {#pen-button-map}
<!-- campaign: iter-005-draft — peer of epaper REQ-18 -->
<!-- ui-outcome retired: 2026-08-20 — desktop map editor is not this product. Id kept; persist/restore remains. -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: no *(no Infini settings screen; do not paint desktop map chrome)*
- **Campaign:** iter-005 **draft**. Not TRACK-004.
- **Lifecycle (this id):** still `active`. The **desktop map-editor UI** outcome is **retired in place** (human 2026-08-20). Do not delete this id.
- **Outcome:** Infini is persistence home for the barrel-button map. After the creator binds Click / Hold-move **on the tablet** ([epaper REQ-18](../epaper/prd.md#pen-buttons)), Infini **persists** that map in app settings (not the SVG) and **restores** it on a later session so the next tablet gesture uses it. Persist/publish is a **settings channel**, not a document edit, and **not** a settings surface — Infini presents **0** map-editor screens. The design package `.plan/iter-005/design/pen-button-map/` is **Epaper**, not Infini.

**Acceptance**
- Given the creator binds a map on the tablet, When Infini is connected, Then Infini persists that map (not in the document file) and sends **0** document messages for that persist.
- Given a later session with a persisted map and matching 1- or 2-button capability, When hello completes, Then the tablet’s next gesture uses that map (p95 ≤300 ms after restore) and in-flight gestures are unchanged.
- Given a 0-button pen, When Infini restores, Then **0** fake button bindings are applied.
- Given Infini has no session, When the tablet edits the map, Then the live device map still applies (persist waits; 0 lost local binds).
- Given Infini chrome, When the creator looks for barrel-button settings, Then Infini presents **0** map-editor screens (the editor is on-device).

## [REQ-06] Viewport-follow Epaper {#viewport-follow}
<!-- added: 2026-08-20 — human decision: optional mutually exclusive follow; icon toggle. -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- **Outcome:** the creator on the desktop can **opt in** to matching the connected tablet’s drawing
  region — and navigates Infini independently by default. Follow is a **choice**, not the session.
- **Affordance:** a viewport-follow **icon toggle button** on Infini. **Not** a ToolChip, not a
  hand-tool tile, not an exclusive tool on the tablet.
- **States:**
  - **Off** (default, and after disconnect): Infini camera is local ([REQ-01](#infinity-canvas));
    tablet pan does not drive Infini.
  - **Following Epaper:** Infini applies the tablet viewport ([REQ-03](#tablet-sync)).
  - **Connection lost / no session:** follow is **automatically off**. Reconnect does **not** restore
    follow.
  - **Mutual exclusion:** exactly one direction at a time when connected. Enabling Infini follow
    **disables** Epaper follow ([epaper REQ-19](../epaper/prd.md#viewport-follow)), and the reverse.
    Both off is allowed.
- A **local navigation** gesture on Infini (trackpad/mouse pan or pinch/zoom) while Infini follow is
  on **turns Infini follow off**. The canvas stays where that gesture left it. Box-unrelated: Infini
  has no tablet box-move.
- Infini→Infini follow is a Non-Goal this campaign.

**Acceptance**
- Given a live session and both follows off, When the creator enables Infini follow, Then Infini
  applies the tablet’s current viewport after settle (0 divergent viewports) and Epaper follow
  remains off (0 dual-follow).
- Given Epaper follow is on, When the creator enables Infini follow, Then Epaper follow turns off
  with p95 ≤300 ms and Infini begins following the tablet (0 intervals where both are on).
- Given Infini follow is on, When the creator pans or pinches Infini, Then Infini follow turns **off**
  and Infini navigates locally (0 continued apply of tablet viewport after that gesture starts).
- Given Infini follow is on, When the session drops, Then follow is off and stays off across
  reconnect until the creator enables it again.
- Given no session, When the creator looks at the toggle, Then it is off (or unavailable) and 0
  follow-on states persist.
- **UI states / journeys to design:** off (default); following Epaper; mutual-exclusion (peer is
  following you — enabling this side turns the other off); local Infini pan while following → off;
  connection lost → forced off; reconnect stays off. Dual-ask: `/designer` Spec + scenes; `/qa` BDD
  from this AC.

---

## Non-Goals

- **Desktop-side ink-box authoring** — no Infini tool creates or transforms a Smart Group this
  campaign ([REQ-04](#smart-group) deprecated). Authoring is on the device.
- **Pushing document changes down to the tablet** — the only inbound document message the tablet
  accepts is the initial full load ([REQ-03](#tablet-sync)).
- **Multi-directional sync / modern document-synchronization algorithm / CRDT** — explicitly
  deferred to a later campaign.
- **Always-on Infini→tablet viewport drive** — obsolete. Independent cameras by default; optional
  follow is [REQ-06](#viewport-follow).
- **Last-writer viewport token as the product model**
  ([ADR-0023](../../adr/ADR-0023-viewport-last-writer.md)) — Architect **supersedes** this ADR with
  a follow / token-optional decision. Do not implement last-writer as the UX.
- **Infini → Infini follow** — later / Non-Goal this campaign.
- **Hand-tool chip tile** on Infini — none. Follow is an icon toggle.
- **Changing the document channel to two-way** — explicit Non-Goal this campaign (tablet still
  publishes; Infini still does not author document changes down).
- Reawa pen-relay / mouse emulation features inside Infini.
- Multi-user collaborative editing; cloud sync.
- Pressure-rich brushes, layers UI, or full illustration suite.
- **Desktop pen-button map editor / settings screen** — the UI outcome of [REQ-05](#pen-button-map)
  is retired in place (human 2026-08-20). The creator configures barrel buttons **on the tablet**
  ([epaper REQ-18](../epaper/prd.md#pen-buttons)). Infini persist/restore is **not** that surface.
  Do not paint Infini slate/desktop map chrome. Package `pen-button-map/` is Epaper.
- Drawing ink on Infini with a mouse — `Pen` is an Epaper-only tool.
- Bitmap / PNG `region_refresh` as the region picture (ignored on device).
- Doc open/save chrome and tree-driven live paint (deferred beyond Must sync wave).

## Assumptions & Dependencies

- Epaper [REQ-01] local ink remains correct (Round 19 digitizer map).
- Epaper [REQ-04](../epaper/prd.md#device-document)–[REQ-07](../epaper/prd.md#one-way-sync) define
  the device side: it owns the working document and publishes changes. Barrel-button **map editor**
  is [epaper REQ-18](../epaper/prd.md#pen-buttons) (on-device). [REQ-05](#pen-button-map) is
  persist/restore only.
- USB network path RM2 ↔ desktop remains available (`RM_SYNC_HOST`, TCP `:9877`).
- Architect owns the session contract — [ADR-0009](../../adr/ADR-0009-shared-document-viewport.md)
  is amended by the ownership inversion (rework ADRs pending under
  [CHL-0008](../../../.plan/iter-003/challenges/CHL-0008-architecture-rework.md)) — and stroke
  parity ([ADR-0012](../../adr/ADR-0012-world-stroke-viewport-parity.md)).
  [ADR-0023](../../adr/ADR-0023-viewport-last-writer.md) last-writer is **not** the product model
  and must be **superseded**.

## Open Questions

- Always-on Infini→tablet viewport as the session — **owner:** pm — **closed 2026-08-20 (human):** independent cameras by default; optional mutually exclusive follow ([REQ-06](#viewport-follow)); [ADR-0023](../../adr/ADR-0023-viewport-last-writer.md) is not the product model (Architect supersedes).
- Pen-button map as Infini desktop settings — **owner:** pm — **closed 2026-08-20 (human):** UI outcome of [REQ-05](#pen-button-map) retired in place (id kept). Persist/restore remains; it is **not** the settings surface. Editor is [epaper REQ-18](../epaper/prd.md#pen-buttons).
- Document-change granularity on the wire (op log vs coalesced change set) — **owner:** architect —
  **needed by:** the rework sync ADR. Drives the ≤300 ms mirror target and the reconnect queue.
- Manual "reload document to tablet" control — **owner:** pm — **needed by:** first
  [REQ-03](#tablet-sync) story. Does Infini keep one, and what does it do to unpublished tablet
  changes?
- Exact SVG attribute grammar for Infini profile v1 — **owner:** architect — **needed by:** 2026-08-24
- Whether Infini ships macOS-first only in v0 — **owner:** pm — **needed by:** 2026-08-24

## Linked Modules

- [epaper](../epaper/prd.md) — owns the working document; publishes document changes; viewports
  independent by default; optional follow [REQ-06](#viewport-follow) /
  [epaper REQ-19](../epaper/prd.md#viewport-follow); barrel-button map editor
  [epaper REQ-18](../epaper/prd.md#pen-buttons); this module [REQ-05](#pen-button-map) persist only;
  iter-005 draft [REQ-11](../epaper/prd.md#erase)–[REQ-18](../epaper/prd.md#pen-buttons)
- [reawa](../reawa/prd.md) — sibling pen-driver product; not in this campaign scope
- Exploration: [EXP-0001](../../../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md)
