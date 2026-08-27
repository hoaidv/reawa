---
title: Use case — epaper god-class → ADR-0033 tool system
status: memory
updated: 2026-08-27
source: git 2026-08-24…27 + chats e160e24c / 2de18ea7
plans:
  - ../../../.cursor/plans/tool_tablet_split_ed25a1b7.plan.md
  - ../plan_epaper-tool-system-refactor.md
  - ../plan_dissolve_host_bags.md
  - ../plan_toolaction_context_ui.md
---

# Use case: extract a canvas god class into a tool system

This is what actually happened. Three plans were required because the first
“payoff” plan named the architecture and the types, then implementation stopped
at **forwarders**. Humans reviewed, wrote a second plan to move **bodies**, then
a third to place leftovers (click-actions, overlay hits, dead sessions).

Load [SKILL.md](SKILL.md) before repeating this kind of extract.

## Starting point (2026-08-24)

[`tabletcanvasitem.h`](../../../epaper/drawing/tabletcanvasitem.h) was a god
header: document surface, live ink, pointer routing, selection/manip, chrome,
toolbar, sync — mixed members, no feature groups.

Human rule before any extract: **group by complete cycle**
(`beginHandleDrag`/`update`/`end` + shared fields). Investigate **only that
file** first. Do not invent types yet.

## Wave A — peel domain out of Tablet (still one canvas)

House rule at the time: controllers talk back with an **Intent** (bits + params),
canvas applies. That rule **expired** once Operations owned the UI thread.

| Commit / step | What landed | Hidden debt |
|---|---|---|
| `WorldPt` / `RawPt` / `PanelPt` | Coordinate vocabulary | — |
| `CanvasFrame` | Camera / panel↔world | — |
| `StrokeCapture` | Live ink pool on Tablet | Must **stay** on Tablet (latency) |
| `SelectionSession` + `ManipSession` | Qt-free selection/manip math | Intent bits for host appliers |
| `FingerGestureMachine` | Pan/pinch classifier | Did **not** shrink the canvas; user asked why |
| `ChipModel` / primary toolbar | Toolbar state | Qt already routes chip taps — canvas must not |

User correction: extracting a machine **without moving the apply loop** does
not shrink the god class. LOC is the wrong metric.

## Wave B — Tablet / Tool split (plan: `tool_tablet_split`)

Human plan, implemented 2026-08-25: [tool_tablet_split](../../../.cursor/plans/tool_tablet_split_ed25a1b7.plan.md).

**Payoff of this plan (and only this plan):**

- `CanvasSession` (`QObject` + NOTIFY) holds doc/frame/chip/follow.
- `TabletCanvas` = document surface (ink, rasterize, sync).
- `ToolCanvas` = overlay **+** `canvasInput` (pointer/finger/selection/manip).
- Named **Surface API** / **Interaction API**; no `friend`, no `paintToolChrome`
  callback into Tablet.
- QML: `ToolCanvas.qml` absorbs handlers; Main rebinds knobs/hand-touch.

**What this plan did not claim:** a Mode/Operation taxonomy. Intent+applier was
the coupling style **of this split**. Later ADR-0033 work had to **retire** it,
not wrap it.

Follow-ups the human had to demand:

- No wrappers around `m_session` — use the session.
- Reorder `.cpp` to match header sections; document methods without “quirky” prose.
- Later: Tablet must not call Tool (punch on session; QML wires session/surface).

## Wave C — ADR-0033 taxonomy (plan: `plan_epaper-tool-system-refactor`)

**Stated payoff:** new gesture = new Operation + descriptor + allow-list entry;
router unchanged. ToolCanvasItem is a router host, not a gesture body.

**Done-when in that file:** Mode objects, one Operation per gesture,
SelectionContext, HandTouch match→lock, recog modifiers, behavior preserved,
`build-warn` clean.

Phases 0–6 in git (`6dcab20` … `4683639`, 2026-08-26/27):

1. PenMode + `InkStrokeOperation` + HandTouch profile + recog modifier **types**
2. SelectionMode + SelectionContext + Lasso/Marquee **types** (Move/Resize still
   `ManipSession` on the host)
3. HandTouch registry; match/lock/feed
4. Move/Resize Operations + HitTargets
5. `SessionDocContext` / `ToolCanvasContext`; **intent appliers** as the sinks
6. Extract finger/selection/chrome from host into **Host bags** + controller

**Why this looked done and was not:** every phase could tick “type exists +
build-warn.” `InkStrokeOperation` was thick (the template). Move/Resize/Nav/Select
were **forwarders** into `ManipHost` / `FingerHost` / `SelectionStrokeHost` /
`SelectionManipController` / `*IntentApplier`.

Human review 2026-08-27:

> They are just forwarder after the refactoring. And real logic lives in
> ManipHost, FingerHost, FingerIntentApplier… that’s why you introduce bag of
> mess by moving mess from toolcanvasitem.cpp to smaller mess.
> Intent & appliers are old concept in very previous refactoring (splitting
> tabletcanvasitem). We intended to keep real logic within Operation.

The taxonomy plan never said “inline the applier sequence into `onMove`.”
It listed `operations/move_operation.hpp` in the layout and treated that as
the extract.

## Wave D — dissolve bags (plan: `plan_dissolve_host_bags`)

**Payoff:** Operations own state and call `DocContext` / `ToolContext` /
`SelectionContext` directly. HostCaps stays four ports. Delete Host bags,
appliers, `SelectionManipController`. Hub is the only event system.
Q_INVOKABLE = one hub call.

Gate written into the plan: *if an Op is still a 4-line lambda, the step is
not done.*

This is the plan the taxonomy file **needed and did not have**.

## Wave E — leftovers (plan: `plan_toolaction_context_ui`)

**Payoff:** click commands are `ToolAction` (not Operations); selection chrome
lives in `tools/ui`; overlay hits are hub `HitTarget`; Interventions are a
registered table; dead sessions die; `stroke_capture` stays on Tablet;
`manip_session` moves next to TransformGesture (later renamed
`transform_session` and moved into `operations/`).

Needed because dissolve still left:

- Enclose / ink-scale / cut-copy-paste on the host and in `Main.qml`
- Knob hit-test on ToolChrome instead of hub `overlayHitAt`
- Pen-near / second-contact as hardwired host methods
- `finger_gesture_machine` / `selection_session` files with no clients
- Unused `ManipIntent` bits after appliers died

## Wave F — this session (cleanup after the three plans)

Product bugs the extract missed (e-ink):

- Lasso delay: overlay + Pen waveform attached on first pen-down
- Slow resize: live transform drove 8 QML knobs every 200 ms ghost
- Disabled chrome dithered (`opacity: 0.4`); Cut/Copy/Paste had no icons
- Toolbar overlapped bottom knob

Ownership questions the taxonomy plan did not answer (human had to stop the
agent):

| Proposal | Verdict |
|---|---|
| Merge `StrokeCapture` into `InkSink` | **No.** Push port vs result machine; virtuals on hot path; kills Qt-free test |
| Merge `ManipSession` into `TransformGesture` | **No.** Qt-free AABB vs HostCaps + ghost clock. Keep two files, same home |
| Drop unused `ManipIntent` | **Yes.** Gesture already ignored the bits |
| Files at `tools/` root | **Move** to `modifiers/`, `contexts/`, `operations/` |

End layout (2026-08-27):

```text
epaper/drawing/tools/
  operation.hpp  mode.hpp  host_caps.hpp  strategy.hpp  input_hub.*
  interventions.hpp  ink_sink.hpp  tablet_ink_sink.hpp  viewport.hpp
  tool_chrome.*
  operations/     # gestures + transform_session/gesture (Move/Resize home)
  actions/        # ToolAction (click, not lock)
  modes/
  modifiers/      # ToolModifier + HandTouch + recog + profile
  contexts/       # Doc / SessionDoc / Selection / Tool / ToolCanvas
  ui/             # SelectionContextBar + toolbar QML
```

`toolcanvasitem.cpp` is Qt entry + hub register/forward (~266 lines).
`tabletcanvasitem.cpp` keeps ingest, rasterize, `StrokeCapture`.

## Why one “promising” plan was not enough

[`plan_epaper-tool-system-refactor.md`](../plan_epaper-tool-system-refactor.md)
is a good **spine**: do not fold everything into one `Tool` god-interface;
lock one Operation; HandTouch is a modifier; overlay ≠ document blit.

It is a weak **extraction plan**:

- No destination table (this host method → this Op method body)
- No delete list for the previous split’s intent/appliers
- “Done when types exist + build-warn” — agents optimize for that
- No leftover-type policy (`stroke_capture` vs `manip_session` vs machine)
- No click-vs-gesture (`ToolAction`)
- No “files live next to their only clients”

The split plan (`tool_tablet_split`) was comprehensive **for coupling two
canvases**. The taxonomy plan reused Intent+applier from that split instead of
killing it. The dissolve plan is the missing extraction chapter. The ToolAction
plan is the missing leftovers chapter.

**Process implication:** after a taxonomy ADR, do not implement Phase 1 until
an extraction plan exists with body-level gates. Expect the human to halt and
review if you only produce types.

## Transcripts (corrections live here)

- [e160e24c](../../../../.cursor/projects/Users-howard-Project-reawa/agent-transcripts/e160e24c-ed9f-478b-8335-a9101e928d43/e160e24c-ed9f-478b-8335-a9101e928d43.jsonl) — grouping, split, taxonomy, this cleanup
- [2de18ea7](../../../../.cursor/projects/Users-howard-Project-reawa/agent-transcripts/2de18ea7-9bd6-490c-b4ad-1fc915021530/2de18ea7-9bd6-490c-b4ad-1fc915021530.jsonl) — hollow Ops review, dissolve plan
