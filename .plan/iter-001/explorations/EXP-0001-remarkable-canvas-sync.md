---
id: EXP-0001
goal: reMarkable as drawing tablet: draw on RM, view infinity canvas on macOS with synced drawing region
goal_source: chat
date: 2026-07-05
driven_by: pm
goal_from: human
initiative:
  - I1-rm-native-draw
  - I2-macos-infinity-viewer
  - I3-region-sync-protocol
status: paused-for-feedback
mode: try-hard
timebox: 2h per round
token_budget: 80k per round
sandbox: .sandbox/EXP-0001-remarkable-canvas-sync (branch exp/EXP-0001-remarkable-canvas-sync)
brainstorm: none
participants: [pm, architect, dev]
iter: iter-001
relates-to: []
---

# EXP-0001 — reMarkable as drawing tablet: draw on RM, view infinity canvas on macOS with synced drawing region

## Goal (from human)

> **macOS:** Display infinity canvas  
> **reMarkable:** Synchronized drawing region ↔ macOS  
> Drawing happens on reMarkable (best writing feel); viewing on macOS (best viewing feel). Makes reMarkable a true drawing tablet.

**Success criteria (measurable):**

| # | Criterion | Target |
|---|---|---|
| S1 | Pen ink appears on RM screen locally (not relayed via macOS) | p95 ≤ 30 ms pen-down → pixel |
| S2 | Completed stroke visible on macOS canvas | p95 ≤ 50 ms after RM sample |
| S3 | macOS pan/zoom updates RM drawing region | p95 ≤ 100 ms viewport → RM full refresh |
| S4 | Brush width visually consistent across zoom levels | No >10% perceived width drift at 0.5×–2× zoom |
| S5 | SDK build + deploy loop works on this dev machine (ARM64 Mac) | Cross-compile + scp + run on connected RM2 |

Loop runs until human stops or S1–S5 met on real hardware.

## Initiatives & approaches (PM)

### I1 — RM native drawing app (`I1-rm-native-draw`)

Qt Quick + C++ `tabletEvent` app; local ink; e-paper tile grid; stops xochitl during session.

### I2 — macOS infinity canvas viewer (`I2-macos-infinity-viewer`)

Swift/AppKit (or SwiftUI+Metal) app: visible frame = window; drawing frame fixed center; pan/zoom changes world sampling only.

### I3 — Region sync protocol (`I3-region-sync-protocol`)

TCP JSON-lines (spike) → protobuf later. Sync **stroke data** RM→macOS; sync **viewport + stroke batch** macOS→RM on pan/zoom. Not pen-event relay (existing Reawa module).

**Candidate approaches** (riskiest / highest-value first):

| ID | Approach | Tests |
|---|---|---|
| **A** | x86_64 Docker SDK toolchain on ARM64 Mac → cross-compile RM Qt app → deploy via scp | S5 |
| **B** | RM `TabletCanvasItem` + 8×6 e-paper tile grid (dragly pattern) + local draw | S1 |
| **C** | macOS viewport model + mock sync client (no RM) | S2/S3 logic |
| **D** | End-to-end: RM draw → macOS view + macOS pan → RM re-render with brush_scale | S1–S4 |
| **E** | Alt: relay pen events to macOS, draw on Mac, push bitmap back to RM | Higher latency; fallback only |

## Priority check — architect + dev push back

- **Vital to the goal?** Yes — without **A** (SDK on this machine) nothing ships to RM; without **B** writing feel fails; **C** can parallelize.
- **Riskiest assumption?** SDK x86_64-only on ARM64 Mac host — requires Docker `--platform linux/amd64`; also pen input not documented in official Qt Quick tutorial (“marker is more involved”).
- **Timeboxable?** Round 1 (2h): Docker amd64 smoke test + protocol sketch + spike scaffold. Evidence: `uname -m` = x86_64 in container; hello_remarkable or calculator builds.
- **Verdict**: **proceed** — start with **A**, parallel **C** while SDK blocked.

## Loop log

### Round 1 — approach A (SDK toolchain) + protocol spike

- **Experiment** (architect + dev, sandbox):
  - Created worktree `.sandbox/EXP-0001-remarkable-canvas-sync` on branch `exp/EXP-0001-remarkable-canvas-sync`.
  - Researched reMarkable SDK docs: x86_64 Linux only; Qt Quick epaper backend; RM2 requires `systemctl stop xochitl`; pen via C++ `tabletEvent` + `evdevtablet` (not QML MouseArea).
  - Scaffolded `spike/docker/` (Dockerfile, compose, install-sdk.sh), `spike/protocol/viewport-sync.md`, RM + macOS spike READMEs.
  - Validated x86_64 emulation: `docker run --platform linux/amd64 ubuntu:22.04 uname -m` → `x86_64`.
  - Built `spike/docker` compose image (Ubuntu 22.04 + cmake/ninja); container reports `x86_64` + cmake 3.22.
  - SDK installer not present yet — cross-compile blocked until user drops `meta-toolchain-remarkable-*-x86_64-toolchain.sh` into `spike/docker/sdk-installer/`.
- **Assessment** (PM, vs goal): **partial** — host toolchain path proven (S5 prerequisite); protocol + module split defined; RM deploy and pen spike still open.
- **Adversarial check** (architect lens): Even with SDK working, official docs omit pen handling — **B** must be C++-first, not pure QML calculator clone. E-paper full refresh on every pan/zoom may miss S3 (100 ms) without tile grid. Confirmed.
- **Feedback gate**: **paused** — need RM device model + OS version (for SDK download), SDK installer in `spike/docker/sdk-installer/`, and checkpoint vs try-hard mode confirmation.
- **Adjust** (PM): Round 2 = (1) amd64 container smoke + SDK install if user provides installer, (2) start **C** macOS viewport mock on host (no Docker), (3) RM CMake skeleton with TabletCanvas stub.

### Round 2 — approach A + B (SDK download, cross-compile, deploy attempt)

- **Experiment** (architect + dev, sandbox):
  - Device probe via Reawa SSH key: RM2 `IMG_VERSION=3.28.0.157`, Codex `5.8.197`, `armv7l`.
  - Downloaded SDK `remarkable-production-image-5.7.119-rm2-public-x86_64-toolchain.sh` (389 MB) from `storage.googleapis.com/remarkable-codex-toolchain/3.27.0.97/rm2/` — closest published match to 3.28.
  - Fixed Docker install (`file` pkg, read-only installer copy, persistent `rm-sdk-cache` volume).
  - Built Qt6 RM app `rm-canvas-spike` — ARM 32-bit ELF 2.5 MB with `TabletCanvasItem` (C++ `QEvent::Tablet*` handler).
  - Deploy blocked: `10.11.99.1` unreachable (USB route via WiFi gateway; ping TTL exceeded). User said connected but host routing not on USB iface.
- **Assessment** (PM, vs goal): **partial** — **S5 build half-met** (cross-compile ✅, scp/run ❌); **S1 untested** on hardware.
- **Adversarial check** (qa lens): SDK 3.27 vs device 3.28 skew — acceptable for spike but may fail at runtime; must verify on device. Zoom gesture in macOS spike is naive (noted).
- **Feedback gate**: **proceed** (try-hard) — continue **C** + prepare deploy script; retry scp when USB up.
- **Adjust** (PM): Round 3 = macOS viewport mock + deploy script; Round 4 = RM↔Mac TCP stub when device online.

### Round 3 — approach C (macOS infinity canvas mock)

- **Experiment** (architect + dev, sandbox):
  - `spike/macos-canvas/` Swift package builds and runs: pan shifts world origin, pinch zoom, **fixed center drawing frame** (dashed blue), viewport JSON emitted on each change matching `spike/protocol/viewport-sync.md`.
  - `spike/scripts/deploy-rm2.sh` — one-command scp + xochitl stop + epaper launch.
- **Assessment** (PM, vs goal): **partial** — **S2/S3 logic validated** on Mac without RM; drawing-frame-fixed-in-window behavior matches spec; no live sync yet.
- **Adversarial check** (architect lens): Drawing frame size hardcoded 702×526 (~RM2 aspect); should derive from RM2 constants (1404×1052 @2x or device pixels). Acceptable for spike.
- **Feedback gate**: **proceed** (try-hard) — next: TCP sync stub + redeploy RM when USB connected.
- **Adjust** (PM): Round 4 = add minimal TCP to macOS spike; retry RM deploy; pen test for S1.

### Round 4 — deploy RM app + macOS TCP sync stub

- **Experiment** (architect + dev, sandbox):
  - Fixed `deploy-rm2.sh` path; scp + launch succeeded when USB route restored.
  - RM2: `rm-canvas-spike` running (PID 4605), `xochitl` inactive. Qt font/thread warnings non-fatal.
  - macOS spike: TCP listener on `:9876` broadcasts viewport JSON on pan/zoom.
- **Assessment** (PM, vs goal): **partial** — **S5 met** (full build+deploy loop). **S1 pending** pen test on device.
- **Adversarial check** (dev lens): SDK 3.27 vs device 3.28 — app launched; tablet ink unverified until user draws.
- **Feedback gate**: **proceed** (try-hard) — Round 5: RM stroke TCP → Mac; e-paper tile grid.
- **Adjust** (PM): User validates pen on RM; build bidirectional stroke sync.

### Round 5 — fix pen input (TabletWindow + app event filter)

- **Experiment** (architect + dev, sandbox):
  - Root cause: Qt tablet events on RM2 go to `QQuickWindow` / `QGuiApplication`, not `QQuickPaintedItem`.
  - Added `TabletWindow` (`tabletEvent` override), `TabletAppFilter` on `QGuiApplication`, correct epaper env vars.
  - Redeployed; user confirmed **Strokes: 13, Events: 3839** — input received but **no visible ink** (coord bug, fixed Round 6).
- **Assessment** (PM, vs goal): **partial** — input path works; rendering broken until coord mapping.
- **Adversarial check** (qa lens): Events counter includes all move events — not a bug; stroke sync should batch/debounce for Mac TCP. E-paper tile grid still needed for S3 pan/zoom refresh perf.
- **Feedback gate**: **proceed** (try-hard) — Round 6: RM stroke TCP → Mac receiver (S2).
- **Adjust** (PM): Stream stroke points from RM; Mac app ingests live strokes; then viewport sync on pan/zoom (S3).

### Round 6 — visible ink + RM→Mac stroke sync

- **Experiment** (architect + dev, sandbox):
  - **Ink fix**: pen coords were digitizer space (0..20967), not screen pixels — strokes painted off-screen while counters incremented. Added `mapInputToCanvas()` + `QPixmap` backing store with immediate segment paint.
  - **S2 spike**: `StrokeSync` TCP (RM→Mac `:9877`); Mac `StrokeIngestServer` + `CanvasDocument.ingestRMLine()`.
  - Redeployed with `RM_SYNC_HOST=10.11.99.12`.
- **Assessment** (PM, vs goal): **partial** — deployed; awaiting user verify visible RM ink + Mac strokes.
- **Feedback gate**: **proceed** (try-hard).

### Round 7 — paused by human (2026-07-06)

- **Status**: User paused exploration. RM ink still not visible on e-paper (`QQuickPaintedItem` / QML Canvas); Mac receives strokes but tiny (coord mapping still off). xochitl restored.
- **Next when resumed**: RM tile-grid renderer with confirmed screen coords; Mac map using `cw/ch` from live RM geometry; verify TCP queue flush.

### Round 8 — candidate research refresh (new examples beyond dragly)

- **Experiment** (architect + dev, sandbox):
  - Researched newer/proven RM2 rendering paths:
    - `ddvk/remarkable2-framebuffer` (`rm2fb`) — RM2-specific framebuffer server/client; proven display path for apps that write `/dev/fb0`.
    - `rmkit-dev/rmkit` — supported RM2 app framework, including low-latency drawing apps; explicitly depends on `rm2fb` for RM2.
    - `CurtisFenner/remarkable-apps` — app engine using `rm2fb-client` + `/dev/fb0` and direct `/dev/input/event1`.
    - `canselcik/libremarkable` framebuffer notes — direct `/dev/fb0` + `MXCFB_SEND_UPDATE` ioctl proof of concept.
    - Qt6 `QQuickPaintedItem` docs — confirms `update(rect)` only schedules scenegraph repaint; does not prove RM2 e-paper partial update.
  - Built `spike/rm-fb-probe/` — a tiny ARM binary that writes a black test rectangle to `/dev/fb0` and attempts `MXCFB_SEND_UPDATE` (`0x4048462e`).
- **Assessment** (PM, vs goal): **partial** — the strongest candidates are framebuffer-level, not Qt/QML-level. The next decisive test is whether direct framebuffer update works on the user's RM2 OS (`3.28.0.157`) without adding `rm2fb`.
- **Adversarial check** (architect lens):
  - `rm2fb` is proven for RM2, but release-address support may lag modern Codex `5.8.197`; installing it is higher-cost.
  - Direct ioctl is lower-dependency and fast to falsify, but may fail on modern RM2 because SWTCON/rm2fb exists to solve exactly this framebuffer update problem.
  - Qt e-paper path remains lower-confidence for live ink because multiple Qt renderers received pen events but did not produce visible local ink.
- **Candidate ranking for next round**:
  1. **Direct framebuffer probe** — run `rm-fb-probe`; if a black rectangle appears, build RM ink on direct framebuffer + evdev/Qt input.
  2. **rm2fb path** — if direct ioctl fails, evaluate installing/building `rm2fb` or using Toltec package assumptions.
  3. **rmkit drawing demo** — use as reference once display backend is settled.
  4. **Qt dirty-rect tuning** — low-confidence fallback only.
- **Feedback gate**: **paused** — running the probe writes directly to the RM framebuffer; ask before deploying/running on hardware.

### Round 9 — framebuffer probe + QML rectangle candidate

- **Experiment** (architect + dev, sandbox):
  - Deployed and ran `rm-fb-probe` on RM2.
  - Probe output: framebuffer reports `260x1408`, `bpp=32`, `line_length=1040`, `smem_len=33554432`; `MXCFB_SEND_UPDATE` (`0x4048462e`) returned `EINVAL`.
  - This falsifies the older `1404x1872` RGB565 direct-ioctl assumptions for the current device state / Codex `5.8.197`.
  - Tried one lower-risk Qt candidate before `rm2fb`: emit pen segments into QML as rotated `Rectangle` items, because debug `Text` is visible on RM2 while `QQuickPaintedItem`/QML `Canvas` ink was not.
  - Built and deployed `rm-canvas-spike` with rectangle segment rendering; app is running as PID `5027`.
- **Assessment** (PM, vs goal): **pending user visual check** — if rectangles show, local RM ink can be built with Qt scenegraph primitives; if not, move to `rm2fb`/SWTCON path.
- **Observation** (human): black segmented strokes did **not** appear; debug counter still increments. Input is confirmed; dynamic Qt primitive ink is not visible.
- **Adversarial check** (dev lens): Direct framebuffer path needs modern RM2 display model research; `rm2fb` support for OS `3.28` appears not officially available, so it may require porting/version mapping rather than a simple install.
- **Feedback gate**: **waiting for human observation** — user should draw on RM2 and report whether black segmented strokes are visible.

### Round 10 — static Qt primitive discriminator

- **Experiment** (architect + dev, sandbox):
  - Added a static black QML `Rectangle` at top-right and thickened dynamic segment rectangles.
  - Rebuilt and deployed `rm-canvas-spike`; app is running as PID `5057`.
  - Purpose: determine whether any non-text Qt primitive can render on the current e-paper backend. Debug `Text` is visible, but `QQuickPaintedItem`, QML `Canvas`, and dynamic `Rectangle` segments have not produced visible ink so far.
- **Observation** (human): static black square did **not** appear; debug counter also stopped incrementing in this diagnostic build.
- **Assessment** (PM, vs goal): **blocked for Qt path** — Qt Quick scenegraph primitives are not a reliable RM ink path on this OS. Move to `rm2fb`/SWTCON or direct framebuffer porting.

### Round 11 — build and smoke-test `rM2-stuff` / `rm2fb`

- **Experiment** (architect + dev, sandbox):
  - Downloaded `timower/rM2-stuff` source into the sandbox (tarball, no git metadata).
  - Built `rm2fb_server`, `librm2fb_server.so`, and `librm2fb_client.so` with the official RM SDK after local build fixes:
    - Explicit hard-float ARM C/CXX flags for CMake compiler checks.
    - Added `xxd` to the SDK container.
    - Patched empty `SYSTEMD_INCLUDE_DIRS` quoting.
    - Pre-downloaded Frida Gum devkit.
  - Deployed `rm2fb` artifacts plus runtime libs (`libevdev.so.2`, `libsystemd.so.0`) to RM2.
  - Smoke test result:
    - `libqsgepaper.so` build ID: `4a b4 d5 3e f4 24 77 34 9c e3 a2 a2 9b 33 25 2f 2f 1a 96 6f` — unsupported.
    - xochitl preload path starts `STARTING RM2FB` but then fails with `Failed to get addresses`.
    - `xochitl` restored after test (`systemctl start xochitl`, PID `5183`).
- **Assessment** (PM, vs goal): **blocked on RM local rendering** — proven paths now point to a required firmware-specific `rm2fb`/SWTCON port for RM OS `3.28.0.157` / Codex `5.8.197`. Continuing by tweaking Qt/QML is low-value.
- **Adversarial check** (architect lens): This is not a drawing algorithm issue; pen input and Mac stroke sync are already demonstrated. The blocker is display ownership/refresh on modern RM2 firmware.
- **Feedback gate**: **paused** — next step is a higher-cost firmware-port task: map current xochitl/SWTCON functions or downgrade/use a firmware with supported `rm2fb`.

### Round 12 — isolate the Qt render path with control primitives

- **Rationale**: Before committing to a costly `rm2fb`/SWTCON firmware port, re-check the one fact we know: the debug `Text` renders and updates live on the panel. If a *filled* QML rect can also refresh, the earlier "no ink" results were coordinate/compositing bugs, not a backend wall — and QML-rect ink is a cheap win.
- **Experiment** (architect + dev, sandbox):
  - Rewrote `Main.qml` into a decisive test with three controls checked against the proven live `Text`:
    - **A** static black `Rectangle` (480×480 @ 120,300) — does the *initial* scene reach e-ink?
    - **B** black `Rectangle` (360×360 @ 120,900) toggled by an 800 ms `Timer` — do *dirty* filled rects refresh like text?
    - **C** pen ink as QML `Rectangle`s appended on `segmentDrawn`.
  - Removed the full-screen opaque `QQuickPaintedItem` from the render path (`visible: false`); it now only maps input + drives macOS sync, so it can no longer white-out the QML layer.
  - Rebuilt and deployed; app running as PID `5303`. Log is clean: epaper platform loads, no QML `objectCreationFailed`, only benign `Could not open bin file` / keymap notices.
- **Observation** (human): **A static square = visible; B square = blinks; C text = updates.** Ink: `segs` increments but no visible lines; **the blinker freezes while drawing** and all counters catch up only a few seconds after the pen lifts.
- **Assessment** (PM, vs goal): **major reframing** — the RM2 e-paper backend is NOT the wall. Filled QML rects both render at load (A) and refresh when dirty (B). The Round 10/11 "no ink → dead backend" conclusion was wrong. The true failure is **event-loop starvation**: each pen point ran `window()->update()` (a full-window e-ink repaint), painted into a now-invisible pixmap, and emitted a debug-text change — hundreds/sec on a Cortex-A7, saturating the e-ink refresh queue so timers/delegates only catch up after input stops.

### Round 13 — remove per-point repaint flood (event-loop fix)

- **Experiment** (architect + dev, sandbox):
  - Deleted all per-point `forceRefresh()` / `window()->update()` calls and the invisible-pixmap `drawSegment` work; QML now renders the ink layer purely from `ListModel` changes.
  - Throttled QML segment creation to a ≥3 px move (`dx²+dy² ≥ 9`) and aligned debug-text refresh to that same cadence, so an input burst can't flood the scene graph or the e-ink queue. Stroke tail is flushed on release.
  - Rebuilt (~6 s incremental) and redeployed; app running as PID `5348`.
- **Observation** (human): blinker now **keeps blinking while drawing** (event loop no longer starved), but still **no visible ink** although the segment counter increments.
- **Assessment** (PM): starvation fixed; remaining failure is that dynamically **inserted** Repeater delegates don't refresh, unlike property changes on existing nodes.

### Round 14 — ink via pre-created node pool

- **Experiment**: pre-instantiate a hidden `Repeater` pool (1500) and *reveal* nodes on draw (mirrors the working blinker toggle) instead of inserting new ones.
- **Observation** (human): ink counter increments, still no visible ink. But a **declared** hidden node toggled visible (probe **R**) *does* appear — so "hidden→shown" is not the discriminator.

### Round 15 — refresh-trigger truth table

- **Experiment**: four labelled probes — **S** static, **B** in-place `visible` toggle, **M** geometry change on a declared node, **R** declared hidden→shown.
- **Observation** (human): **S** shows, **B** blinks, **M** moves, **R** blinks — i.e. static render + in-place toggle + geometry change + reveal all refresh.

### Round 16 — Repeater `itemAt` vs coordinates

- **Experiment**: probe **P** moves a `Repeater` delegate via `itemAt(0)`; ink pool now visible-at-load and moved into place; debug prints last segment coords + item/window size.
- **Observation** (human): **both M and P move** (Repeater mutation refreshes fine), ink still absent, and **`last:` stays (0,0)** — the smoking gun.
- **Assessment** (PM): root cause found — the input `QQuickPaintedItem` was `visible:false`, so `width()/height()==0`; `mapInputToCanvas`'s digitizer fallback divided every pen coord down to ~(0,0). All ink since Round 12 was piling up invisibly at the origin.

### Round 17 — coordinate fix (first real ink)

- **Experiment**: input item made visible + transparent + non-painting (real geometry); `mapInputToCanvas` falls back to window size; ink pool recycles so it never stalls.
- **Observation** (human): **ink appears** and tracks input, but rotated (horizontal→vertical) and squashed (circle→ellipse); `last:` shows real coords.

### Round 18 — measure the transform (calibration)

- **Experiment**: log raw pen coords + window size; two clean edge strokes.
- **Data**: `win=1404×1872`; top-edge stroke → scene `x 29→1391` (long edge squashed into 1404), left-edge stroke → scene `y 0→1872`. **The device is used in landscape while the panel is portrait**, so the digitizer axes are swapped + unevenly scaled vs the panel.

### Round 19 — 90° rotation + aspect + flip (drawing works)

- **Experiment**: `mapInputToCanvas` rotates into an isotropic landscape frame — `renderX = penY·(w/h)`, `renderY = h − penX·(h/w)` (the `h −` flip corrects a left/right mirror found on first pass). Restored real line-segment ink; removed calibration logging. App running as PID `5716`.
- **Observation** (human): **"Ink tracks my pen correctly — lines, direction, aspect all right."** ✅ Drawing on RM2 achieved.

## Outcome

- **Result**: **drawing works on RM2** — pen ink renders locally on the e-paper with correct orientation and aspect, on stock firmware `3.28.0.157` (no `rm2fb`/root display hacks needed).
- **Root cause chain**: (1) the e-paper Qt scenegraph *does* render — it refreshes static nodes, in-place `visible` toggles, geometry moves, and `Repeater`/`itemAt` mutations; per-point `window()->update()` floods merely **starved** the event loop. (2) The input item was `visible:false` → zero-size → coords collapsed to (0,0). (3) The tablet is used in **landscape** vs a **portrait** panel → a 90° swap + aspect scale + one-axis flip fully corrects input→render.
- **Working recipe**: `QT_QPA_PLATFORM=epaper`, ink as a pre-created QML `Rectangle` pool moved into place (no per-point full-window repaint), input via a visible-but-transparent `QQuickPaintedItem`, coordinate transform `renderX=penY·(w/h)`, `renderY=h−penX·(h/w)`.
- **Still open**: verify macOS stroke sync uses the corrected coords; viewport/drawing-frame sync (S3); pressure-width and latency polish.

## Recommendation & routing

- **Decision** → ADR TBD (module split vs extend Reawa; stroke-sync vs event-stream)
- **Spec impact** → new module `.docs/modules/<name>/` via PM challenge after EXP achieves S1–S5 or downscope
- **Build path** → stories in iter-001 after greenlight

## Code disposition

- [x] Sandbox worktree created — `.sandbox/EXP-0001-remarkable-canvas-sync`
- [ ] Discard sandbox worktree (default) — after routing
- [ ] Promote to production via story(ies) — docs-first re-implementation
