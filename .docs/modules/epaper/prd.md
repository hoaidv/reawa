---
title: PRD — Epaper
module: epaper
version: 0.11.0-draft
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
becomes viewer, navigator, and **document** persistence home. Sync is one-way per direction
([REQ-07](#one-way-sync)). **Device Settings** persist on the Epaper device
([REQ-20](#device-settings)) — not Infini, not the document.

**Viewport follow (2026-08-20, human).** Epaper and Infini cameras are **independent by default**.
Always-on Infini→tablet viewport drive is **obsolete**. Optional one-way **follow**
([REQ-19](#viewport-follow) / [infini REQ-06](../infini/prd.md#viewport-follow)) is mutually
exclusive and off on disconnect. The tablet **can** change its own viewport ([REQ-10](#hand-touch)).
Document channel stays one-way (change later is a Non-Goal).

**Device Settings (2026-08-20, human).** Device-level preferences (example this campaign: the
pen-button map) are configured **on the tablet**, saved **on the Epaper device**, and are **not**
Infini settings and **not** a document setting. There is no document-settings product this campaign.
The Settings page is master-detail; first (only this package) master item is **Pen buttons**
([REQ-20](#device-settings) / [REQ-18](#pen-buttons)). Infini [REQ-05](../infini/prd.md#pen-button-map)
persist/restore is **retired**.

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
| Drawing-region map latency | **While Epaper follow is on:** next sample uses Infini viewport (ahead of full refresh). **Follow off (default):** Infini pan does not change the tablet map | Manual / EXP S3 |
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
| Two-finger pan/zoom on tablet | Next pen sample uses **local** new region p95 ≤100 ms. Infini view matches after settle **only if Infini follow is on**. Default: independent. BRD-07 ship gate **lifted** | Manual QA — REQ-10 |
| One-finger empty-canvas pan | Travel **> 10 mm** pans **locally**; palm-rest (**≤ 10 mm**) 0 pan / 0 selection; box hit wins over pan | Manual QA — REQ-10 |
| Device Settings persist | After Epaper restart on the same device, next barrel gesture uses the last map; 0 Infini copies; 0 SVG copies | Manual QA — REQ-20 |
| Viewport-follow toggle | Enabling one peer’s follow disables the other with p95 ≤300 ms; 0 dual-follow; disconnect forces off | Manual QA — REQ-19 |
| Erase stroke / selection-erase | p95 ≤50 ms after gesture end; 1 undo restores; 0 accidental ink | Manual QA — **iter-005 draft** |
| Paste fidelity | Pasted subtree geometry ±1 px @ 100% zoom vs source | Manual QA — **iter-005 draft** |
| Connector end style + warp | Style survives bound-node drag; endpoint ink stays on the end (0 orphaned ink) | Manual QA — **iter-005 draft** |
| Mid-attachment follows warp | Attachment stays on spine; 0 px jump on pen-up vs last preview | Manual QA — **iter-005 draft** |
| Barrel button hold vs click | 0 click+hold double-fires on a 20-gesture fixture; missing buttons no-op | Manual QA — **iter-005 draft** |

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
<!-- revised: 2026-08-20 — Human: independent viewports by default. Apply Infini viewport only
     while Epaper follow ([REQ-19]) is on. Same id: outcome remains mapping pen samples to world;
     follow is the gate. Always-on Infini→tablet drive is obsolete. -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: no
- **Default: local camera.** Epaper maps pen samples through **its own** viewport. Infini pan/zoom
  does **not** change that map while follow is off.
- **While Epaper follow is on** ([REQ-19](#viewport-follow)): Epaper applies Infini's drawing region
  (and the gut orientation / settle that travel with that region) so pen input maps to the right
  world coordinates. Full e-paper refresh may lag; **coordinate mapping for new pen input must track
  the active source with least latency** — Infini while following, otherwise the local camera.
- Epaper re-rasterizes locally from its own document — it does not wait for a picture from the
  desktop. PNG `region_refresh` is **not** used.

**Acceptance**
- Given Epaper follow is **on** and Infini changes pan/zoom, When Epaper receives the drawing region,
  Then the next pen sample (≤100 ms map apply p95) uses that world region even if the panel still
  shows a stale refresh (ghosting allowed).
- Given Epaper follow is **off** (default), When Infini pans or zooms for ≥5 s, Then the tablet map
  is unchanged (0 Infini-driven region changes) and the next pen sample uses the local camera.
- Given a settle viewport **while following**, When Epaper repaints the region, Then figures match
  the device's own document for those bounds after sharp settle (0 divergent figures).
- Given zoom-out on Infini **while following**, When the user continues drawing, Then live stroke
  thickness matches thinned existing vectors (world width × current panel scale).

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
  ink, not on the chip) is [REQ-10](#hand-touch). Viewport-follow is a **separate icon toggle**
  ([REQ-19](#viewport-follow)) — **not** a ToolChip exclusive tool, recognizer, or hand-tool tile.
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
- The **document** is **in memory only** this iter. On-device **document** persistence, offline work
  across restarts, and sync-any-moment are deferred (Non-Goals). **Device Settings** are not the
  document — they persist on this device ([REQ-20](#device-settings)). No document settings.

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
<!-- revised: 2026-08-13 — CHL-0008. Downward traffic narrowed; document changes flow up. -->
<!-- revised: 2026-08-20 — Viewport down is follow-gated ([REQ-19]); document channel still
     one-way. Same id; no supersession. -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: no *(status affordances are designed under [REQ-03](#tool-modes))*
- **Outcome:** the session is stable because each direction carries exactly one kind of traffic, and
  neither end has to reconcile the other's edits.
- **Desktop → Tablet — document vs viewport:**
  1. an **initial full-document load**, at session start, on reconnect, or on an explicit resync
     (**only** inbound document traffic);
  2. **pan/zoom viewport events** ([REQ-02](#region-sync)) **only while Epaper follow is on**
     ([REQ-19](#viewport-follow)).
  Viewport is **not** a document message. It flows **only along the active follow direction**
  (Epaper following Infini → down; Infini following Epaper → up). Always-on viewport drive is
  obsolete. Nothing else crosses downward. No post-edit picture push, no pickable list, no
  correction of a gesture in flight. Document channel stays **one-way** tablet → desktop (changing
  that is a Non-Goal).
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
- Given a live session and Epaper follow **on**, When the creator pans or zooms on the desktop for
  ≥5 s, Then the tablet receives viewport messages (**0** document messages) and the next pen sample
  uses the new region with p95 ≤100 ms.
- Given a live session and Epaper follow **off**, When the creator pans or zooms on the desktop for
  ≥5 s, Then the tablet receives **0** viewport messages from that gesture and **0** document
  messages; the local map is unchanged.
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
- **Not this REQ.** Squared/rounded routing, dash/double-line, non-SmartGroup
  targets — later. **Endpoint styles / arrowheads / endpoint ink** → [REQ-13](#connector-ends).
  **Mid-attachments** → [REQ-14](#connector-attachments). Full product depth:
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

## [REQ-10] Hand-touch on canvas {#hand-touch}
<!-- added: 2026-08-15; revised: 2026-08-16 — merged [REQ-16] two-finger pan/zoom into this REQ; revised: 2026-08-20 — CHL-0024 finger resize knobs; revised: 2026-08-20 — two-finger local Must (BRD-07 ship gate lifted); one-finger empty = local pan; publish only if Infini is following; revised: 2026-08-20 — pan threshold is 10 mm (human lock for STORY-EP-054) -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- **Campaign:** **one grammar, two slices.** One-finger pick/move/resize **and** two-finger **local**
  pan/zoom are both Must this iteration. The BRD-07 deferral is **not** a ship gate for **local**
  pan (human 2026-08-20). [REQ-16](#device-pan-zoom) is **retired** — superseded by this section.
- **Outcome:** the **hand** is how the creator moves around the page and shoves large objects; the
  **pen** stays the precision instrument. One coherent capacitive grammar — not a separate “pan
  product” and “hit-box product.” The tablet camera is **local**. Infini matches it **only** while
  Infini follow is on ([infini REQ-06](../infini/prd.md#viewport-follow)).
- **One finger — pick, move, resize knobs, or empty-canvas pan.** A finger press whose hit is an
  **ink-box** (Smart Group world bounds, at/above the LOD cutoff) **selects that box** and
  **switches the exclusive tool to `sel_freeform`**. The chip updates with the same p95 ≤300 ms bar
  as [REQ-03](#tool-modes). A finger-down whose hit is a **resize knob** (hit target ≥ primary
  ToolChip tile) **resizes** with the same live-direct contract as pen on that knob; the knob wins
  over box-move. Ink-scale mode still applies ([REQ-06](#device-manipulation)). Once a box is
  selected (by this hit, or already selected), a finger drag **inside the bounds** moves it with the
  [REQ-06](#device-manipulation) live-direct contract. The same down that hits the box may start the
  move.   **Empty canvas (no box, knob, or chip hit):** the touch does not switch tools and does not
  start a lasso. The pan threshold is **10 mm** Euclidean panel travel from finger-down. If movement
  stays **at or below 10 mm**, it is palm-rest / tap — **no pan, no selection, no tool switch**. If
  movement goes **past 10 mm**, it is **local one-finger pan**. Box / knob / chip hit always wins
  over empty-canvas pan. Do not invent extra hand-touch chrome for this rule.
- **Two fingers — local pan and zoom.** Two-finger pan/pinch changes the **tablet’s local viewport**.
  The device **publishes** that viewport **only if Infini follow is on**. Does not run while a
  one-finger box-move **or resize** is in flight. Link down: local viewport still works; follow
  auto-off ([REQ-19](#viewport-follow)).
- **Follow vs local navigation.** A local navigation gesture (one-finger empty pan past threshold,
  or two-finger pan/pinch) on a tablet that is following Infini **turns Epaper follow off**. Box
  pick/move/resize does **not** turn follow off.
- **Size rule.** Finger may hit a control only if its **hit target is ≥ the primary ToolChip tile**
  (**64×64 du**, [CHL-0019](../../../.plan/iter-004/challenges/CHL-0019-toolchip-tile-size.md)).
  Resize knobs meet that floor (visual may stay a small hollow square). Controls still under the
  floor (rotation, connector end-kind, …) stay **pen**. Enclose and ToolChip tiles remain
  finger-eligible.

**Acceptance**
- Given `Pen` active and a Smart Group at/above LOD, When the creator **finger-downs inside
  the box**, Then the exclusive tool becomes `sel_freeform`, the box is selected, and the chip
  shows freeform, all with p95 ≤300 ms.
- Given that finger-down (or a following finger drag) inside the selected box, When the finger
  moves, Then the box follows with the [REQ-06](#device-manipulation) live-direct bar (0 px jump
  on lift; ≥5 Hz partial refresh) and **0** viewport pan starts.
- Given a selected Smart Group, When the creator **finger-downs on a resize knob** (hit ≥ primary ToolChip tile), Then resize starts with the [REQ-06](#device-manipulation) live-direct bar (0 px jump on lift; ≥5 Hz partial refresh) and **0** viewport pan. Pen on the same knob still resizes.
- Given a control whose hit target is **< 64 du** (not a resize knob), When the creator finger-downs on it, Then **no** scale-mode/end-kind/rotation gesture starts (0 accidental transforms).
- Given **one** finger-down on empty canvas (no box, knob, or chip hit) and movement **≤ 10 mm**,
  When the touch ends, Then the exclusive tool is unchanged, 0 nodes are selected, and **0** pan
  occurs (palm rest / tap — 0 accidental lassos).
- Given **one** finger-down on empty canvas (no box, knob, or chip hit) and movement **> 10 mm**,
  When the finger moves, Then the **local** viewport pans, the exclusive tool is unchanged, 0 nodes
  are selected, and 0 lasso starts. Infini’s view matches after settle **only if Infini follow is
  on**; otherwise Infini is unchanged.
- Given a box, knob, or chip hit, When the same one-finger drag would otherwise pan empty canvas,
  Then pick/move/resize/chip wins (0 empty-canvas pan).
- Given **two** fingers on empty canvas (no box-move or resize in flight), When the creator pans or
  pinches for ≥5 s, Then the **local** drawing region translates/scales with p95 map apply ≤100 ms
  for the next pen sample. Infini’s view matches after settle **only if Infini follow is on**
  (0 divergent viewports in that case); if Infini follow is off, Infini’s view is unchanged
  (independent cameras).
- Given Infini follow is **off**, When the creator pans on the tablet, Then Infini sends **0**
  viewport messages and does not move its canvas from that gesture.
- Given Infini follow is **on**, When the creator pans on the tablet, Then Infini applies the
  published region after settle (0 divergent viewports) and Epaper follow is off (0 dual-follow;
  0 competing Infini→tablet viewport).
- Given Epaper follow is **on**, When the creator starts a local navigation gesture (one-finger empty
  pan past threshold or two-finger pan/pinch), Then Epaper follow turns **off** and the gesture
  drives the local camera.
- Given finger-down on a ToolChip primary tile (64 du), When the tap completes, Then
  [REQ-03](#tool-modes) still holds — this REQ does not steal chip hits.
- **UI states / journeys to design:** finger hit box while `Pen`; finger move in progress;
  finger resize in progress; one-finger empty **palm-rest no-op**; one-finger empty **local pan**;
  two-finger pan in progress (local; publish only if Infini following); pinch; pan vs box-move
  conflict; link down (local viewport). Follow-toggle chrome is **[REQ-19](#viewport-follow)**, not
  this package’s ToolChip.

## [REQ-19] Viewport-follow Infini {#viewport-follow}
<!-- added: 2026-08-20 — human decision: optional mutually exclusive follow; icon toggle, not a
     ToolChip hand-tool tile. -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- **Outcome:** the creator on the tablet can **opt in** to matching Infini’s drawing region — and
  works on an independent camera by default. Follow is a **choice**, not the session.
- **Affordance:** a viewport-follow **icon toggle button** on Epaper. **Not** a ToolChip exclusive
  tool, recognizer, or hand-tool tile ([REQ-03](#tool-modes) stays three exclusive tools).
- **States:**
  - **Off** (default, and after disconnect): local camera; Infini pan does not drive the tablet
    ([REQ-02](#region-sync)).
  - **Following Infini:** apply Infini viewport ([REQ-02](#region-sync)).
  - **Connection lost / no session:** follow is **automatically off**. Reconnect does **not** restore
    follow — the creator must opt in again.
  - **Mutual exclusion:** exactly one direction at a time when connected. If Infini is following
    this tablet ([infini REQ-06](../infini/prd.md#viewport-follow)), Epaper follow cannot stay on.
    Enabling Epaper follow **disables** Infini follow (and the reverse). Both off is allowed.
- Infini→Infini follow is a Non-Goal this campaign.

**Acceptance**
- Given a live session and both follows off, When the creator enables Epaper follow, Then the tablet
  applies Infini’s current viewport with p95 map ≤100 ms and Infini follow remains off (0 dual-follow).
- Given Infini follow is on, When the creator enables Epaper follow, Then Infini follow turns off
  with p95 ≤300 ms and Epaper begins following Infini (0 intervals where both are on).
- Given Epaper follow is on, When the session drops, Then follow is off before the next gesture and
  stays off across reconnect until the creator enables it again.
- Given no session, When the creator looks at the toggle, Then it is off (or unavailable) and 0
  follow-on states persist.
- **UI states / journeys to design:** off (default); following Infini; mutual-exclusion (peer is
  following you — enabling this side turns the other off); connection lost → forced off; reconnect
  stays off; placement vs ToolChip (icon toggle, **not** a fourth exclusive tool). Dual-ask:
  `/designer` Spec + scenes for those states; `/qa` BDD from this AC.

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

## [REQ-11] Erase like paper {#erase}
<!-- campaign: iter-005-draft — BS-0002. Not TRACK-004. Do not slice until iter-004 retro-gate. -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- **Campaign:** iter-005 **draft**. Not in the current lock.
- **Outcome:** a mark the creator no longer wants is gone the way pencil paper works — flip or stroke it away, or delete what is already selected — without a round trip.
- **Path A — hardware eraser nib** (when the stylus reports a distinct eraser tool): rubbing with that nib removes intersecting **ink samples** (and may delete a node that has no remaining samples). Does not start a new ink stroke.
- **Path B — selection-erase:** with a non-empty selection, an **Erase** command (chip, barrel-click if bound, or equivalent) deletes the selected nodes. One undo restores them.
- Barrel **hold-move = temporary erase** is an accelerator of Path A’s stroke-erase feel, bound via [REQ-18](#pen-buttons) — not a third grammar.

**Acceptance**
- Given a stylus with an eraser nib and ink on the panel, When the creator rubs the nib across that ink, Then intersecting samples are gone with p95 ≤50 ms after the gesture ends, **0** new Ink nodes are created, and one undo restores the pre-erase document (±1 px @ 100% zoom).
- Given a stylus **without** an eraser nib, When the creator uses the pen tip, Then Path A does not fire (0 accidental erases); erase is Path B and/or a bound [REQ-18](#pen-buttons) hold-move.
- Given a non-empty selection, When the creator invokes Erase, Then every selected node is removed from the local document (0 leftovers on the next settled frame) and one undo restores them.
- Given empty selection, When Erase is invoked, Then 0 nodes change (no-op).
- Given **no session**, When any erase path runs, Then the result matches the linked case.
- **UI states / journeys to design:** eraser-nib in progress; selection-erase CTA; empty selection no-op; undo after erase; missing nib.

## [REQ-12] Copy, cut, and paste on the device {#clipboard}
<!-- campaign: iter-005-draft — BS-0002 -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- **Campaign:** iter-005 **draft**.
- **Outcome:** a cluster that is already right can be duplicated or moved as a copy **on the tablet**, without redrawing and without the desktop clipboard.
- **Scope v1:** in-document clipboard only (copy/cut selected nodes; paste into the current document, offset so the paste is visible). One clipboard slot. Cut = copy + delete. Paste is one undoable op. Cross-app / OS paste is out.

**Acceptance**
- Given a non-empty selection, When the creator copies then pastes, Then a new subtree exists with new ids, geometry equal to the source translated by a documented offset (±1 px @ 100% zoom), and the source is unchanged.
- Given a non-empty selection, When the creator cuts then pastes, Then the originals are gone after cut and the paste matches the cut content (geometry ±1 px @ 100% zoom); one undo of paste removes the copies; a second undo restores the cut originals.
- Given an empty clipboard, When paste is invoked, Then 0 nodes change.
- Given **no session**, When copy/cut/paste runs, Then behaviour matches the linked case; published ops still satisfy [REQ-07](#one-way-sync) when the link is up.
- **UI states / journeys to design:** copy/cut/paste affordances on selection; empty clipboard; paste offset visible; undo stack after cut+paste.

## [REQ-13] Connector endpoint styles {#connector-ends}
<!-- campaign: iter-005-draft — BS-0002; was explicit Non-Goal of REQ-09 -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- **Campaign:** iter-005 **draft**. Extends [REQ-09](#device-connectors); does not replace recognition or warp.
- **Outcome:** a connection can *say* how it ends (arrow, empty arrow, star, one, many, …) and stay that way when boxes move.
- **Path A — context toolbar** after the connector is created or selected: the creator sets **each end** independently from a closed style set. Needs design (e-ink chrome).
- **Path B — endpoint ink:** strokes whose samples run over a connector **end** are recognized as **part of that end**, not ordinary ink / not a new connector. They are **preserved** through warp (they ride the end). Wrong recog costs one undo.

**Acceptance**
- Given a selected connector, When the creator picks an end style from the context toolbar, Then that end shows the style with p95 ≤300 ms and the other end is unchanged; one undo reverts the style.
- Given a connector with end styles, When a bound box is dragged, Then styles remain on the correct ends and committed geometry equals last preview (0 px jump) — [REQ-09](#device-connectors) warp bar.
- Given Connector recognition (or a dedicated endpoint-ink rule) and ink drawn over an existing connector end, When the stroke ends, Then the ink is bound as endpoint decoration of that end (0 new free Ink at that location; 0 second connector) and survives a subsequent bound-node drag (0 orphaned samples).
- Given the same stroke over empty canvas or the connector **spine** (not an end), When the stroke ends, Then it is **not** stolen as endpoint style (ordinary ink / membership / connector rules apply).
- **UI states / journeys to design:** post-create toolbar; selected connector per-end styles; endpoint-ink accepted; endpoint-ink refused; warp with decorated ends.

## [REQ-14] Connector mid-attachments {#connector-attachments}
<!-- campaign: iter-005-draft — BS-0002 -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes
- **Campaign:** iter-005 **draft**.
- **Outcome:** a label or figure hung on a connection **stays on the line** as the line warps — like text taped to a string.
- Attachments are nodes (text, figures, ink clusters) bound to a parameter on the connector **rest spine**. Moving/resizing a bound box re-warps the connector **and** moves attachments with it. No rebake of rest shape ([REQ-09](#device-connectors) / ADR-0020).

**Acceptance**
- Given a connector and an attachment bound to it, When the creator moves a bound SmartGroup, Then the attachment stays on the spine (arc-length / `t` preserved) at ≥5 Hz partial refresh, and on pen-up its pose equals the last previewed pose (0 px jump).
- Given that attachment, When the creator undoes the box move, Then both connector and attachment return to the pre-move pose (±1 px @ 100% zoom).
- Given a connector with no attachments, When the box moves, Then [REQ-09](#device-connectors) still holds (0 regression).
- **UI states / journeys to design:** place/bind an attachment; selected attachment on a connector; drag of bound box with attachments; empty connector.

## [REQ-15] Table recognition {#table-recognition}
<!-- campaign: iter-005-draft — BS-0002 -->
- **Priority:** Could · **Traces:** [BRD-07]
- Needs design: yes
- **Campaign:** iter-005 **draft**. Not v1-core.
- **Outcome:** a drawn grid can start behaving as a **table** (cells as structure), not only as ink lines. Best-effort plus undo. Must not blow the [REQ-09](#device-connectors) ≤2% false-positive ship gate when armed.

**Acceptance**
- Given table recognition armed and a fixture of a clear drawn grid, When the stroke set completes, Then a table node exists in the local document with p95 ≤500 ms after the last pen-up and 0 desktop messages required; one undo restores ink.
- Given table recognition **disarmed**, When the same ink is drawn, Then 0 tables are created.
- Given both existing recognizers plus table recognition on a writing corpus (incl. a fresh page’s first 20 strokes), When replayed, Then unintended table **or** connector **or** enclose creations remain ≤2% of `pen` strokes.
- **UI states / journeys to design:** armed/disarmed; accepted grid; rejected (not a grid); undo.

## [REQ-16] Finger pan and zoom on the tablet {#device-pan-zoom}
<!-- lifecycle: retired -->
<!-- superseded-by: [REQ-10] -->
<!-- retired: 2026-08-16 — merged into [REQ-10](#hand-touch) (one-finger pick/move + two-finger pan/zoom). -->
- **Priority:** Won't (retired) · **Traces:** [BRD-07]
- Needs design: no
- **Retired.** Two-finger pan/zoom is specified under [REQ-10](#hand-touch). Do not implement this id.

**Acceptance**
- Given this id, When an auditor looks up pan/zoom, Then they follow [REQ-10](#hand-touch) (0 new stories tagged only REQ-16).

## [REQ-17] Manual creation of frames, connectors, attachments, and primitives {#manual-create}
<!-- campaign: iter-005-draft — BS-0002. Ink-box manual/enclose already REQ-05. -->
- **Priority:** Should · **Traces:** [BRD-07]
- Needs design: yes
- **Campaign:** iter-005 **draft**. Adjacent to [REQ-08](#node-manipulation) (manipulate vs **insert**).
- **Outcome:** when recognition misses, the creator can still **place** a frame (artboard), a connector, an attachment, or a **beautiful** primitive on purpose — not a raw ink stand-in. Ink-box creation remains [REQ-05](#device-ink-box) (done). Not a general brush/color/layer palette.

**Acceptance**
- Given a manual Frame create control, When the creator places a frame, Then a Frame node exists at the drawn/placed bounds (±1 px @ 100% zoom) with p95 ≤300 ms and one undo removes it.
- Given two bindable nodes, When the creator manually creates a connector between them, Then a connector exists with the same warp contract as [REQ-09](#device-connectors).
- Given a connector, When the creator manually attaches a node, Then [REQ-14](#connector-attachments) holds.
- Given a primitive create (ellipse/rect/line — closed set), When placed, Then the painted shape is the primitive geometry (not a polyline stand-in) and survives save/mirror ([REQ-07](#one-way-sync)).
- **UI states / journeys to design:** entry into manual create (chip vs context); frame place; connector place; primitive place; cancel; conflict with Pen ink.

## [REQ-18] Configurable pen barrel-button accelerators {#pen-buttons}
<!-- campaign: iter-005-draft — BS-0002 D9; editor home revised 2026-08-20 (human); Settings page + device persist 2026-08-20 (CHL-0025 / human lock) -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes *(Pen buttons is the first Device Settings detail; chip still mirrors temporary tool during hold-move)*
- **Campaign:** iter-005 **draft**. [BS-0002](../../../.plan/iter-004/brainstorms/BS-0002-iter-005-feature-wave.md) **D9** catalogues **replaced** 2026-08-20 (human). D9 lists are not current.
- **Outcome:** optional Wacom barrel buttons (0, 1, or 2) speed up erase / select / drag **without** making those the only path. Each present button has two slots — **Click** and **Hold-move** — each bound to **exactly one** item from a **closed catalogue**. The creator binds those slots **on the tablet** as the **Pen buttons** detail of Device Settings ([REQ-20](#device-settings)) — not Infini, not a document, not a sheet. **Never** three jobs on one hold-while-moving gesture. Persist of the map is [REQ-20](#device-settings) (on this device). Infini [REQ-05](../infini/prd.md#pen-button-map) persist/restore is **retired**.
- **Click** (button down+up, movement below threshold) — discrete toggle; closed catalogue **only**:
  - Current primary tool ↔ Freeform Select (`sel_freeform`)
  - Current primary tool ↔ Eraser (erase arm — **not** the nib; [REQ-11](#erase))
  - Off
- **Hold-move** (button down + movement past threshold until release) — temporary while the button is held **and** moving; closed catalogue **only**:
  - Temporary eraser
  - Drag node under tip (miss → no-op, **0** lasso)
  - Off
- **Why Hold-move is not temporary freeform (or rect).** Hold-move is a **temporary** overlay: on release the exclusive tool snaps back to whatever it was. Temporary freeform is meaningless if we do nothing after it and immediately switch back to the current tool — the selection gesture never becomes a lasting tool change. Same for temporary rect. Those items are **removed** from Hold-move (`temp_sel_freeform` and `temp_sel_rect` are not in the catalogue). Lasting select stays a **Click** toggle (current primary ↔ Freeform Select) or the ToolChip ([REQ-03](#tool-modes)).
- **Not in the catalogues:** Undo (stays on the chip); temporary freeform; temporary rect; a combined “empty→lasso / node→drag” item (later only as its **own** named id).
- **Defaults:** 1-button → Click = current primary tool ↔ Freeform Select; Hold-move = **Temporary eraser** (replaces the old 1-button Hold-move of temporary freeform). 2-button → B1 as 1-button; B2 Click = current primary tool ↔ Eraser; B2 Hold-move = Temporary eraser.
- Missing hardware: slots absent (0 misfires). Eraser **nib** is [REQ-11](#erase), not barrel 2. Accelerators only — ToolChip remains complete. Latch at button-down; rebind never changes a gesture in flight.
- **Chip during hold-move:** when Hold-move is Temporary eraser, the chip **mirrors** that temporary tool until release, then restores (unless the event was a Click toggle). Drag-under-tip does **not** switch the exclusive tool on the chip. The chip is **not** a 5-way radio and is **not** the map editor.

**Acceptance**
- Given a 1-button pen and default map, When the creator **clicks** the button (no move), Then the exclusive tool toggles current primary ↔ `sel_freeform` with p95 ≤300 ms and **0** hold-move gesture runs.
- Given the same pen and default map, When the creator **hold-moves** with the button down, Then **temporary erase** runs until release ([REQ-11](#erase) Path A feel) and **0** click toggle fires on release.
- Given a 2-button pen and defaults, When B2 is hold-moved, Then temporary erase runs until release and B1 is unchanged.
- Given the creator rebinds Hold-move to “drag node under tip”, When they hold-move starting on a hittable node, Then that node moves with the [REQ-06](#device-manipulation) live-direct bar; when they start on empty canvas, Then 0 nodes move and 0 lasso starts.
- Given a 0-button pen, When the creator draws, Then 0 button gestures fire and [REQ-03](#tool-modes) still works.
- Given a 20-gesture fixture mixing clicks and holds, When executed, Then 0 events fire **both** click and hold-move.
- Given Device Settings · Pen buttons and 0-button capability, When the page is shown, Then **0** barrel slots appear (0 fake bindings).
- Given Device Settings · Pen buttons and 1- or 2-button capability, When the detail pane is shown, Then Click offers only the three Click items above **inline** (0 Undo, 0 extra ids, 0 sheets).
- Given the same page, When the detail pane is shown, Then Hold-move offers only Temporary eraser, Drag node under tip, and Off **inline** (0 temporary freeform, 0 temporary rect, 0 sheets).
- **UI states / journeys to design (epaper-device, 1-bit, no hover):** Device Settings · Pen buttons — 0-button (slots absent), 1-button, 2-button, offline (still usable); Click and Hold-move catalogues **inline** on the same page (not sheets); entry via [REQ-20](#device-settings) leading tile (not Infini, not a 5-way radio on exclusive-tool tiles, not a fourth exclusive tool); chip during hold-move Temporary eraser (mirror then restore); chip during hold-move Drag-under-tip (exclusive tool unchanged); rebound map mid-session (next gesture only). Dual-ask: `/designer` Spec + scenes (painted [UI-EP-08](../../../.plan/iter-005/design/pen-button-map/ui-spec.md)); `/qa` BDD from this AC. Do **not** invent other Settings master items in this package.

## [REQ-20] Device Settings {#device-settings}
<!-- added: 2026-08-20 — human lock: Device Settings saved on Epaper; CHL-0025 Settings page; GAP-01 entry tile -->
- **Priority:** Must · **Traces:** [BRD-07]
- Needs design: yes *(Settings shell already painted as [UI-EP-08](../../../.plan/iter-005/design/pen-button-map/ui-spec.md); do not invent further Settings inventory)*
- **Campaign:** iter-005. First (only this package) settings item is **Pen buttons** ([REQ-18](#pen-buttons)).
- **Outcome:** preferences that belong to **this tablet** — not to a file, not to the desktop app — stay on this tablet. The creator opens **one Settings page**, picks a topic in a master list, and edits it in the detail pane. They can close and keep drawing; the next gesture uses what they just set. After Epaper restarts on **the same device**, those preferences are still there.
- **What this is not.** Device Settings are **not** document settings. There is **no** document-settings product this campaign. Infini is **not** the persistence home for Device Settings. The working **document** remains in-memory / Infini-persisted ([REQ-04](#device-document) / [REQ-07](#one-way-sync)).
- **Shell (CHL-0025 adopted).** Full-panel **Settings** page, **master-detail**. Master: first item **Pen buttons** (only item this package — do not invent other master rows). Click and Hold-move catalogues live **inline** in the Pen buttons detail pane (in-place pick). **0 sheets.** Capability 0 / 1 / 2 / offline are **states of that same page**, not separate products.
- **Entry (GAP-01 adopted).** A lone **10 mm** 1-bit tile (`cta.pen_map_open`) — stylus-with-barrels glyph — floating orientation-top **leading**, **sibling of ToolChip** (same chrome family as viewport-follow trailing and Undo). **Not** a fourth exclusive tool, **not** a 5-way radio on ToolChip tiles, **not** Infini File menu. Tap → `present-modal` Settings with Pen buttons selected. Close dismisses to drawing; live map kept.
- **Persist.** Saved **on the Epaper device**. Survives Epaper app restart on that device. **0** Infini app-settings copies. **0** SVG / VectorDocument fields. **0** `doc_*` messages for a settings write. Live map still applies with the desktop session down — persist does **not** wait on Infini.

**Acceptance**
- Given the leading 10 mm stylus-with-barrels tile on drawing chrome, When the creator taps it, Then Settings opens as a full-panel master-detail page with **Pen buttons** selected (p95 ≤300 ms), the exclusive tool is unchanged, and ToolChip is not a fourth exclusive for this action.
- Given Settings · Pen buttons, When shown, Then the master list contains **only** Pen buttons this package (0 invented sibling items) and Click / Hold-move catalogues are **inline** in the detail pane (0 sheets, 0 list-cancel hops).
- Given Settings · Pen buttons at 0-button / 1-button / 2-button / session-down, When shown, Then those are states of **the same page** (0 extra products) and the live map still applies while session-down.
- Given the creator rebinds a present Click or Hold-move slot, When they dismiss Settings, Then the **next** barrel gesture uses the new binding and any in-flight gesture is unchanged.
- Given a rebound map on this device, When Epaper restarts on **the same device**, Then the next barrel gesture uses that map with p95 ≤300 ms after the first HID report and Infini holds **0** copy of it (0 app-settings map, 0 SVG map).
- Given Infini is connected, When the creator rebinds on the tablet, Then Infini sends and receives **0** document messages for that write and does **not** persist or restore the map.
- Given a different Epaper device with factory defaults, When it connects to the same Infini document, Then it does **not** inherit the first device’s barrel map (settings are per device, not per file).
- **UI states / journeys to design:** drawing + `cta.pen_map_open` (rest / pressed / open); Settings · Pen buttons for layout 0 / 1 / 2 / offline; close back to drawing. Dual-ask: `/designer` Spec + scenes (already in [UI-EP-08](../../../.plan/iter-005/design/pen-button-map/ui-spec.md)); `/qa` BDD from this AC. Chip hold-move journeys stay [REQ-18](#pen-buttons) drawing scenes — not this page.

---

## Non-Goals

- **Always-on Infini→tablet viewport drive** — obsolete (human 2026-08-20). Viewports are
  independent by default; optional one-way follow is [REQ-19](#viewport-follow) /
  [infini REQ-06](../infini/prd.md#viewport-follow).
- **Hand-tool exclusive chip tile** for pan — follow is an **icon toggle**, not a ToolChip tool.
  No fourth exclusive tool for hand/pan.
- **Infini → Infini follow** — later / Non-Goal this campaign (do not specify as Must).
- **Two-way document sync** — changing the document channel is an explicit Non-Goal this campaign
  ([REQ-07](#one-way-sync) stays one-way tablet → desktop).
- **Finger rotation or connector re-anchor** — Won't this slice. Fine gizmos stay pen.
  **Finger resize knobs are in** ([REQ-10](#hand-touch),
  [CHL-0024](../../../.plan/iter-005/challenges/CHL-0024-finger-resize-knobs.md)).
- Acting as a Reawa-style mouse/stylus driver for other Mac apps.
- Cloud sync or multi-peer sessions.
- **On-device persistence of the working document, offline work across app restarts, and
  sync-at-any-moment** — the **document** is in memory for the session; Infini is the **document**
  persistence home ([REQ-07](#one-way-sync)). **Exception:** Device Settings persist on this
  device ([REQ-20](#device-settings)). Do not mint document settings.
- **Document settings** — none this campaign. Device Settings are device-level only.
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
- **Infini as Device Settings or pen-button map editor / persist home** — the creator does not
  configure barrel buttons on a desktop settings screen and Infini does **not** persist or restore
  the map. Home is [REQ-20](#device-settings) / [REQ-18](#pen-buttons). Infini [REQ-05](../infini/prd.md#pen-button-map)
  is **retired**.
- **Other Settings master items this package** — only **Pen buttons**. Do not invent Wi-Fi, display,
  accounts, or document prefs in this Settings page.
- **Sheet / `present-sheet` catalogues for Click or Hold-move** — dropped ([CHL-0025](../../../.plan/iter-005/challenges/CHL-0025-pen-map-settings-page.md)). Catalogues are inline.
- **Temporary freeform or temporary rect on Hold-move** — removed (human 2026-08-20). Hold-move
  snaps back on release; a temp select that does nothing afterward is meaningless.
- **Undo as a barrel Click catalogue item** — not in the closed Click list. Undo stays on the
  ToolChip ([REQ-03](#tool-modes) / [ADR-0018](../../adr/ADR-0018-undo-redo-chip-actions.md)).
- A general on-device tool palette — no brushes, colors, layers, or document browser.
  [REQ-17](#manual-create) is a **closed** insert set (frame, connector, attachment, primitive),
  not an illustration suite. ToolChip exclusive tools stay three unless a later ADR adds an entry.
- Production use of `regionsync/` `append_ink` NetSink until wired into the Qt binary
  (library remains the future ADR-0009 shape).

## Assumptions & Dependencies

- Infini [REQ-01]–[REQ-03] and [REQ-06](../infini/prd.md#viewport-follow) define the desktop side of
  the session; Infini is viewer + navigator + **document** persistence home, and its own ink-box
  authoring is deprecated ([infini REQ-04](../infini/prd.md#smart-group)). Viewport last-writer
  ([ADR-0023](../../adr/ADR-0023-viewport-last-writer.md)) is **not** the product model — Architect
  supersedes it with a follow / token-optional ADR. Device Settings persist on Epaper
  ([REQ-20](#device-settings)); Infini [REQ-05](../infini/prd.md#pen-button-map) persist/restore is
  **retired**. [ADR-0030](../../adr/ADR-0030-tablet-authors-pen-button-map.md) still says Infini
  persist/restore — Architect must supersede (PM does not write the ADR).
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

- **Iter-005 draft REQs minted 2026-08-16** from [BS-0002](../../../.plan/iter-004/brainstorms/BS-0002-iter-005-feature-wave.md): [REQ-11](#erase)–[REQ-18](#pen-buttons) ([REQ-16](#device-pan-zoom) **retired** into [REQ-10](#hand-touch)); [REQ-20](#device-settings) minted 2026-08-20. **AI** still unspecified — no REQ. — **owner:** pm — **needed by:** iter-005 open.
- Pen-button map editor on Infini desktop — **owner:** pm — **closed 2026-08-20 (human):** editor is Device Settings · Pen buttons on-device ([REQ-20](#device-settings) / [REQ-18](#pen-buttons)). Infini persist/restore **retired**. D9 catalogues replaced: Click = current↔Freeform Select / current↔Eraser / Off (no Undo); Hold-move = Temporary eraser / Drag node under tip / Off (no temp freeform/rect). 1-button Hold-move default = Temporary eraser.
- Device Settings persist home — **owner:** pm — **closed 2026-08-20 (human):** saved **on the Epaper device**, not Infini, not the document. No document settings. [CHL-0025](../../../.plan/iter-005/challenges/CHL-0025-pen-map-settings-page.md) Settings page adopted; GAP-01 leading 10 mm tile adopted.
- [REQ-10](#hand-touch) two-finger pan/zoom vs [BRD-07](../../brd.md) on-device pan/zoom deferral — **owner:** pm — **closed 2026-08-20 (human):** two-finger **local** pan is Must; always-on viewport sync is obsolete; optional mutually exclusive follow ([REQ-19](#viewport-follow)). Analyst amends BRD-07 in parallel; this PRD is the product source until BRD catches up. One-finger empty canvas is **local pan** (threshold vs palm-rest), not a no-op.
- Exact empty-canvas pan threshold (distance / time) — **owner:** pm — **closed 2026-08-20 (human lock for STORY-EP-054):** **10 mm** Euclidean panel travel. ≤10 mm = palm-rest / tap (0 pan); >10 mm = local one-finger pan; box/knob/chip hit wins. No new hand-touch UI inventory. Architect millimetre↔du bind stays in SRS (89 du @ 226 dpi).

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
  (CHL-0019). Hit target **< 64 du** → pen only, **except** resize knobs which **must** meet the
  floor so finger can resize ([CHL-0024](../../../.plan/iter-005/challenges/CHL-0024-finger-resize-knobs.md), 2026-08-20).
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
