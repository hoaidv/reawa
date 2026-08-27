---
name: god-class-extraction
description: >-
  Extract logic from a god class without leaving hollow types, host bags, or
  leftover session files. Use when refactoring TabletCanvas/ToolCanvas, splitting
  a god header, introducing Operation/Mode/Modifier types, dissolving Host bags,
  or any extract that might stop at forwarders. Read this before writing types.
---

# God-class extraction (Reawa)

A taxonomy plan (Router / Mode / Operation) is **not** an extraction plan.
Creating the types and wiring them is the cheap half. The expensive half is
moving **method bodies** into those types and deleting the old home.

Full case study: [usecase-epaper-tool-system.md](usecase-epaper-tool-system.md).
Traps with examples: [failure-modes.md](failure-modes.md).

## Before writing any new type

1. **Group members by complete cycle** (begin/update/end + the fields they share).
   Do not extract a name until the cycle is listed.
2. Write a **destination table**: every fat method on the god class → the new
   type’s method that will contain **that body**. Not “MoveOperation exists.”
3. Write a **delete list**: bags, appliers, second routers, leftover sessions.
4. Write a **leftover-type policy** for each old helper: delete / move next to
   its only client / keep on the other canvas (hot path).
5. Write a **done-when gate that inspects bodies**, not type existence.

## Done-when gate (inspect bodies)

A step is **not done** if any of these is still true:

- An Operation / Action `onDown`/`trigger` is a 4-line lambda into a Host bag,
  controller, or applier.
- The god class still has `if (pen) / if (selectionTool) / if (twoFinger)` besides
  the router.
- A `*Intent` bitmask is produced and a `*Applier` switches on it.
- A leftover session/machine file still exists after its only client died.
- A helper file sits at the package root while its only clients live in a
  subfolder (`operations/`, `modifiers/`, `contexts/`).
- Tests still assert deleted intent bits instead of the remaining geometry.

**Good template:** [`InkStrokeOperation`](../../../epaper/drawing/tools/operations/ink_stroke_operation.hpp)
(`HostCaps*`, private helpers, `onDown` does the work).

**Bad template (caught in review):** early `MoveOperation` calling
`m_host.beginMoveFromPanel(...)`.

Plan 2 said it explicitly: *“If an Op is still a 4-line lambda call, the step
is not done.”*

## What each plan layer must contain

| Layer | Answers | Not enough if it only… |
|---|---|---|
| Taxonomy (ADR / plan 1) | What concepts exist | Lists types + folder layout |
| Extraction (plan 2) | Where each **body** goes; what dies | Relocates methods into bags |
| Leftovers (plan 3) | Dead types, UI home, click vs gesture | Assumes “types exist ⇒ done” |

If plan 1 is “promising” but does not have a destination table + delete list +
body-level gate, **stop and write plan 2 before implementing phases 1–N**.
Do not “create the types first and move logic later.” That later never happens
without a human review.

## House rules from this codebase

- **Qt routes events.** Canvas does not hit-test ToolChip taps.
- **HostCaps is ports only.** No new lambda bags. No `ToolHostBinder`.
- **Intent+applier is split debt.** Tablet/Tool split used “session returns bits,
  host applies.” A locked Operation already has HostCaps — inline the sequence,
  delete the bitmask.
- **Hot path stays where latency lives.** `stroke_capture.hpp` stays on Tablet
  next to ingest; do not fold it into `InkSink` or `tools/`.
- **Math vs side effects can be two files**, but they live next to the Ops that
  own them (`operations/transform_session.hpp` + `transform_gesture.hpp`).
- **Click ≠ gesture.** `ToolAction` is tap → document. `Operation` is
  pointer-locked. Do not force enclose/cut into Operation.
- **LOC shrink is not success.** Success is ownership + named APIs + no parallel
  event system. Extracting `FingerGestureMachine` did not shrink the canvas.
- **Do not edit Cursor `.plan.md` files** unless the user says to.

## After each extract

- Clean the old file in the same change (user had to ask “are you cleaning up
  tabletcanvasitem when refactoring?”).
- Put the new file in its **home folder**, not `tools/` root.
- Drop unused APIs (`ManipIntent`, `clearNodeId`, `previewDue` on apply) when
  nothing reads them.
- Verify ARM `build-warn` + host suite + a smoke of the extracted gesture.
  Compile-only is how chrome/lasso regressions shipped.

## Load this skill when

- Splitting a god header/class
- Introducing Operation / Mode / Modifier / ToolAction types
- “Thin the host” / “move logic into tools/”
- The user says types look like forwarders
- Proposing to merge a Qt-free session into a port or a gesture
