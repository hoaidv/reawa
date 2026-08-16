---
title: PRD — Epaper
module: epaper
version: 0.7.0
lifecycle: active
parent_brd: [BRD-06, BRD-07]
owner: pm
source: EXP-0001
---

# PRD — Epaper

Native drawing surface that runs **on the reMarkable 2**: pen ink renders locally
on the e-paper panel (not relayed via macOS). Sibling to desktop [Infini](../infini/prd.md);
code lives in repo-root `epaper/`.

**Docs rule (2026-08-11):** product docs describe **what ships in the Qt app today**.
The header-only `regionsync/` library is a tested target shape, not the device runtime.

**Ownership rework (2026-08-13, [CHL-0008](../../../.plan/iter-003/challenges/CHL-0008-architecture-rework.md)).**
Epaper is no longer an intent courier. It **owns the working document in-session** — it holds the
tree, ingests its own ink, recognizes and creates ink-boxes, and manipulates them locally. Infini
becomes viewer, navigator, and persistence home. Sync is one-way per direction
([REQ-07](#one-way-sync)).

## Problem & Job-to-be-Done

Artists and note-takers want the reMarkable's writing feel while using a larger
synced canvas on the desktop. Relaying every pen sample to the desktop and pushing
pixels back is too slow for local ink.

The 2026-08-11 pilot proved the same is true one level up. Ink was local, but every *edit* was not:
enclose a cluster and the box came back from the desktop; drag a box and the device drew an advisory
outline that a peer snapshot later corrected. Four consecutive fix waves
([CHL-0004](../../../.plan/iter-003/challenges/CHL-0004-fixedink-resize-boundary.md) …
[CHL-0007](../../../.plan/iter-003/challenges/CHL-0007-selection-move-enclose-sync.md)) failed human
verify with the same class of symptom — geometry snap-back, selection residue, desync on consecutive
operations. None were fixable, because the device never held the truth it was drawing.

**The job:** *when I edit on the tablet, the tablet decides, immediately, and the desktop follows.*
Epaper owns local ink **and** local document editing; the desktop is where the document is kept,
viewed at scale, and saved.

## Success Metrics

| Metric | Target | Source |
|---|---|---|
| Pen-down → local pixel (p95) | ≤ 30 ms | Manual / EXP S1 |
| Orientation / aspect match | Circle stays circle; axes match chosen gut pose | Manual |
| Drawing-region map latency | Next sample uses Infini viewport (ahead of full refresh) | Manual / EXP S3 |
| Stroke thickness under zoom | Live + rasterized ink use world × panel scale | Manual |
| Tool switch (tap → active indicator) | p95 ≤ 300 ms; no measurable ink-latency regression | Manual |
| Stroke → local document node | p95 ≤ 50 ms after pen-up; 0 strokes lost | Manual / trace |
| Enclose → Smart Group on panel | p95 ≤ 500 ms after pen-up, **peer not involved** | Manual / trace |
| Manipulation commit fidelity | Committed geometry = last previewed geometry; 0 px jump on pen-up; 0 snap-backs in a 20-gesture set | Manual QA |
| Editing works with the link down | 100% of create / move / resize / undo succeed offline | Manual QA |
| Device change → desktop mirror | p95 ≤ 300 ms after the op; 0 divergent figures after settle | Manual / trace |
| Connector recognized → visible on panel | p95 ≤500 ms after pen-up (same bar as enclose) | Manual / trace |
| Connector re-warp during bound-node drag | ≥5 Hz partial refresh; 0 full-panel invalidations; committed geometry = last previewed | Manual QA |
| Default-on recognizer false positives | ≤2% of `pen` strokes on a real corpus incl. a fresh page's first 20 strokes | EXP-0002 G1/G2 — **ship gate** |
| Connector selectable (marquee or pen hit) | 100% of recognized connectors on a 10-connector fixture; 0 missed hits on the stroke, 0 AABB-only false hits | Manual QA |
| Finger hit on ink-box → freeform + move | p95 ≤300 ms to `sel_freeform` + selection; move follows finger; 0 accidental resizes | Manual QA |

## [REQ-01] Local pen-matched ink {#local-pen-ink}
- **Priority:** Must · **Traces:** [BRD-06]
- Needs design: no
- While Epaper is running (xochitl stopped), pen strokes appear on the RM2 panel
  under the pen tip with correct aspect. Digitizer→panel uses the verified Round 19
  map (landscape digitizer on portrait framebuffer). Infini gut orientation does **not**
  change this local map — it only changes sync-frame UV for world mapping.

**Acceptance**
- Given Epaper is fullscreen on RM2, When the user draws a stroke, Then black ink
  appears along the pen path with p95 ≤30 ms pen-down → pixel (no Mac round-trip).
- Given landscape device use on a portrait panel, When the user draws a circle,
  Then the on-screen stroke is circular (not elliptical) and follows the pen
  direction.

## [REQ-02] Drawing-region mapping from Infini {#region-sync}
<!-- revised: 2026-08-13 — CHL-0008. Document transport moved to [REQ-07]; this REQ is now
     viewport/world-mapping only. Same id, content revised; no supersession. -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: no
- Infini owns pan/zoom. Epaper applies Infini's `viewport` (drawing region + gut `orientation` +
  optional `settle`) so that pen input maps to the right world coordinates. Full e-paper refresh may
  lag; **coordinate mapping for new pen input must track Infini with least latency.** Epaper
  re-rasterizes locally from its own document — it does not wait for a picture from the desktop.
  PNG `region_refresh` is **not** used.

**Acceptance**
- Given Infini changes pan/zoom, When Epaper receives the drawing region, Then the next
  pen sample (≤100 ms map apply p95) uses that world region even if the panel still
  shows a stale refresh (ghosting allowed).
- Given a settle viewport, When Epaper repaints the region, Then figures match the device's own
  document for those bounds after sharp settle (0 divergent figures).
- Given zoom-out on Infini, When the user continues drawing, Then live stroke thickness
  matches thinned existing vectors (world width × current panel scale).

## [REQ-03] On-device tool modes {#tool-modes}
<!-- revised: 2026-08-14 — BS-0001 / ADR-0021. Three exclusive tools + two recognizer
     toggles + Undo/Redo actions. Same id, content revised; [ADR-0017](../../adr/ADR-0017-four-tool-chip.md)
     superseded. -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- The creator decides **on the device** what the pen does, without reaching for the desktop.
  A minimal, always-visible toolbar:
  **Selection rect | Selection freeform | Pen ⟨space⟩ Ink-box recognition | Connector recognition ⟨space⟩ Undo | Redo**
  ([ADR-0021](../../adr/ADR-0021-connector-toolchip.md),
  [ADR-0018](../../adr/ADR-0018-undo-redo-chip-actions.md)).
  Switched by **finger touch** on the chip, so the pen stays free for content. `Pen` is the default and
  leaves [REQ-01](#local-pen-ink) local ink behaviour unchanged. Canvas **hand-touch** (finger on
  ink, not on the chip) is [REQ-10](#hand-touch).
- **Exclusive tools** (exactly three): `sel_rect` · `sel_freeform` · `pen`. There is no
  `ink_box` tool. The two Selection arms still replace a single mouse-like Selection button.
- **Recognizer toggles** (independent, not tools): `recog.ink_box` ("Ink-box recognition")
  and `recog.connector` ("Connector recognition"). Both **ship armed**. While a Selection
  tool is active they are **dimmed** (armed state kept). Tool and both toggles **latch at
  pen-down** for the whole stroke.
- Tools act on the **device's own document** ([REQ-04](#device-document)): `Pen` adds ink and,
  when a recognizer is armed, may enclose ([REQ-05](#device-ink-box)) or create a connector
  ([REQ-09](#device-connectors)); `Selection rect` / `Selection freeform` pick and manipulate
  ([REQ-06](#device-manipulation)). No tool depends on a reply from the desktop.
- Tool state is **local to the device** — Infini neither drives nor mirrors it.
- The toolbar also carries the **session/publish status** affordance for
  [REQ-07](#one-way-sync): linked, link down with changes queued, and document reloading.

**Acceptance**
- Given Epaper is running, When the creator taps an exclusive tool with a finger, Then the toolbar shows the
  new active tool with p95 ≤300 ms and the pen's next action uses that tool.
- Given the `Pen` tool, When the creator draws, Then local ink latency stays within
  [REQ-01](#local-pen-ink) (p95 ≤30 ms pen-down → pixel) — the toolbar costs no ink latency.
- Given any tool and no session, When the creator uses it, Then it behaves identically to the
  linked case (editing is local; only publishing waits).
- Given both recognizer toggles, When the creator taps one, Then it flips armed/disarmed with p95 ≤300 ms
  and does not change the exclusive tool.
- Given `sel_rect` or `sel_freeform` active, When the chip is shown, Then both recognizer toggles are
  dimmed and retain their armed state; switching back to `Pen` restores them as they were.
- Given Undo / Redo, When tapped, Then they remain **actions** (not `toolMode`) per
  [ADR-0018](../../adr/ADR-0018-undo-redo-chip-actions.md).
- **UI states / journeys to design:** three exclusive tools; two toggles armed / disarmed / dimmed;
  Undo/Redo enabled / empty no-op; publish status on the same chip; ToolChip during a trailing panel
  refresh; orientation-top placement; link down with queued changes; document reloading after reconnect.
- Given any tool, When the pen passes over the floating ToolChip, Then no ink is drawn there
  (0 stray strokes inside the chip exclusion rect; InkSurface stays full-bleed).
- Given a full-panel refresh is in flight, When the creator switches tools, Then the active-tool
  indicator is still legible (partial refresh of the chip, no dependence on the settled frame).

## [REQ-04] On-device working document {#device-document}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: no
- **Outcome:** the creator's document lives on the device while they work on it. Epaper holds the
  document tree, turns its own finished strokes into document nodes, and paints the panel **from
  that document** — so the picture on the panel never waits for, or is overruled by, a peer message.
- The device is the **only writer** of its document during a session. The desktop contributes
  exactly one document input: the initial full load ([REQ-07](#one-way-sync)).
- Node semantics are shared with Infini (Ink, SmartGroup roles, `inkScaleMode`, `layoutOffset`) so a
  document means the same thing on both ends — see [ADR-0011](../../adr/ADR-0011-smart-group.md) and
  [ADR-0010](../../adr/ADR-0010-tree-of-vectors.md).
- **Undo** is on the device, because editing is on the device: a bounded history of structural ops
  with one entry per completed gesture.
- The document is **in memory only** this iter. On-device persistence, offline work across restarts,
  and sync-any-moment are deferred (Non-Goals).

**Acceptance**
- Given Epaper is running with no session, When the creator draws 20 strokes, Then all 20 exist as
  Ink nodes in the local document and the panel paints them from that document (0 strokes lost,
  0 dependence on a peer message).
- Given a stroke ends, When the device ingests it, Then the node is in the local document with p95
  ≤50 ms after pen-up, and the *next* stroke still meets [REQ-01](#local-pen-ink) p95 ≤30 ms
  pen-down → pixel (ingestion costs no ink latency).
- Given the device receives an initial full-document load, When it applies it, Then the local
  document equals the loaded document (0 divergent nodes) and becomes the base for local edits.
- Given any structural op (create box, move, resize, draw-into), When the creator undoes it, Then
  the local document returns to the exact pre-op state (geometry ±1 px @ 100% zoom) with no peer
  involvement, for at least the last 20 structural ops.
- Given the panel repaints for any reason (settle, viewport change, refresh), When it paints, Then
  it paints the local document (0 repaints sourced from an inbound peer picture).

## [REQ-05] On-device ink-box creation {#device-ink-box}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- **Outcome:** a creator who has handwritten a thought can make that cluster behave as **one
  object** — and it happens **where they drew it**, immediately, with no round trip and no OCR.
- **Creation A — enclose.** With `Pen` and **Ink-box recognition** armed, the creator draws a shape around ink.
  The **device** recognizes a roughly rectangular stroke (recognize, do **not** convert to a
  Primitive), captures every ink whose samples are ≥80% inside, and creates a Smart Group: the
  enclose stroke is kept as `role: boundary` ink, contained ink is reparented as `role: content`,
  and `bounds` is the fitted rect. Guards: the fitted rect must be ≥ a fixed minimum size **and**
  contain ≥1 ink — otherwise the stroke stays ordinary ink. Enclosure is **rectangle-only**.
- **Creation B — selection (explicit).** The creator arms **Selection rect** or **Selection
  freeform** on the primary ToolChip ([ADR-0017](../../adr/ADR-0017-four-tool-chip.md)):
  - **Rect:** one straight drag. Thin dotted **rectangle** while dragging. Membership = ink with
    **≥80% of samples** inside the rectangle (other nodes: ≥80% of AABB area inside). A mere
    AABB touch does **not** select.
  - **Freeform:** draw around. Thin dotted **polyline** while drawing; pen-up **closes** the
    polyline. Membership = ink with **≥80% of samples** inside the closed polyline (even-odd);
    other nodes ≥80% of AABB grid inside. **Not** the AABB of the gesture.
  After pen-up, chrome is the thin dotted **tight union AABB** of selected nodes (**0** extra
  padding) + **6 square anchors**. They invoke Smart Group via an **icon-only Enclose** control on
  the selection overlay ([ADR-0016](../../adr/ADR-0016-selection-create-enclose-cta.md) — Enclose
  is **not** a fifth chip; no context-toolbar chrome; size matches primary tool buttons). The device must find **one stroke among the selected free ink** that
  surrounds almost all of the other selected free ink (≥80% of each other stroke's samples inside
  that surround stroke's region). The surround stroke may be **open**; the device builds an
  **artificial closed path** from it for the containment test only. **If no such surround stroke
  exists, creation is refused** with a visible reason — no AABB-only Smart Group. If the selection
  includes a Smart Group, Enclose is refused this campaign (no nesting —
  [CHL-0011](../../../.plan/iter-003/challenges/CHL-0011-nested-smartgroup-enclose.md)).
- **Appearance.** A Smart Group always has boundary ink after a successful create. The creator's
  surround stroke is the visual frame — never a synthetic ink rectangle.
- **Draw into an existing box.** When the creator draws with `Pen` and ≥80% of the new stroke's
  samples fall inside one or more Smart Groups' world bounds, the device parents that stroke as
  `role: content` of the matching box. If several boxes qualify, pick the one with the **highest
  paint / z order** (later siblings paint above). Adding ink never reflows, realigns, or shifts
  existing content — freehand placement as drawn.
- Recognition is **best-effort plus undo**: a wrong box costs one undo, never a stuck document.

**Acceptance**
- Given **Ink-box recognition** armed and ink on the panel, When the creator draws a closed roughly rectangular
  stroke whose fitted rect is ≥ the minimum size and contains ≥80% of that ink's samples, Then a
  Smart Group exists in the local document and is visible on the panel with p95 ≤500 ms after
  pen-up, with **0 messages required from the desktop**.
- Given the same gesture over empty canvas, or a fitted rect below the minimum size, or **Ink-box recognition disarmed**, When the stroke ends, Then no Smart Group is created and the stroke remains ordinary ink
  (0 creations on the negative fixture set).
- Given the `Selection` tool, When the creator pens down and moves, Then a thin dotted rubber-band
  follows the pen tip; on pen-up a thin dotted selection rect **tightly** equals the union AABB of
  the selected document nodes (**0** extra padding) with **6** square anchors, and an **icon-only
  Enclose** control (primary-button size, no toolbar chrome) is available on the selection overlay.
- Given the `Selection` tool and selected free ink that includes a surround stroke containing ≥80% of
  each other selected stroke's samples (open stroke OK), When the creator taps **Enclose**, Then
  a Smart Group is created with that stroke as `boundary`, the others as `content`, and `bounds`
  from the surround stroke's fitted rect (±1 px @ 100% zoom).
- Given selected ink with **no** surround stroke at the ≥80% bar, When the creator taps **Enclose**,
  Then no Smart Group is created (0 creations on the negative fixture set), the selection is
  unchanged, and the UI states the reason.
- Given a selection that includes a Smart Group, When the creator taps **Enclose**, Then creation is
  refused (0 nested boxes this campaign) and the reason is visible.
- Given the `Pen` tool and an existing Smart Group, When the creator draws a stroke with ≥80% of
  samples inside that box's world bounds, Then the stroke becomes `role: content` of that box within
  300 ms and no other content ink is translated or reflowed.
- Given nested Smart Groups that each contain ≥80% of a new `Pen` stroke, When the stroke ends, Then
  the stroke parents under the qualifying box with the highest paint/z order (0 dual-parent outcomes).
- Given **no session**, When the creator performs any creation path above, Then the result is
  identical to the linked case (100% parity across a scripted 10-gesture set).
- Given any successful create, When the creator undoes once, Then the previous document is restored
  exactly (geometry ±1 px @ 100% zoom).
- Given 10 consecutive enclose gestures, When each completes, Then each produces exactly one Smart
  Group with correct membership (0 desync, 0 lost boxes) — regression bar from
  [CHL-0007](../../../.plan/iter-003/challenges/CHL-0007-selection-move-enclose-sync.md).
- **UI states / journeys to design:** `Ink-box` armed; enclose accepted; enclose rejected (too small
  / no ink inside); `Selection` → Smart Group with surround stroke; create refused (no surround);
  draw-into an existing box; nested membership; undo of a create.

## [REQ-06] On-device ink-box manipulation {#device-manipulation}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- **Outcome:** manipulating a box on the tablet feels like moving paper, not like filing a request.
  What the creator sees under the pen **is** the document — no advisory ghost, no peer correction,
  no snap-back.
- **Pilot scope (pen).** Select by pressing a Smart Group with the **pen**; move by dragging inside
  the bounds (no prior selection needed); resize via the **6 square anchors** after explicit
  selection; toggle `inkScaleMode` (`withBounds` | `fixedInk`); deselect by pressing empty canvas.
  **Rotation is out of scope this iter** — it arrives with [REQ-08](#node-manipulation).
  **Connector attachment** is [REQ-09](#device-connectors). **Finger** select/move of a box is
  [REQ-10](#hand-touch) — not a second resize grammar.
- **`inkScaleMode` feel.** `withBounds`: content scales with the box. `fixedInk`: each content ink
  keeps its sample size fixed and tracks the box via **its own** relative offset / UV inside the box,
  so a newly drawn stroke never moves older content. Boundary ink always transforms with the frame.
- **Live and direct.** Feedback during a drag mutates the real document (not an advisory outline).
  **Paint (2026-08-14, [CHL-0018](../../../.plan/iter-003/challenges/CHL-0018-live-node-tool-canvas.md)):**
  while move/resize is in flight, the origin box is hidden on the document surface and the live node
  (ink + AABB + handles) is drawn on the selection overlay's canvas layer (ToolCanvasLayer). On
  pen-up the committed node is rasterized back onto the document surface — one settled picture.
  Mid-gesture e-ink ghosting and dirty traces are allowed; a settled frame that disagrees with the
  document is not. Painting the live node on the document surface instead (option 2) is **out of
  this campaign** — reopen only if a later rendering phase cannot keep Pen ink isolated from overlay
  refresh.
- **Below the LOD cutoff** manipulation is unavailable and the UI says so; the gesture does not
  silently do something else.
- **Forward compatibility (binding).** This REQ ships as a **conforming subset** of
  [REQ-08](#node-manipulation): SmartGroup declares `{select, move, resize, set-ink-scale-mode}`
  through the same per-node-kind capability descriptor, uses the same selection model and gizmo
  geometry, and emits a transform op whose envelope already carries a **rotation field that stays
  unset** this iter. See
  [node-manipulation](./features/node-manipulation/srs-product.md).

**Acceptance**
- Given a Smart Group and a viewport scale ≥ the LOD cutoff, When the creator drags inside its
  bounds, Then the **actual ink** follows the pen (not an outline stand-in), and on pen-up the
  committed geometry equals the last previewed geometry (0 px jump; 0 snap-backs across a scripted
  20-gesture set) — regression bar from
  [CHL-0006](../../../.plan/iter-003/challenges/CHL-0006-live-direct-resize.md) /
  [CHL-0007](../../../.plan/iter-003/challenges/CHL-0007-selection-move-enclose-sync.md).
- Given a selected Smart Group under `fixedInk`, When the creator drags a resize handle, Then
  `bounds` follow the handle, each content ink's sample size changes ≤1 px, **each** content ink's
  own UV/offset in the box is preserved (±1 px @ 100% zoom), unrelated content inks do not move, and
  the boundary ink still transforms with the frame — regression bar from
  [CHL-0004](../../../.plan/iter-003/challenges/CHL-0004-fixedink-resize-boundary.md) /
  [CHL-0005](../../../.plan/iter-003/challenges/CHL-0005-tablet-fixedink-resize-ghost.md).
- Given `withBounds`, When the creator resizes, Then content ink scales with the bounds
  (geometry ±1 px @ 100% zoom against the expected transform).
- Given a drag in progress, When the device renders feedback, Then it updates at ≥5 Hz using partial
  refresh only (0 full-panel invalidations during the gesture) and the UI never freezes for >200 ms.
  The live node is painted on ToolCanvasLayer; CanvasLayer does not keep a second copy of that box.
  Mid-gesture ghosting/dirty traces do not fail this AC; on pen-up the next settled frame shows
  **one** box at the committed geometry (0 leftover origin pixels, 0 overlay duplicate).
- Given any completed manipulation gesture, When the creator undoes once, Then exactly that gesture
  is reverted (1 undo entry per gesture; 0 partial reverts).
- Given a selected Smart Group, When the creator presses empty canvas, Then selection clears and the
  next settled frame shows 0 residual selection pixels — regression bar from
  [CHL-0007](../../../.plan/iter-003/challenges/CHL-0007-selection-move-enclose-sync.md).
- Given a viewport scale below the LOD cutoff, When the creator presses a Smart Group, Then no
  manipulation starts (0 accidental transforms) and the UI shows manipulation is unavailable.
- Given **no session**, When the creator performs any manipulation above, Then the result is
  identical to the linked case (100% parity across a scripted 10-gesture set).
- Given the [REQ-08](#node-manipulation) capability descriptor, When SmartGroup is registered under
  it, Then its declared verbs are exactly `{select, move, resize, set-ink-scale-mode}` and the
  transform op validates against the shared envelope with `rotation` unset (0 bespoke op shapes).
- **UI states / journeys to design:** `Selection` idle; Smart Group selected with handles; move in
  progress; resize in progress (both ink-scale modes); `inkScaleMode` toggle; deselect; manipulation
  unavailable below LOD; undo of a manipulation.

## [REQ-07] One-way document sync {#one-way-sync}
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: no *(status affordances are designed under [REQ-03](#tool-modes))*
- **Outcome:** the session is stable because each direction carries exactly one kind of traffic, and
  neither end has to reconcile the other's edits.
- **Desktop → Tablet — only two things:**
  1. an **initial full-document load**, at session start, on reconnect, or on an explicit resync;
  2. **pan/zoom viewport events** ([REQ-02](#region-sync)).
  Nothing else crosses. No post-edit picture push, no pickable list, no correction of a gesture in
  flight.
- **Tablet → Desktop — document changes.** The device publishes what it did to the document; the
  desktop applies them to its mirror and persists ([infini REQ-02](../infini/prd.md#vector-document)).
- **Live ink preview.** The device may keep streaming in-progress stroke samples so the desktop feels
  live; that stream is a **preview only**. The authoritative node arrives with the document change at
  pen-up, and preview never flows back to the device.
- **Load safety.** A full load **replaces** the local document, so it is only accepted at session
  start or on an explicit resync, and only after queued changes have been published. The desktop
  must not send an unsolicited load mid-session.
- **Link down is a normal state.** Editing continues locally ([REQ-04](#device-document)); changes
  queue in order and publish on reconnect.
- **Deferred by name:** a modern document-synchronization algorithm enabling multi-directional sync,
  on-device storage, and working offline then syncing at any moment. Not this iter (Non-Goals).

**Acceptance**
- Given a live session, When the creator pans or zooms on the desktop for ≥5 s, Then the tablet
  receives viewport messages only (**0** document messages) and the next pen sample uses the new
  region with p95 ≤100 ms.
- Given a live session after the initial load, When anything happens on the desktop, Then the tablet
  receives **0** inbound document messages for the rest of the session (desktop authoring is
  deprecated — [infini REQ-04](../infini/prd.md#smart-group)).
- Given the creator creates, moves, or resizes on the tablet, When the op commits, Then the desktop
  mirror shows the same result with p95 ≤300 ms after the op, and 0 divergent figures for that
  region after settle.
- Given the creator is drawing on the tablet, When samples arrive, Then Infini shows the in-progress
  path with p95 ≤50 ms after the sample, and at pen-up the preview is replaced by the published node
  (0 duplicate or orphaned preview paths).
- Given the link drops, When the creator performs 10 document operations, Then all 10 succeed
  locally and all 10 reach the desktop in order on reconnect (0 lost ops, 0 reordered ops).
- Given queued unpublished changes at reconnect, When the desktop offers a full load, Then the load
  is applied only after the queue drains (0 changes discarded by a load).
- Given a completed reconnect load, When both ends settle, Then the tablet document equals the
  desktop document (0 divergent nodes) and the tablet resumes publishing.

## [REQ-09] On-device connectors {#device-connectors}
- **Priority:** Must · **Traces:** [BRD-07] · **Campaign:** this iteration — ranked **above**
  [REQ-08](#node-manipulation), CHL-0011, CHL-0012
- Needs design: yes
- **Outcome:** the creator draws a line between two ink-boxes and it *stays* a connection —
  their ink, attached, alive when either box moves — as natural as enclosing a thought.
- **Create.** With `Pen` and **Connector recognition** armed, a stroke whose ends bind two
  different SmartGroups becomes a connector ([ADR-0022](../../adr/ADR-0022-recognizer-dispatch.md)
  fall-through). Multi-stroke chains merge at completion (no pending half-connector). One undo
  reverts the recognition. v1 targets SmartGroup only.
- **Style.** Auto-picked from the (merged) rest spine: **Ink** (`morph`) if wiggly, **Curve**
  (`cubic`) if smooth. Creator may change it later. Geometry:
  [ADR-0020](../../adr/ADR-0020-connector-ink-geometry.md).
- **Live.** Moving or resizing a bound box re-warps attached connectors during the drag
  (partial refresh). Deleting a box **keeps** the connectors; they glue back if the delete is undone.
- **Select (this campaign).** A recognized connector is a first-class selectable node, not only
  chrome that appears after an unspecified pick. The creator selects it with:
  1. **Selection rect / Selection freeform** — same membership grammar as ink/other nodes
     ([REQ-05](#device-ink-box)): ≥80% of the connector's **path samples** (not its AABB) inside
     the rectangle or closed polyline.
  2. **Pen-touch** — press the connector **stroke** (hit-test along the path with a generous
     on-panel tolerance). AABB-only press of the connector's bounding box does **not** select it
     (avoids stealing box hits).
  Selected chrome (Ink/Curve; per-end Edge/Centre) is already in
  [connector-ink srs-ui](./features/connector-ink/srs-ui.md). Re-anchor as a generic verb stays
  [REQ-08](#node-manipulation).
- **Not this REQ.** Squared/rounded routing, arrowheads, dash/double-line, non-SmartGroup
  targets — later. Full product depth:
  [features/connector-ink/srs-product.md](./features/connector-ink/srs-product.md).

**Acceptance**
- Given two SmartGroups and Connector recognition armed, When the creator draws an open stroke
  from near A into C, Then a connector exists in the local document and is visible on the panel
  with p95 ≤500 ms after pen-up, **0 desktop messages required**, and one undo restores the
  original ink.
- Given the same stroke with Connector recognition **disarmed**, When the stroke ends, Then it
  remains ordinary ink (0 connectors).
- Given a connector, When the creator moves or resizes a bound box, Then the connector stays
  attached, re-warps live at ≥5 Hz with 0 full-panel invalidations, and on pen-up committed
  geometry equals the last previewed pose (0 px jump).
- Given a connector, When the bound box is deleted, Then the connector remains drawn; When that
  delete is undone, Then the connector glues back to the restored box (same id).
- Given a scripted 20-gesture set on shared fixtures, When both ends settle, Then device and
  desktop connector geometry match (0 divergent nodes) — [REQ-07](#one-way-sync).
- Given both recognizers armed as shipped, When a real writing corpus (incl. a fresh page's
  first 20 strokes) is replayed, Then unintended connector or enclose creations are ≤2% of
  `pen` strokes — **ship gate**.
- Given `sel_rect` or `sel_freeform` and a recognized connector, When the marquee/lasso contains
  ≥80% of that connector's path samples, Then the connector is in the selection (Ink/Curve chrome
  visible). Given the same gesture that only intersects the connector AABB with <80% of samples,
  Then the connector is **not** selected.
- Given `Pen` (or any tool) and a recognized connector, When the creator **pen-downs on the
  connector stroke**, Then that connector is selected with p95 ≤300 ms. Given a pen-down inside
  the connector AABB but off the stroke, Then the connector is not selected (box or empty-canvas
  rules apply).
- **UI states / journeys to design:** recognition blink (connector + both nodes, once);
  selected connector (Ink/Curve; per-end Edge/Centre); dimmed toggles under Selection;
  refuse/no-op (stroke stays ink, no banner); marquee vs path-hit vs AABB-miss.

## [REQ-10] Hand-touch on canvas (first slice) {#hand-touch}
<!-- added: 2026-08-15 — start adopting capacitive finger on the document, not only ToolChip -->
- **Priority:** Must · **Traces:** [BRD-07] · **Campaign:** this iteration, after connector select
- Needs design: yes
- **Outcome:** the creator can pick up and move an ink-box with a **finger** without first
  hunting a Selection chip, while **fine chrome** stays pen-only. The pen remains the precision
  instrument; the hand is for coarse, large targets.
- **Hit-test → freeform.** A finger press whose hit is an **ink-box** (Smart Group world bounds,
  at/above the LOD cutoff) **selects that box** and **switches the exclusive tool to
  `sel_freeform`**. The chip updates with the same p95 ≤300 ms bar as [REQ-03](#tool-modes).
  Finger on empty canvas does not switch tools and does not start a lasso (no accidental
  selection while resting a palm — architect may keep the existing palm/reject filter).
- **Move with finger.** Once a box is selected (by finger hit, or already selected), a finger
  drag **inside the bounds** moves the box with the same live-direct contract as
  [REQ-06](#device-manipulation) (actual ink follows; 0 px jump on lift). Finger may also start
  a move on the same down that hit the box (no extra tap required).
- **No subtle manipulation for hand-touch.** Finger does **not** drive the **6 square anchors**
  or any control whose hit target is **< 64 du**. Resize stays **pen**. Enclose and ToolChip
  tiles are 64 du → still finger-eligible under the size rule.
- **Size rule (simple).** Finger may hit a control only if its **hit target is ≥ the primary
  ToolChip tile**. That tile is **64×64 du** ([CHL-0019](../../../.plan/iter-004/challenges/CHL-0019-toolchip-tile-size.md);
  32 px was verified too small on RM2). Handles are 28 visual / 56 hit → **finger-ineligible**.
  ToolChip tiles, Undo/Redo, recognizer toggles, and Enclose (64 du) stay finger-eligible.

**Acceptance**
- Given `Pen` active and a Smart Group at/above LOD, When the creator **finger-downs inside
  the box**, Then the exclusive tool becomes `sel_freeform`, the box is selected, and the chip
  shows freeform, all with p95 ≤300 ms.
- Given that finger-down (or a following finger drag) inside the selected box, When the finger
  moves, Then the box follows with the [REQ-06](#device-manipulation) live-direct bar (0 px jump
  on lift; ≥5 Hz partial refresh).
- Given a selected Smart Group, When the creator **finger-downs on a resize anchor** (or any
  control whose hit target is **< 64 du**), Then **no** resize/scale-mode/end-kind gesture
  starts (0 accidental transforms). Pen on the same anchor still resizes.
- Given finger-down on empty canvas (no box hit), When the touch ends, Then the exclusive tool
  is unchanged and 0 nodes are selected by that touch.
- Given finger-down on a ToolChip primary tile (64 du), When the tap completes, Then
  [REQ-03](#tool-modes) still holds (tool/toggle/undo) — this REQ does not steal chip hits.
- **UI states / journeys to design:** finger hit box while `Pen`; finger move in progress;
  finger on anchor no-op; finger empty canvas no-op; mixed pen-resize after finger-select.

## [REQ-08] Direct manipulation of any document node {#node-manipulation}
- **Priority:** Should · **Traces:** [BRD-07] · **Campaign:** distinct iteration — thickened now,
  designed and built after the [REQ-04](#device-document)…[REQ-07](#one-way-sync) wave is verified
- Needs design: yes
- **Outcome:** anything the creator can see on the panel, they can manipulate — with one coherent
  vocabulary, so learning to move a Smart Group teaches them how to move text, a primitive, a frame,
  or a connector. The bar is the depth of a modern vector drawing app, not a one-off box handler.
- **Shared verbs.** ~99% of node kinds support the same base set: select, multi-select, marquee,
  enter/exit group, move, resize (corner and side, aspect lock), **rotate with snap increments**,
  nudge, duplicate, delete, z-order, align/distribute, transform origin, snapping and guides,
  modifier constraints.
- **Distinct tools.** Some kinds add their own: Ink (ink-scale mode, simplify, split), Text (edit
  content, resize vs reflow, baseline), Primitive (endpoints, corner radius), Connector (re-anchor
  endpoints, routing), Frame (artboard resize, clip), SmartGroup (boundary vs content, enter/exit).
- **Extensibility contract.** Each node kind declares, through a **capability descriptor**, which
  shared verbs it supports and which bespoke tools it adds. The manipulation host reads the
  descriptor; adding a node kind must not require editing the host.
- Full product depth — verb semantics, per-kind tools, gesture grammar, e-ink constraints, and the
  descriptor schema — lives in
  [features/node-manipulation/srs-product.md](./features/node-manipulation/srs-product.md).
- [REQ-06](#device-manipulation) is the first conforming citizen of this model and must not require
  rework when this REQ lands.

**Acceptance**
- Given the capability descriptor, When a new node kind is registered declaring the shared verbs,
  Then it gains selection, move, and resize with **0 changes** to the manipulation host.
- Given any two node kinds that both declare `move` / `resize` / `rotate`, When the creator
  manipulates either, Then the gesture grammar and gizmo vocabulary are identical (1 shared model;
  0 per-kind special cases for shared verbs).
- Given the node kinds in the document model, When this REQ ships, Then ≥90% support the full shared
  verb set and every exception is documented with its reason in the capability descriptor.
- Given SmartGroup as shipped under [REQ-06](#device-manipulation), When this REQ lands, Then
  SmartGroup requires 0 changes to its declared capabilities and 0 changes to its transform op shape.
- Given a node with `rotate`, When the creator rotates with snapping active, Then the angle lands on
  the snap increment within ±0.5° and one undo restores the previous transform exactly.
- **UI states / journeys to design:** multi-select and marquee; enter/exit group; rotate with snap;
  per-kind tool surface for a selected node; align/distribute on a multi-selection; nudge; delete
  and duplicate; snapping guides; manipulation unavailable below LOD; conflicting capabilities in a
  mixed multi-selection.

---

## Non-Goals

- **On-device pan / zoom / pinch** on the Epaper UI (deferred). Finger on canvas is **not** a
  pan — [REQ-10](#hand-touch) is hit-test / move only.
- **Finger resize, rotation, or connector re-anchor** — Won't this slice. Fine gizmos stay pen.
- Acting as a Reawa-style mouse/stylus driver for other Mac apps.
- Cloud sync or multi-peer sessions.
- **On-device persistence, offline work across app restarts, and sync-at-any-moment** — the device
  document is in memory for the session; the desktop is the persistence home
  ([REQ-07](#one-way-sync)).
- **Multi-directional sync / modern document-synchronization algorithm / CRDT** — explicitly deferred
  to a later campaign; this iter is one-way per direction.
- **Desktop-authored document changes reaching the tablet mid-session** — the only inbound document
  message is the initial full load.
- **Rotation on a Smart Group this iter** — arrives with [REQ-08](#node-manipulation).
  Connector attachment is [REQ-09](#device-connectors).
- **Automatic (unprompted) creation** — every Smart Group or connector still requires an
  **armed recognizer** (or an explicit selection command). Recognizers **ship armed**;
  "ordinary ink forever in Pen mode" is retired.
- **In-box content alignment / reflow** (left/center/right, baseline snap, auto-padding when
  appending ink) — freehand placement and `fixedInk` UV tracking only.
- **OCR / handwriting-to-Text** — the ink-box captures **any** ink inside the enclosure; there is no
  "is this text?" gate.
- Non-rectangular enclosure shapes (ellipse, lasso) — `bounds` is axis-aligned.
- **Nested enclose (Smart Groups capturing other Smart Groups)** — this campaign's enclose captures
  **free top-level ink only**. Capturing whole ink-boxes as content (nested ink-boxes) is adopted
  product intent for a **later campaign** —
  [CHL-0011](../../../.plan/iter-003/challenges/CHL-0011-nested-smartgroup-enclose.md). Draw-into
  membership into an existing box (flat parent) remains in [REQ-05](#device-ink-box) now.
- **Ink-box sizing `FREE_FORM` / `WRAP_CONTENT` and `align-content`** — adopted product intent for a
  **later campaign** ([CHL-0012](../../../.plan/iter-003/challenges/CHL-0012-inkbox-sizing-align.md)):
  - `FREE_FORM`: w/h required (first create from boundary-ink); optional `align-content`
    TOP|RIGHT|BOTTOM|LEFT for **content-ink** only (not boundary).
  - `WRAP_CONTENT`: w/h from content-ink; no `align-content`; bounds may **auto-expand** on
    draw-into.
  - This campaign keeps `inkScaleMode` (`withBounds` | `fixedInk`) only; membership does **not**
    expand bounds; no content align/reflow.
- A general on-device tool palette — the toolbar is exactly the three exclusive tools,
  two recognizer toggles, and Undo/Redo in [REQ-03](#tool-modes); no brushes, colors,
  layers, or document browser.
- Production use of `regionsync/` `append_ink` NetSink until wired into the Qt binary
  (library remains the future ADR-0009 shape).

## Assumptions & Dependencies

- Infini [REQ-01]–[REQ-03] define the desktop side of the session; Infini is viewer + navigator +
  persistence home, and its own ink-box authoring is deprecated
  ([infini REQ-04](../infini/prd.md#smart-group)).
- Node semantics are shared with Infini: [ADR-0010](../../adr/ADR-0010-tree-of-vectors.md) tree,
  [ADR-0011](../../adr/ADR-0011-smart-group.md) Smart Group. Where recognition and writes happen is
  re-decided by the architect under CHL-0008 (ADR-0014 / ADR-0015 pending).
- Consistency: [ADR-0009](../../adr/ADR-0009-shared-document-viewport.md) is amended by the
  ownership inversion — the device is the sole in-session writer.
- Stroke paint: [ADR-0012](../../adr/ADR-0012-world-stroke-viewport-parity.md).
- RM2 capacitive touch is reachable from the Qt app (verified by the STORY-EP-004 spike).
- The device has enough memory and CPU headroom to hold the document and hit-test it without
  regressing [REQ-01](#local-pen-ink) ink latency — **architect to confirm before the first
  [REQ-04](#device-document) story.**

## Open Questions

- **Next campaign (iter-005) — parked, no REQs yet.** Human 2026-08-15 list in [BS-0002](../../../.plan/iter-004/brainstorms/BS-0002-iter-005-feature-wave.md): eraser (pencil + selection-erase); copy/cut/paste; connector endpoint styles (context toolbar + preserve endpoint ink); connector mid-attachments that follow warp; table recognition; finger pan/zoom on tablet; manual frame + beautiful connectors/attachments/primitives (ink-box already done); AI unspecified. Finger pan/zoom still a **Non-Goal** / [BRD-07](../../brd.md) defer until PM adopts. Author REQs only after iter-004 retro-gate.

- Undo depth and affordance on the device — **closed 2026-08-14** ([CHL-0016](../../../.plan/iter-003/challenges/CHL-0016-undo-redo-toolbar.md)
  / [ADR-0018](../../adr/ADR-0018-undo-redo-chip-actions.md)): depth 20; on-panel Undo and Redo after
  a gap on the primary strip. Exclusive tools are three as of [ADR-0021](../../adr/ADR-0021-connector-toolchip.md).
- Document-change granularity on the wire (op log vs coalesced change set) — **owner:** architect —
  **needed by:** ADR-0015. Affects the ≤300 ms mirror target and the reconnect queue.
- Manual "reload document to tablet" control — **owner:** pm — **needed by:** first
  [REQ-07](#one-way-sync) story. Does the desktop keep one, and what does it do to unpublished
  tablet changes?
- Multi-document / document switching on the device — **owner:** pm — assumed out of scope this
  iter; confirm before [REQ-04](#device-document) is sliced.
- Finger-eligible control size vs 32 vs 64 — **closed 2026-08-15 (pm):** primary tile is **64 du**
  (CHL-0019). Hand-touch rule: hit target **< 64 du** → pen only ([REQ-10](#hand-touch)).
- Minimum fitted-rect size for enclose — **closed 2026-08-15 (pm):** adaptive **28** with content, **36** empty + primitive-shape gate.
- LOD cutoff for on-device manipulation — **closed 2026-08-13 (architect).** Unavailable when the
  selected box's smaller **on-panel** axis is **< 96 du** (not `TILE_LOD_SCALE = 0.35`). Handle
  visual **28 du** / hit **56 du**. Binding: [SRS-EP-11](./features/ink-box/srs-logic.md) /
  [SRS-EP-12](./features/ink-box/srs-ui.md).
- Live node paint during move/resize (CanvasLayer vs ToolCanvasLayer) — **closed 2026-08-14
  (pm)** ([CHL-0018](../../../.plan/iter-003/challenges/CHL-0018-live-node-tool-canvas.md)):
  option 1 (ToolCanvasLayer) adopted; option 2 deferred to a later rendering phase.

## Linked Modules

- [infini](../infini/prd.md)
- [EXP-0001](../../../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md)
