# Failure modes — god-class extraction

Examples from the 2026-08 epaper tool-system work. Skill: [SKILL.md](SKILL.md).
Case: [usecase-epaper-tool-system.md](usecase-epaper-tool-system.md).

## 1. Hollow extract (the main failure)

**Symptom:** New `Operation` / `Mode` files exist. Git message says the phase
is done. `onDown` is:

```cpp
m_host.beginMoveFromPanel(s.panel, arm);
```

Real work is still in `ManipHost`, `SelectionManipController`, or
`ManipIntentApplier`.

**Why it happens:** The taxonomy plan lists `operations/move_operation.hpp`.
The agent creates the class and wires the hub. Body move is “later.”

**Avoid:** Destination table + “4-line lambda ⇒ not done.” Copy the applier
sequence into the Op in the **same** change that deletes the bag.

## 2. Relocating mess into smaller bags

**Symptom:** `FingerHost` / `ManipHost` / `SelectionStrokeHost` are walls of
`std::function` back into `ToolCanvasItem`. HostCaps grows, or a
`ToolHostBinder` is proposed.

**Human:** “moving mess from toolcanvasitem.cpp to smaller mess.”

**Avoid:** Bags are not capabilities. Unshared state is a **member of the Op**.
Shared document queries are functions on `DocContext` / `DeviceDocument`.
HostCaps does not grow.

## 3. Keeping the previous split’s coupling

**Symptom:** `ManipIntent` bitmask + `ManipIntentApplier` switch of 12 bits,
after Operations already run on the UI thread with `HostCaps*`.

**Origin:** Tablet/Tool split house rule: “return Intent; canvas applies.”

**Avoid:** That rule expires when the locked Op **is** the applier. Inline
`doc->applyLiveSmartGeometry` / `toolUi->redrawLiveManip` in order. Delete
`*Intent` / `*Applier`. Do not wrap them.

## 4. Taxonomy plan treated as extraction plan

**Symptom:** Phases 1–6 tick “PenMode exists,” “MoveOperation exists,”
“build-warn clean.” Human must write a second plan to move bodies, then a
third for leftovers.

**Avoid:** Do not implement Phase 1 of a taxonomy-only plan. Demand:

- destination table (method body → new home)
- delete list
- leftover-type policy
- body-level done-when

If the plan only has a folder layout and mermaid of concepts, it is not
comprehensive enough.

## 5. Parallel event systems

**Symptom:** `InputHub::dispatchPointerMove` returns false; host falls through
to ink / `selectionGestureActive` / `endTwoFingerTouch`.
`SelectionManipController` re-implements match (`resolvePress`).

**Avoid:** One router. Locked Op is the only receiver. Pinch goes through
`PinchSink`, not `dynamic_cast<NavigationOperation*>`. Host Q_INVOKABLE is
one hub call. No `if (m_finger.isTwoFinger())` on the canvas.

## 6. Wrong home for leftover “session” files

| File | Wrong | Right |
|---|---|---|
| `stroke_capture.hpp` | `tools/` or merge into `InkSink` | Tablet, next to ingest (`m_stroke`) |
| `finger_gesture_machine.hpp` | Keep “just in case” | Delete; Nav owns pan/pinch |
| `selection_session.hpp` | Keep after Lasso/Marquee own pts | Delete |
| `manip_session.hpp` | `drawing/` root, or merge into `TransformGesture` | `operations/transform_session.hpp` (Qt-free math) |
| `transform_gesture.hpp` | `tools/` root | `operations/` next to Move/Resize |
| Recog + HandTouch | `tools/` root | `modifiers/` under `ToolModifier` |
| `*context*` | `tools/` root | `contexts/` |

**Avoid:** After deleting a client, grep for the old type. Either delete the
file or `git mv` it next to the remaining client. Do not leave “seed” headers
at package root.

## 7. Merging layers that look similar

**StrokeCapture vs InkSink:** not “InkSink then StrokeCapture.” Pen →
`InkStrokeOperation` (lock) → `InkSink::ingestPen` → Tablet
`StrokeCapture::begin/append/end`. Capture pools **and** emits live segments.
Merge adds virtuals on the sample path and kills the Qt-free test.

**TransformSession vs TransformGesture:** session = world AABB math, no Qt.
Gesture = HostCaps + 200 ms ghost. One client, still two files — same folder.

**Avoid:** “1:1 client” is not a merge argument. Ask: would the merge put
virtuals on a hot path, or Qt on a host test?

## 8. Dead API left after the consumer died

**Symptom:** `ManipIntent`, `operator|`, `has()`, `apply(..., previewDue)`,
`abort()` returning unused result, `clearNodeId()` — TransformGesture only
reads `moved`/`resized` from `commit()`.

**Avoid:** After deleting appliers, grep the bits. Rewrite tests to assert
**geometry**, not intent flags. Drop parameters the session does not use
(preview gating belongs on the gesture).

## 9. Click commands forced into Operations

**Symptom:** Enclose / ink-scale / cut stay as host Q_INVOKABLE + Main.qml
rects because they are not pointer-locked gestures.

**Avoid:** `ToolAction` = tap → `HostCaps`. `Operation` = lock. Overlay knobs
are HitTarget, not actions. Context toolbar is `tools/ui`, instantiated from
`ToolCanvas.qml`, not Main.

## 10. Qt routing ignored

**Symptom:** Canvas handles ToolChip `setExclusive` because “the god class
owns tools.”

**Human:** Qt already routes chip taps. Canvas only needs exclusive id,
recog on/off, hand-touch on/off.

**Avoid:** Do not pull QML-owned chrome events into the canvas.

## 11. Extract without cleaning the old file

**Symptom:** User: “Are you cleaning up tabletcanvasitem.cpp when refactoring?”
Wrappers around `m_session`. `.cpp` section order ≠ header.

**Avoid:** Same change: move body, delete old method, drop dead members,
match header section order.

## 12. LOC shrink as success metric

`FingerGestureMachine` extract: user asked why `tabletcanvasitem.cpp` did not
shrink. The apply loop stayed. Split plan said success is **ownership + named
APIs**, not LOC.

**Avoid:** Measure: is there still a second router? Does the Op contain the
sequence? Can a new gesture land without editing `onPointerStart`?

## 13. Product/e-ink behavior not in the plan

After types were “done”:

- Lasso waited hundreds of ms (overlay + Pen waveform on first down)
- Live resize drove 8 QML knobs every ghost frame
- `opacity: 0.4` dithered disabled tiles
- Toolbar overlapped the bottom knob

**Avoid:** Smoke the extracted gesture on device, not only `build-warn`.
For e-ink: freeze settled chrome during live transform; keep overlay ready
in the mode that needs it.

## 14. Tests that fossilize deleted design

`manip_session_test` asserted `ManipIntent::BeginGesture` after nothing
read intents. Host suite still built `manip_session_test` after rename.

**Avoid:** Tests move with the type. Assert the remaining contract
(geometry, `moved`/`resized`). Update `run_device_document_test.sh`.

## 15. Editing the Cursor plan file

User: implement the attached plan; **do not edit the plan file**. Memory
belongs in `.docs/memory/`. Cursor `.plan.md` is the human’s checklist.

## 16. Propose merge; skip the latency/test argument

Agent offered to merge StrokeCapture into the sink / ManipSession into the
gesture. Human stopped both.

**Avoid:** Before merging, state: thread, virtuals on sample path, which
host test stays Qt-free, who paints (Tablet vs Tool overlay). If any of
those get worse, do not merge.
