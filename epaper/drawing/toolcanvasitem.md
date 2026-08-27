# ToolCanvasItem — event routing

ToolCanvas owns pointer/finger interaction and selection chrome. TabletCanvas owns
document ink, rasterize, and the sync wire. They share one `CanvasSession`.

**Target architecture (ADR-0033):** `ToolCanvasItem` + `tools::InputHub` are the
Interaction Router. Modes / Operations / HandTouch live under
[`epaper/drawing/tools/`](tools/). Design notes:
[`.docs/memory/epaper-tool-system-refactor.md`](../../.docs/memory/epaper-tool-system-refactor.md).

Phase 0–5: contracts + InputHub; Pen/Ink; Selection/Marquee/Lasso; HandTouch dispatch;
Move/Resize via ManipHost + HitTarget knob regions; DocContext/ToolContext adapters +
intent appliers (`applySelectionIntent` / `applyManipIntent` delegate to `tools/`).

Input enters through `ToolCanvas.qml` handlers (and a few `Input` Connections in
`Main.qml`). **Tool decides** whether a stylus sample is selection/handle work or
ink; only ink is forwarded with `ingestPen`.

```mermaid
flowchart TB
  subgraph qt [Qt / QML]
    DH["DragHandler / TapHandler / PinchHandler<br/>ToolCanvas.qml"]
    IN["Input filter<br/>penNear, contactCount"]
  end

  subgraph entry [Pointer entry]
    PS[onPointerStart / Move / End]
    PC[onPointerCancel]
    FT[onFingerTap]
    PIN[onPinchStart / Update / End]
    SC[onSecondContact / onContactsCleared]
  end

  subgraph penBranch [Pen branch — decided on Tool]
    PH[tryBeginHandleAtPanel]
    PSEL[selectionToolArmed / selectionGestureActive]
    IP["Surface ingestPen<br/>ink only"]
  end

  subgraph fingerPath [Finger path — hand touch]
    F1[begin / update / end FingerTouch]
    F2[begin / update / end TwoFingerTouch]
    FG[FingerGestureMachine]
    AFI[applyFingerIntent]
  end

  subgraph shared [Shared selection / manip on Tool]
    BSG[beginSelectionGesture]
    USG[updateSelectionGesture]
    ESG[endSelectionGesture]
    SLM[startLiveManip / beginHandleDrag]
    ADW[applyDragWorld]
    BML[beginMarqueeOrLasso / finish…]
  end

  subgraph sinks [Intent sinks — tools/ appliers]
    ASI[SelectionIntentApplier]
    AMI[ManipIntentApplier]
  end

  subgraph ctx [Context ports]
    DOC[SessionDocContext]
    TUI[ToolCanvasContext]
  end

  subgraph out [Outcomes]
    CHROME[Selection chrome paint / Q_PROPERTY]
    SURF[Tablet Surface — rasterize punch wire]
    SESS[CanvasSession]
    INK[Tablet beginStroke / appendPoint]
  end

  DH --> PS
  DH --> FT
  DH --> PIN
  IN --> SC
  IN --> PC

  PS -->|pen press| PH
  PH -->|hit| SLM
  PH -->|miss| PSEL
  PSEL -->|sel tool| BSG
  PSEL -->|pen tool| IP
  PS -->|pen move + gesture| USG
  PS -->|pen move + ink| IP
  PS -->|pen end + gesture| ESG
  PS -->|pen end + ink| IP
  IP --> INK

  PS -->|finger| F1
  FT --> F1
  PIN --> F2
  SC --> AFI
  F1 --> FG
  F2 --> FG
  FG --> AFI
  AFI --> BSG
  AFI --> SLM
  AFI --> BML
  AFI --> SESS
  AFI --> SURF

  BSG --> SLM
  BSG --> BML
  USG --> ADW
  USG --> ASI
  ESG --> AMI
  ESG --> BML
  SLM --> AMI
  ADW --> AMI
  ASI --> CHROME
  ASI --> TUI
  AMI --> CHROME
  AMI --> DOC
  AMI --> TUI
  AMI --> SURF
  AMI --> SESS
  DOC --> SESS
  DOC --> SURF
  TUI --> CHROME
```

## Normal paths

### Pen ink (pen tool)
Stylus with exclusive tool `pen` (and no active selection gesture):

`onPointer*(pen)` → `ingestPen` → `ingestMappedTablet` → `applyContactPress` →
`beginStroke` / `appendPoint` / `endStroke`.

Tablet ingest is **ink-only**. Stash + origin guard stay on that path.

### Pen marquee / lasso / pick-move (selection tool)
Stylus with `sel_rect` / `sel_freeform`. Handled entirely on Tool — no Tablet hop:

| Phase | Call chain |
|---|---|
| Press | `onPointerStart(pen)` → `tryBeginHandleAtPanel` **or** `beginSelectionGesture` → `startLiveManip` / `beginMarqueeOrLasso` |
| Drag | `onPointerMove(pen)` → `updateSelectionGesture` → marquee/lasso update **or** `applyDragWorld` |
| Release | `onPointerEnd(pen)` → `endSelectionGesture` → `finishMarqueeOrLasso` / `commitLiveManip` |

### Finger select / deselect
Capacitive tap with hand-touch armed:

`onFingerTap` (or short drag) → `beginFingerTouch` / `endFingerTouch` →
`FingerGestureMachine` → `applyFingerIntent` → may `setExclusiveTool(sel_freeform)`,
`beginSelectionGesture`, `clearSelection`, `refreshSelectionChrome`.

### Finger move / resize / marquee
Armed finger drag:

`onPointerStart(finger)` → `beginFingerTouch` → machine intents →
`beginSelectionGesture` / handle resize / marquee.

While locked to live manip or marquee:

`onPointerMove(finger)` → `updateFingerTouch` → (if live manip)
`updateSelectionGesture` → `applyDragWorld`.

Release: `endFingerTouch` → settle pan or `endSelectionGesture`.

### Two-finger pan / pinch
`PinchHandler` → `onPinch*` → `begin/update/endTwoFingerTouch` → camera via
`applyCameraRegion`. Aborts one-finger manip first.

### Pen near / second finger
`Main.qml` → `cancelHandTouch` + `cancelInteraction` / `onSecondContact` so
navigation outranks an in-flight one-finger manip without DragHandler.

Live manip suppress: Tool writes `CanvasSession::liveManipSuppressIds` and paints
the live ghost via `DocumentRenderer::renderSubtree`; Tablet rasterize omits those ids.

## Shared vs separate

### Same selection pipeline, two entry doors
`beginSelectionGesture` / `updateSelectionGesture` / `endSelectionGesture` are
shared. **Who calls them**:

- **Finger:** pointer → finger machine → `applyFingerIntent` → selection API.
- **Pen:** pointer → handle/selection branch on Tool → selection API directly.

No Tool → Tablet → Tool round trip for selection.

### Pen never uses `updateFingerTouch`
Pen selection moves call `updateSelectionGesture` from `onPointerMove`. Finger
live-manip moves still go `updateFingerTouch` → (machine) → `updateSelectionGesture`.

### Handle hit on both devices
Press: pen and finger both can call `tryBeginHandleAtPanel` (different hit sizes:
`kHandleHitDu` vs `kFingerHandleHitDu`). After that, pen continues via
`onPointerMove(pen)`; finger via `updateFingerTouch`.

### Ink vs selection is decided on Tool press
`onPointerStart(pen)` order: handle → selection tool → else `ingestPen`. Once a
selection gesture is active, move/end stay on Tool even if the exclusive tool
changes mid-gesture until `endSelectionGesture`.
