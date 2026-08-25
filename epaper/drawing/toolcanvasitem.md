# ToolCanvasItem — event routing

ToolCanvas owns pointer/finger interaction and selection chrome. TabletCanvas owns
document ink, rasterize, and the sync wire. They share one `CanvasSession`; Tool
calls Tablet only through the Surface API.

Input enters through `ToolCanvas.qml` handlers (and a few `Input` Connections in
`Main.qml`), not through Tablet.

```mermaid
flowchart TB
  subgraph qt [Qt / QML]
    DH["DragHandler / TapHandler / PinchHandler<br/>ToolCanvas.qml"]
    IN["Input filter<br/>penNear, contactCount"]
  end

  subgraph entry [Pointer entry — pen and finger]
    PS[onPointerStart / Move / End]
    PC[onPointerCancel]
    FT[onFingerTap]
    PIN[onPinchStart / Update / End]
    SC[onSecondContact / onContactsCleared]
  end

  subgraph branch [Branch by device]
    PEN["Surface ingestPen<br/>Tablet stroke / selection press"]
    F1[begin / update / end FingerTouch]
    F2[begin / update / end TwoFingerTouch]
  end

  subgraph machine [Gesture machines]
    FG[FingerGestureMachine]
    SS[SelectionSession]
    MS[ManipSession]
  end

  subgraph sinks [Intent sinks]
    AFI[applyFingerIntent]
    ASI[applySelectionIntent]
    AMI[applyManipIntent]
  end

  subgraph out [Outcomes]
    CHROME["Selection chrome<br/>refresh / damage / paint"]
    SURF["Tablet Surface API<br/>rasterize, punch, wire, ingest"]
    SESS["CanvasSession<br/>document / camera / chip"]
  end

  DH --> PS
  DH --> FT
  DH --> PIN
  IN --> SC
  IN --> PC

  PS -->|pen| PEN
  PS -->|finger| F1
  FT --> F1
  PIN --> F2
  SC --> AFI

  F1 --> FG
  F2 --> FG
  FG --> AFI

  AFI -->|select / move / resize| SS
  AFI -->|manip abort| MS
  AFI -->|camera / viewport| SESS
  AFI -->|rasterize / publish| SURF

  SS --> ASI
  MS --> AMI
  ASI --> CHROME
  AMI --> CHROME
  AMI --> SURF
  AMI --> SESS
  PEN --> SURF
  CHROME --> SURF
```

## Use cases

### Pen ink
User draws with the stylus. `onPointerStart/Move/End` see `pen=true` and call
`TabletCanvasItem::ingestPen`. Tool does not run the finger machine. Live ink and
`append_ink` stay on Tablet.

### Finger select / deselect
User taps a SmartGroup (or empty canvas) with hand-touch armed. `onFingerTap` or
finger drag runs `beginFingerTouch` → `endFingerTouch`. `FingerGestureMachine`
returns intents; `applyFingerIntent` may force `sel_freeform`, start a selection
gesture, clear selection, and refresh chrome. Knobs/enclose update via
`refreshSelectionChrome` → QML properties.

### Finger move / resize
User grabs a selected box or a knob. Press resolves to live manip
(`startLiveManip` / `beginHandleDrag`). Moves go
`updateFingerTouch` → `updateSelectionGesture` → `applyDragWorld` →
`applyManipIntent`. Tablet punches the origin hole (`notifyOriginPunch`); Tool
paints the live ghost; Infini gets `publishManipPreview`.

### Marquee / lasso
Empty press in a selection tool starts `beginMarqueeOrLasso`. Drag updates the
overlay path; release runs `finishMarqueeOrLasso` → selection set → chrome.

### Two-finger pan / pinch
`PinchHandler` drives `onPinch*`. One-finger manip is aborted; `FingerGestureMachine`
two-finger path updates the camera through `applyCameraRegion` on the session.
Tablet rasterizes because it listens to `cameraChanged`.

### Pen near / second finger
`Main.qml` Connections call `cancelHandTouch` or `onSecondContact` so navigation
outranks an in-flight one-finger manip without going through DragHandler.
