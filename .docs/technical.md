# reMarkable RM2 → macOS Pen Driver — Technical Reference

This document describes the internal architecture, data flow, implementation details, and engineering history of the `reawa` package. For product behavior and user-facing setup, see [doc.md](doc.md).

---

## Table of Contents

1. [Goals](#goals)
2. [High-Level Architecture](#high-level-architecture)
3. [Project Layout](#project-layout)
4. [Data Model](#data-model)
5. [Pen Event Pipeline](#pen-event-pipeline)
6. [Planned Native Pen Device Mode (Swift)](#planned-native-pen-device-mode-swift)
7. [Driver Layer](#driver-layer)
8. [Absolute Mode and Window Snapping](#absolute-mode-and-window-snapping)
9. [Coordinate Systems](#coordinate-systems)
10. [UI Architecture](#ui-architecture)
11. [Services](#services)
12. [Threading Model](#threading-model)
13. [Storage](#storage)
14. [RM2 Device Constants](#rm2-device-constants)
15. [Dependencies](#dependencies)
16. [Entry Points](#entry-points)
17. [Key Design Decisions](#key-design-decisions)
18. [Bug Fix History](#bug-fix-history)
19. [Known Limitations](#known-limitations)

---

## Goals

- Stream pen events from a reMarkable 2 over SSH and translate them into macOS mouse events via Quartz `CGEvent`.
- Support multiple saved **connections** (devices) with per-connection SSH keys and device settings.
- Offer two output modes: **RELATIVE** (pen deltas) and **ABSOLUTE** (screen-mapped region snapped to a window).
- Run as a **menu bar–only** app (Dock icon only while the settings window is open).
- Auto-detect the device when plugged in via USB Ethernet.

---

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│  RemarkableApp (rumps menubar)                                          │
│  ├── ConnectionManager        — connect/disconnect, status, live config │
│  ├── USBWatcher               — reachability + auto-connect               │
│  ├── SnapPicker               — window selection overlay                │
│  ├── RegionOverlayController  — region border + resize handles          │
│  ├── WindowSnapController     — AX window move/resize + follow          │
│  └── ConnectionsWindow        — AppKit settings UI                      │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  DriverSession (background thread)                                      │
│  SSH pen stream → RelativeDriver | AbsoluteDriver → Quartz mouse      │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  driver/rm2.py — SSH, key setup, /dev/input/event1 parsing              │
└─────────────────────────────────────────────────────────────────────────┘
```

### App-Level State Machine (Absolute Mode)

```
                    ┌─────────────────┐
                    │   RELATIVE      │
                    │ (pen → cursor)  │
                    └────────┬────────┘
                             │ user selects ABSOLUTE
                             ▼
                    ┌─────────────────┐
         Esc ──────►│   PICKING       │◄──── Choose window / Absolute
                    │ input paused    │
                    │ SnapPicker open │
                    └────────┬────────┘
                             │ click window
                             ▼
                    ┌─────────────────┐
 Relative toggle ─►│   SNAPPED       │
                    │ overlay visible │
                    │ AbsoluteDriver  │
                    └─────────────────┘
```

App-level flags:


| Flag                    | Meaning                                                             |
| ----------------------- | ------------------------------------------------------------------- |
| `_picking`              | Snap picker is active; pen input paused                             |
| `_snapped_conn_id`      | Connection ID that completed a successful snap this session         |
| `_snapped_window_state` | `"normal"` or `"minimized"` — tracks overlay visibility transitions |


`_update_overlay()` is the central orchestrator: it shows/hides the region overlay, starts the picker when mode is ABSOLUTE but `_snapped_conn_id` is unset, and starts the window-follow timer when snapped.

---

## Project Layout


| Path                             | Responsibility                                                   |
| -------------------------------- | ---------------------------------------------------------------- |
| `app.py`                         | Menu bar entry, snap-flow orchestration, overlay lifecycle       |
| `driver/rm2.py`                  | SSH connection, RSA key install, pen event binary parsing        |
| `models/connection.py`           | `Connection`, `DeviceConfig`, `AbsoluteConfig`, status enum      |
| `models/store.py`                | JSON persistence, per-connection SSH key paths, legacy migration |
| `services/connection_manager.py` | Active session, connect/disconnect, live config cache            |
| `services/usb_watcher.py`        | Background reachability polling + auto-connect                   |
| `services/network_discovery.py`  | `ifconfig` parsing, SSH subnet probing                           |
| `services/keychain.py`           | macOS Keychain password storage                                  |
| `services/notifications.py`      | `osascript` display notifications                                |
| `driver/session.py`              | Background SSH read loop, live driver swap                       |
| `driver/relative.py`             | Delta-based pen → cursor translation                             |
| `driver/absolute.py`             | Region-mapped pen → cursor translation                           |
| `driver/mouse.py`                | Quartz `CGEvent` posting, display bounds, scale                  |
| `driver/window_snap.py`          | `CGWindowList` enumeration, AX move/resize, lifecycle            |
| `ui/connections_window.py`       | AppKit settings window                                           |
| `ui/snap_picker.py`              | Full-desktop window picker overlay                               |
| `ui/region_overlay.py`           | Click-through region border + corner handles                     |
| `ui/display_bounds.py`           | Quartz ↔ Cocoa coordinate conversion                             |
| `ui/dock_policy.py`              | `NSApplicationActivationPolicy` management                       |
| `assets/menu_icon.png`           | Menu bar template glyph                                          |


---

## Data Model

### Connection

```python
@dataclass
class Connection:
    id: str           # UUID
    name: str
    ip: str
    auto_connect: bool
    device_config: DeviceConfig
```

Persisted in `~/Library/Application Support/remarkable-rm2/connections.json`.

### DeviceConfig


| Field                             | Type                        | Notes                                                     |
| --------------------------------- | --------------------------- | --------------------------------------------------------- |
| `output_mode`                     | `"RELATIVE"` | `"ABSOLUTE"` | Drives driver selection                                   |
| `scale`                           | `float | None`              | Points per digitizer unit; `None` = auto from display PPI |
| `swap_xy`, `invert_x`, `invert_y` | `bool`                      | Axis transforms before mapping                            |
| `absolute`                        | `AbsoluteConfig`            | Region geometry + snap metadata                           |


### AbsoluteConfig

Region stored in **Quartz global coordinates** (see [Coordinate Systems](#coordinate-systems)):


| Field                                                   | Purpose                                                      |
| ------------------------------------------------------- | ------------------------------------------------------------ |
| `region_x`, `region_y`, `region_width`, `region_height` | Snapped mapping rect (RM2 aspect locked via `lock_aspect()`) |
| `border_color`, `border_style`                          | Overlay border (`solid` | `dashed`)                          |
| `snap_window_enabled`                                   | Whether a window is bound                                    |
| `snapped_window_ref`                                    | Window title (or `pid N`) for display                        |


RM2 aspect ratio: `PEN_X_MAX / PEN_Y_MAX` = `20967 / 15725`.

### ConnectionStatus

Derived state (not persisted):


| Status      | Condition                                   |
| ----------- | ------------------------------------------- |
| `offline`   | Device IP not reachable                     |
| `online`    | IP reachable, no active pen stream          |
| `connected` | Active `DriverSession` with open SSH stream |
| `error`     | Last connect attempt failed                 |


---

## Pen Event Pipeline

```
RM2 digitizer (/dev/input/event1)
  → SSH stream (paramiko, dd bs=24)
  → rm2.read_pen_frames() → PenFrame
  → DriverSession loop
       → [paused? skip]
       → read live DeviceConfig via config_getter
       → swap driver if output_mode changed
       → RelativeDriver | AbsoluteDriver.handle_frame()
  → MouseController.post_mouse_event() (Quartz CGEvent)
```

### PenFrame

```python
@dataclass(frozen=True)
class PenFrame:
    tv_sec: int
    tv_usec: int
    x: int              # digitizer X (0..PEN_X_MAX)
    y: int              # digitizer Y (0..PEN_Y_MAX)
    pressure: int | None
    touching: bool      # BTN_TOUCH
    in_proximity: bool  # BTN_TOOL_PEN
```

### SSH and Key Setup (`driver/rm2.py`)

- Connect with RSA key; fall back to password + `setup_key()` on auth failure.
- `setup_key()`: generate 3072-bit RSA key, install public key in `authorized_keys` on device.
- Pen stream: `dd bs=24 if=/dev/input/event1` over SSH.
- Event format: `struct` format `"2IHHi"` (timestamp, type, code, value).
- Frames assembled on `EV_SYN` + `SYN_REPORT` when both X and Y are known.

---

## Planned Native Pen Device Mode (Swift)

This section describes the **planned Swift-only pen-device backend**. The current shipping path still ends in Quartz mouse events.

### Goal

Add a second output backend that publishes reMarkable pen input to macOS as a **generic HID digitizer / stylus device**, so drawing apps can receive pen data directly instead of seeing only mouse movement and clicks.

### Core design rules

1. Reuse the existing Swift SSH/event parser pipeline as the input source of truth.
2. Do **not** emulate a specific Wacom driver identity; expose a generic macOS tablet/stylus device.
3. Keep the current Quartz mouse path as a fallback backend.
4. Avoid classic kernel extensions; the target implementation is a modern Swift virtual-HID / DriverKit-compatible path.

### Planned high-level flow

```mermaid
flowchart TD
    RM2[RM2Digitizer] --> SSH[SSHSessionAndPenFrameParser]
    SSH --> Normalize[SwiftPenStateNormalizer]
    Normalize --> MouseBackend[QuartzMouseBackend]
    Normalize --> TabletBackend[VirtualTabletDeviceBackend]
    TabletBackend --> MacOS[macOSHIDTabletEvents]
    MacOS --> Apps[CreativeApps]
```

### Intended backend split

- **Current backend:** `InputDrivers.swift` posts Quartz `CGEvent` mouse events.
- **Planned backend:** a new Swift tablet-device backend creates a virtual digitizer device and submits stylus reports into macOS.
- **Selection model:** backend choice should be explicit and runtime-switchable, so the app can fall back to mouse emulation on machines where tablet-device mode is unavailable.

### Data the tablet backend must carry

The current Swift `PenFrame` now carries the RM2 data needed for richer diagnostics and a future tablet-class backend:

- `x`, `y`
- `pressure`
- `touching`
- `inProximity`
- `stylusButton` (`BTN_STYLUS`)
- `distance` (`ABS_DISTANCE`)
- `tiltX`, `tiltY`
- `rawEvents` captured until each `SYN_REPORT`

For the planned tablet backend, the implementation should preserve and forward this metadata rather than collapsing the stream down to mouse-only semantics.

That means the planned implementation should build on:

- `Sources/ReawaApp/Models.swift` — already-expanded `PenFrame`, `PenRawEvent`, and `PenStateSnapshot`
- `Sources/ReawaApp/SSHSession.swift` — current RM2 event parsing and per-frame raw-event retention
- `Sources/ReawaApp/InputDrivers.swift` or a sibling backend file — normalize `PenFrame` into either Quartz mouse output or virtual tablet reports

### Preferred implementation path

Primary target:

- Implement the feature in **Swift** as a user-space virtual HID device backend when the necessary Apple entitlement is available.

Current repository state:

- The Swift app now contains a **Native Stylus** backend spike using `CoreHID.HIDVirtualDevice`.
- The code path is useful for integration work, HID report design, fallback behavior, and packaging preparation.
- However, **actual virtual HID device creation is still blocked until Apple approves** `com.apple.developer.hid.virtual.device` for the signing team and the app is launched as a signed `.app` bundle with that entitlement.
- `swift run reawa` cannot exercise this feature because the SwiftPM-built executable is not a provisioned app bundle with the restricted entitlement.

Fallback / advanced path:

- If the user-space virtual-HID path is insufficient, move the tablet backend into a **HIDDriverKit / DriverKit system extension** while keeping the existing Swift app as the controller and SSH event source.

Explicitly avoided:

- classic kernel extensions
- Quartz mouse emulation as the only output path for drawing-app compatibility
- spoofing Wacom branding instead of publishing a generic digitizer

### Expected integration points in the Swift codebase

| Swift area | Planned change |
| ---------- | -------------- |
| `Sources/ReawaApp/Models.swift` | Reuse the richer `PenFrame` / `PenRawEvent` model for a tablet backend |
| `Sources/ReawaApp/SSHSession.swift` | Reuse the current RM2 parser, raw-event retention, and richer pen-frame emission |
| `Sources/ReawaApp/InputDrivers.swift` | Split Quartz mouse output from tablet-device output |
| `Sources/ReawaApp/ConnectionManager.swift` | Carry backend selection into the live session |
| `Sources/ReawaApp/SettingsUI.swift` | Expose backend choice and capability / availability state |
| `Sources/ReawaApp/Logging.swift` | Surface tablet-backend startup and entitlement errors |

### Verification targets

Recommended validation order:

1. **Krita** — use Tablet Tester first to verify that macOS and the app see a pen device instead of a mouse.
2. A browser pointer-events test page — quick sanity check for pen/pressure/tilt reporting where supported.
3. **Photoshop** or another production drawing app — end-to-end validation after Krita works.

### Major delivery risk

The biggest non-code risk is Apple entitlement approval for virtual HID / DriverKit capabilities. The product should therefore treat native pen-device output as an optional backend and preserve mouse emulation as a supported fallback.

Practical implication:

- Without Apple approval, local development can prepare the code, app bundle, signing flow, and entitlement files, but it **cannot** make macOS accept the process as a real Virtual HID publisher in the supported path.

---

## Driver Layer

### DriverSession

Runs on a **daemon background thread**. One session per active connection.

Key behaviors:

1. **Live config** — Each pen frame reads `DeviceConfig` from `ConnectionManager.active_device_config()` (in-memory cache, no per-frame disk I/O).
2. **Live driver swap** — When `output_mode` changes, calls `cleanup()` on the old driver and instantiates the new one **without** tearing down SSH.
3. **Pause / resume** — `pause()` sets a flag so frames are discarded (used during window picking). SSH stays open. `resume()` clears the flag.

```python
# Simplified session loop
for frame in rm2.read_pen_frames(stdout):
    if paused: continue
    cfg = config_getter()
    if cfg.output_mode != current_mode:
        driver.cleanup()
        driver = make_driver(cfg.output_mode)
    if isinstance(driver, AbsoluteDriver):
        driver.update_region(cfg.absolute)
    driver.handle_frame(frame)
```

### RelativeDriver

- Computes pen deltas from successive `PenFrame` values.
- Applies scale (`effective_scale`: auto PPI / RM2 DPI ≈ 2531), swap, invert.
- Clamps cursor to union of all display bounds.
- Hover → `kCGEventMouseMoved`; touch down/up/drag → left button events.
- Releases button on proximity loss.
- **Swift port note:** `RelativePenDriver` synthesizes a gesture lifecycle from `PenFrame.inProximity` / `touching`: hover-start → hover-move → hover-end, and touch-start → touch-drag → touch-end. Each gesture captures both the pen anchor `(x, y)` and the live cursor position at the start of the gesture (or phase transition), then computes cursor motion relative to that anchor. This preserves the scale fix without depending on a continuously refreshed live cursor baseline.
- If the live cursor diverges from the gesture's expected cursor (for example from a trackpad or mouse move), the current relative gesture is **rebased** to the live cursor using the latest pen point as the new anchor. That prevents the next pen move from teleporting back to an older synthetic cursor position.

### AbsoluteDriver

- Maps pen `(x, y)` linearly into the configured region rectangle.
- Clamps synthesized cursor position to the region via `clamp_to_rect`.
- Region updates every frame from live config (supports resize/follow without reconnect).

### MouseController

Thin wrapper over Quartz `CGEventCreateMouseEvent` / `CGEventPost`. Also provides:

- `desktop_bounds()` — union of active displays (Quartz)
- `display_at_point()` / `effective_scale()` — PPI-based auto scaling for RELATIVE mode
- `map_pen_coords()` / `map_delta()` — axis transforms

---

## Absolute Mode and Window Snapping

### Snap Picker (`ui/snap_picker.py`)

**One borderless overlay window per display** at `NSScreenSaverWindowLevel`. A single window spanning all displays is only event-interactive on the display holding the majority of its area (with "Displays have separate Spaces" enabled), so clicks on other displays never reach the view. Cursor polling is global, which masked this bug because highlighting worked everywhere.

Flow:

1. Polls `CGEventGetLocation` every 50 ms (global cursor).
2. Hit-tests against on-screen windows via `window_under_point()` (Quartz coords).
3. Broadcasts the hovered window's highlight to every per-screen view (each converts the global Cocoa rect into view-local coords); hint text drawn only on the primary display's view.
4. **Click** on whichever screen's view is under the cursor → `on_pick(WindowInfo)`; **Esc** → `on_cancel()` → revert to RELATIVE.

`_PickerWindow` overrides `constrainFrameRect:toScreen:` to return the frame unchanged (otherwise AppKit shrinks borderless windows away from the primary display's menu bar).

### Window Enumeration (`driver/window_snap.py`)

Uses `CGWindowListCopyWindowInfo` (not per-process `NSRunningApplication` scans):

- Filters: `kCGWindowLayer == 0` (normal windows), excludes own PID, min size 40×40.
- Returns front-to-back list with bounds in **Quartz top-left** global coordinates.
- Resolves chosen window to an Accessibility element by PID + frame proximity for move/resize.

`WindowSnapController` methods:


| Method                      | Purpose                                                                                       |
| --------------------------- | --------------------------------------------------------------------------------------------- |
| `pick_from_info()`          | Resolve CGWindowList entry to AX element, bind element, store title ref + window number + pid |
| `restore_window()`          | Un-minimize / un-stage and focus the picked window; poll until frame stabilizes (~0.5 s)      |
| `snap_region_to_window()`   | Align region to window top-left, fit RM2 aspect **inside** window bounds                      |
| `sync_window_to_region()`   | AX set position/size so snapped window matches region                                         |
| `sync_region_to_window()`   | Align region to current window bounds (aspect-fit inside)                                     |
| `current_window_frame()`    | Read AX position/size for follow timer                                                        |
| `snapped_lifecycle_state()` | `"closed"` | `"minimized"` | `"maximized"` | `"normal"`                                       |


`sync_window_to_region()` wraps position/size in `AXValueRef` via `AXValueCreate` (raw `CGPoint`/`CGSize` is silently ignored). Sets size → position → size so a window being shrunk isn't clamped by its current larger frame while moving.

`window_frame()` unwraps `AXValueRef` handles via `AXValueGetValue(kAXValueCGPointType | kAXValueCGSizeType)` before reading coordinates.

### Region Overlay (`ui/region_overlay.py`)

Two layered UI pieces:

1. **Overlay window** — spans all displays (Cocoa-framed). Click-through (`setIgnoresMouseEvents_(True)`). Draws **border only** — no dim fill.
2. **Corner handle windows** — 18×18 pt at each corner; receive mouse drags; diagonal resize cursor; aspect-locked resize syncs snapped window via `_on_region_changed` → `sync_window_to_region`.

`RegionOverlayController` is a plain Python class (not `NSObject`) to avoid PyObjC selector collisions (`show`/`hide` vs Objective-C zero-arg methods).

### Window Follow and Lifecycle

`rumps.Timer` (0.4 s) polls the snapped AX window:


| State         | Detection                                                                                                                                              | Action                                                                                                   |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------- |
| **normal**    | Default; window on screen with ordinary size                                                                                                           | Overlay visible. Region follows window moves; external resizes re-fit region and `sync_window_to_region` |
| **minimized** | `kAXMinimizedAttribute` **OR** absent from *OnScreenOnly* list (Cmd+H, off-stage/Space) **OR** Stage Manager strip (CG on-screen area ≪ AX frame area) | Hide overlay. Stay in ABSOLUTE                                                                           |
| **restored**  | Was off-screen, now back in *OnScreenOnly* list                                                                                                        | `sync_region_to_window()`, show overlay                                                                  |
| **maximized** | `AXFullScreen` attribute **or** frame ≈ screen `visibleFrame` (6 pt tolerance)                                                                         | `snap_region_to_window()` for largest aspect-fit rect, then `sync_window_to_region()`                    |
| **closed**    | AX element reports `kAXErrorInvalidUIElement`                                                                                                          | `_revert_to_relative()`                                                                                  |


**Close detection via AX validity, not the window list.** Stage Manager removes minimized windows from every `CGWindowList` query even though they still exist. Close is detected from AX element validity — `_element_alive()` reads `kAXRoleAttribute` and treats only `kAXErrorInvalidUIElement` as closed. Transient `kAXErrorCannotComplete` is treated as alive.

**Stage Manager minimize detection.** `_is_stage_manager_minimized()` compares on-screen CG area to AX frame area; ratio below `_STAGE_AREA_RATIO` (0.5) means staged.

**Restore-on-pick.** When a Stage Manager thumbnail is picked, `restore_window()` clears `kAXMinimizedAttribute`, performs `kAXRaiseAction`, activates the owning app, and polls until the frame stabilizes.

### Picker Lifecycle (`_cancel_pick`)

Exiting the snap picker through switching to **Relative** (settings toggle or menu bar) or **Choose window** (not only Esc or a successful click) must call `_cancel_pick()` to stop the overlay and clear `_picking`. Otherwise the flag stays `True`, `_start_pick()` becomes a no-op, input stays paused, and the user cannot re-enter the picker.

---

## Coordinate Systems

macOS uses two global coordinate spaces. Mixing them breaks picking, pen output, and overlay drawing.


| Space             | Origin                               | Used by                                                                                  |
| ----------------- | ------------------------------------ | ---------------------------------------------------------------------------------------- |
| **Quartz global** | Top-left of primary display, Y down  | `CGEventGetLocation`, `CGEventCreateMouseEvent`, `CGWindowList` bounds, AX position/size |
| **Cocoa global**  | Bottom-left of primary display, Y up | `NSScreen.frame()`, `NSWindow` frames, `NSView` drawing                                  |


**Design rule:** store `AbsoluteConfig` region and all window geometry in **Quartz**. Convert via `ui/display_bounds.py` only when placing/drawing AppKit UI:

```python
cocoa_y = primary_height - quartz_y - height
```

`desktop_bounds()` returns the Cocoa union of all `NSScreen` frames (for sizing overlay windows).

---

## UI Architecture

### Menu Bar App (`app.py` + rumps)

- Icon from `assets/menu_icon.png` (template image); title is `None` (icon only).
- Menu rebuilt on each status change (fresh `MenuItem` instances — rumps items cannot be reused across menus).
- `AppHelper.callAfter()` marshals menu updates to the main thread.

### Dock Policy (`ui/dock_policy.py`)

- Default: `NSApplicationActivationPolicyAccessory` (menu bar only).
- Settings window open: `NSApplicationActivationPolicyRegular` (Dock icon).
- Uses `NSApplication.sharedApplication()` (not `NSApp()`, which is `None` before the run loop starts).

### Settings Window (`Sources/ReawaApp/SettingsUI.swift`)

Native `NSWindow` hosting SwiftUI, with:

- A tabbed settings surface: **Connections**, **App Behavior Log**, and **Pen Event Log**
- A two-pane connection editor: discovered devices and saved connections on the left, active connection form on the right
- Immediate-apply editing for existing connections; new connections still use **Add connection**
- Segmented **Relative | Absolute** mode control; switching to Absolute still enters the snap-picker flow when that connection is active
- A higher-level **Tablet orientation** picker replacing raw `swap_xy` / `invert_x` / `invert_y` controls
- Absolute-mode context such as the snapped-window reference and border color in the same editor
- `window.isReleasedWhenClosed = false` in `SettingsWindowController`, so reopening the window reuses the controller safely

### Logging / Diagnostics (`Sources/ReawaApp/Logging.swift` + log tabs)

The Swift app now has two separate in-memory log channels:

- **Behavior log** — always on; intended for mode changes, settings changes, connection/session/SSH events, notifications, device detection, and Absolute-mode picker / snapped-window lifecycle
- **Pen event log** — off by default; enabled from the UI and intended for RM2 event debugging

Pen-event presentation is driven by the richer parser state:

- Raw Linux event names such as `EV_KEY BTN_STYLUS 1`
- Accumulated semantic state such as `PEN TOUCH (x, y) = (...)`
- Recognized gesture-state labels such as `START`, `MOVE`, `END`, and `OUT`
- Observed capability chips (`BTN_STYLUS`, `ABS_TILT_X`, `ABS_DISTANCE`, etc.) which can also be clicked in the UI to prefill log search

---

## Services

### AppLogger

`AppLogger` is now a small logging hub rather than a single flat list:

- `behaviorEntries` — capped, always-on stream for product / app behavior debugging
- `penEntries` — separate capped stream for high-frequency pen diagnostics
- `penLoggingEnabled` — runtime toggle, off by default
- `penCapabilityLabels` — observed event-family / capability labels derived from the RM2 stream

High-frequency pen logs are appended through a locked background-safe store and then published back to SwiftUI as debounced main-actor snapshots. This keeps the pen log usable without pushing every event directly through a main-thread-only observable list.

### ConnectionManager

- Single active connection and `DriverSession`.
- `_active_conn` in-memory cache updated on `update_connection()` — session reads live config without re-parsing JSON.
- `pause_input()` / `resume_input()` delegate to session.
- Listeners notify UI (menu bar refresh, overlay update).
- `_check_session()` polls session state after connect (0.5 s intervals, up to 30 attempts).

### USBWatcher

Background poll (~3 s):

1. Scan USB Ethernet subnets for SSH hosts (`discover_usb_ssh_hosts()` preferring non-primary `en`* interfaces).
2. Update reachability per saved connection IP.
3. Auto-connect or notify when device appears/disappears.
4. Disconnect active session when device goes offline.

### KeychainStore

Passwords keyed by connection ID, service name `remarkable-rm2`. Used only for first-time RSA key install via `rm2.setup_key()`.

### Network Discovery

Parses `ifconfig` for `en*` interfaces (skips `lo0`, `utun*`, `bridge0`, etc.), derives subnets, probes port 22 with thread pool (max 64 hosts, 0.35 s timeout per host). Gateway (.1) probed first.

---

## Threading Model


| Thread                  | Work                                                     |
| ----------------------- | -------------------------------------------------------- |
| Main (AppKit / rumps)   | Menu bar, settings window, overlays, snap picker, timers |
| `DriverSession._thread` | SSH read loop, pen → mouse translation                   |
| USB watcher             | Reachability scan                                        |
| Settings scan           | `discover_usb_ssh_hosts()` in worker thread              |


Cross-thread rules:

- UI mutations always on main thread (`AppHelper.callAfter`).
- Config changes go through `ConnectionManager.update_connection()` which updates `_active_conn`; session sees changes on next pen frame.
- Do **not** disconnect/reconnect SSH to change output mode — live driver swap handles it.
- Pen-event logs are appended off the session thread into a locked store, then surfaced to SwiftUI as main-actor snapshots.

---

## Storage

```
~/Library/Application Support/remarkable-rm2/
  connections.json
  keys/<connection-id>/id_rsa
  keys/<connection-id>/id_rsa.pub
```

Legacy migration: project-local `.rm2_config.json` + `.ssh/id_rsa` → first connection on first run.

---

## RM2 Device Constants


| Constant       | Value               | Meaning                                  |
| -------------- | ------------------- | ---------------------------------------- |
| `PEN_X_MAX`    | 20967               | Digitizer X range                        |
| `PEN_Y_MAX`    | 15725               | Digitizer Y range                        |
| `RM2_ASPECT`   | 20967/15725         | Portrait aspect                          |
| `RM2_DPI`      | 2531                | Digitizer DPI for scale auto-calculation |
| `RM2_USER`     | `root`              | SSH user                                 |
| `RM2_PEN_FILE` | `/dev/input/event1` | Pen input device                         |
| `SSH_KEY_BITS` | 3072                | RSA key size                             |


---

## Dependencies


| Package                                | Role                                  |
| -------------------------------------- | ------------------------------------- |
| `paramiko`                             | SSH to RM2                            |
| `pyobjc-framework-Quartz`              | CGEvent, CGWindowList, display bounds |
| `pyobjc-framework-Cocoa`               | AppKit UI                             |
| `pyobjc-framework-ApplicationServices` | Accessibility (`AXUIElement*`)        |
| `rumps`                                | Menu bar app shell                    |
| `keyring`                              | macOS Keychain passwords              |


---

## Entry Points

```bash
PYTHONPATH=.. python -m reawa                   # menu bar app from repo root
PYTHONPATH=packaging/src python -m reawa.driver # CLI: connect to first saved connection
```

---

## Key Design Decisions

### 1. Live driver swap instead of SSH reconnect

Early versions reconnected SSH after snapping or changing mode. That caused races: overlay state reset while the new session was still connecting, leaving input paused and the picker re-triggered. The session now swaps `RelativeDriver` ↔ `AbsoluteDriver` in place.

### 2. ABSOLUTE always requires a snapped window

There is no standalone absolute region. Entering ABSOLUTE always runs the picker; cancel reverts to RELATIVE. See [doc.md](doc.md) for product rationale.

### 3. Pause input during picking, not disconnect

`DriverSession.pause()` discards pen frames while the user selects a window. SSH stays connected so resume is instant.

### 4. CGWindowList for enumeration

Per-app `NSRunningApplication` enumeration was unreliable (missing API, wrong z-order). `CGWindowListCopyWindowInfo` gives correct stacking and multi-display bounds.

### 5. In-memory active config cache

`ConnectionManager._active_conn` avoids reading `connections.json` every pen frame (~60+ Hz). Disk is updated on save; session reads memory.

### 6. Plain Python overlay controllers

`RegionOverlayController` and `SnapPicker` are not `NSObject` subclasses to avoid PyObjC selector collisions.

### 7. Picker lifecycle via `_cancel_pick()`

Exiting the snap picker through Relative toggle or Choose window must call `_cancel_pick()` to clear `_picking`. See [Bug Fix History](#bug-fix-history).

### 8. AXValueRef unwrapping for window frames

Accessibility position/size attributes may return opaque `AXValueRef` objects on some macOS/PyObjC versions. `window_frame()` unwraps them with `AXValueGetValue` before reading coordinates.

### 9. Per-screen picker windows

One overlay window per `NSScreen` ensures every display receives mouse events for window selection. See [Bug Fix History](#bug-fix-history).

### 10. Close detection via AX, not CGWindowList

Stage Manager removes windows from all `CGWindowList` queries when staged, so window-number absence is not a reliable close signal. AX element invalidity (`kAXErrorInvalidUIElement`) is the authoritative close check.

---

## Bug Fix History

These issues blocked ABSOLUTE mode end-to-end until fixed. Documented here for porting reference.


| #   | Symptom                                                                                             | Root Cause                                                                                                                                                                                                                                                                                                                                                                       | Fix                                                                                                                                                                                                             |
| --- | --------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1   | Primary-display windows not pickable (only secondary worked)                                        | A single full-desktop picker window is only click-interactive on the display holding most of its area ("Displays have separate Spaces"); the larger secondary display captured all clicks. Cursor polling is global, so highlighting worked everywhere and hid the routing failure. The `constrainFrameRect:toScreen:` override fixed overlay *geometry* but not *event routing* | Create **one picker window per `NSScreen*`* so every display receives mouse events; broadcast the highlight to all per-screen views                                                                             |
| 2   | Snapped window never resized/moved to the region                                                    | `sync_window_to_region()` passed raw `CGPoint`/`CGSize` to `AXUIElementSetAttributeValue`; AX requires `AXValueRef`, so writes were silently dropped                                                                                                                                                                                                                             | Wrap with `AXValueCreate(kAXValueCGPointType/CGSizeType, ...)`; set size → position → size to avoid clamping                                                                                                    |
| 3   | "Choose window" / re-entering Absolute did nothing                                                  | `_picking` stuck `True` after leaving picker via Relative/Choose window                                                                                                                                                                                                                                                                                                          | `_cancel_pick()` in `_revert_to_relative()` and `_restart_pick()`                                                                                                                                               |
| 4   | Clicking a window did not snap; UI stuck on "selecting…"; no pen output                             | `_on_picked` raised `AttributeError` on `AXValueRef.x` in `window_frame()`                                                                                                                                                                                                                                                                                                       | `_unwrap_point()` / `_unwrap_size()` using `AXValueGetValue`                                                                                                                                                    |
| 5   | Session stayed paused after failed snap                                                             | Exception in `_on_picked` before `resume_input()`; picker already stopped                                                                                                                                                                                                                                                                                                        | Fixed by AX unwrap (#4); `_cancel_pick()` prevents stuck `_picking` on other exit paths                                                                                                                         |
| 6   | Relative worked; Absolute had wrong/stale region                                                    | Mode saved via settings without a successful snap; stale region coords used                                                                                                                                                                                                                                                                                                      | Successful snap now completes the flow; region set from picked window bounds                                                                                                                                    |
| 7   | Settings window segfault on reopen                                                                  | `setReleasedWhenClosed_(True)` deallocated window on close                                                                                                                                                                                                                                                                                                                       | `setReleasedWhenClosed_(False)`; reuse window instance                                                                                                                                                          |
| 8   | Paste (⌘V) failed in settings text fields                                                           | rumps builds only the status-bar menu, never the app's main Edit menu                                                                                                                                                                                                                                                                                                            | Install standard Edit menu with nil-target items for responder-chain dispatch                                                                                                                                   |
| 9   | Minimize to Stage Manager reverted to RELATIVE                                                      | Close detection used `CGWindowList` membership; Stage Manager removes staged windows from every list query even though the AX element is still valid                                                                                                                                                                                                                             | Close only via `_element_alive()` (`kAXErrorInvalidUIElement` on `kAXRoleAttribute`); never treat window-number absence as closed                                                                               |
| 10  | Stage Manager minimize did not hide overlay (Cmd+H worked)                                          | `kAXMinimizedAttribute` stays false; window remains in `kCGWindowListOptionOnScreenOnly`; per-window `kCGWindowIsOnscreen` flag is often absent                                                                                                                                                                                                                                  | First attempt: strip-thumbnail delta on same display + loss of frontmost (see #11–12)                                                                                                                           |
| 11  | Overlay stuck after minimizing target to Stage Manager when another window was minimized first (R1) | Strip-delta heuristic required `target_is_frontmost=False`; after minimizing the other window the target app stayed frontmost, so staging was never detected                                                                                                                                                                                                                     | Rejected strip-delta-only approach; see #12                                                                                                                                                                     |
| 12  | Overlay hidden when focusing another visible window in the same Stage Manager group (R2)            | Lingering `new_strip` delta from unrelated windows + `not frontmost` matched staging even though target CG bounds were still full-size                                                                                                                                                                                                                                           | **Final fix:** `_is_stage_manager_minimized()` compares live CGWindow on-screen area to AX frame area; staged when ratio below `_STAGE_AREA_RATIO` (0.5). Target-specific; unaffected by other windows or focus |
| 13  | `kAXFullScreenAttribute` import crash on some PyObjC builds                                         | Constant not exported by all `ApplicationServices` bindings                                                                                                                                                                                                                                                                                                                      | Use raw attribute string `"AXFullScreen"`                                                                                                                                                                       |
| 14  | Picking a Stage Manager thumbnail snapped to tiny bounds                                            | Picker lists strip thumbnails as on-screen windows; pick returns thumbnail-sized bounds                                                                                                                                                                                                                                                                                          | `restore_window()` on pick: clear minimized, raise, activate app, `_await_stable_frame()` before snapping                                                                                                       |
| 15  | Swift relative mode felt under-scaled compared with Python                                          | The Swift port re-read `CGEventGetLocation` on every pen frame. Posted Quartz mouse events may not advance the live cursor immediately, so each new frame sometimes started from a stale cursor position and lost part of the intended delta accumulation                                                                                                                       | Stop using the live cursor as the per-frame movement baseline; compute relative motion from gesture anchors instead                                                                                              |
| 16  | Swift relative mode teleported back after trackpad / mouse interference                             | The first scale fix kept a synthetic cursor alive across pen hover frames. If another input device moved the real cursor mid-gesture, the next pen move resumed from the stale synthetic cursor and jumped back before continuing                                                                                                                                             | Model explicit hover/touch gesture lifecycles in `RelativePenDriver` and rebase the current gesture to the live cursor whenever external input moves the cursor away from the gesture's expected position        |


**Verified working after fixes:** snap picker opens on all displays, window hover highlight, click-to-snap, pen → mouse in ABSOLUTE mode, window move → region follow, minimize/restore/maximize/close lifecycle, Stage Manager thumbnail restore-on-pick, Stage Manager minimize/restore with other windows on screen (R1/R2 scenarios).

For the full investigation narrative (signals tried, runtime evidence, rejected approaches), see [macos-window-lifecycle.md](macos-window-lifecycle.md).

---

## Known Limitations

- Window picking lists normal layer-0 windows only (no menu bar, dock, or overlay windows).
- AX window resolution matches by frame proximity; untitled or duplicate frames may pick the wrong element.
- Stage Manager and some full-screen spaces may affect window enumeration.
- Region resize handles use Quartz math; extreme multi-display layouts may need further coordinate testing.
- Single active connection: connecting to B disconnects A.
- Some apps enforce minimum/maximum window sizes, so the snapped window may not match the region exactly even after `sync_window_to_region()`.
- Border style (`solid`/`dashed`) is configurable in the data model but has no settings UI picker yet.
- `border_style` field exists in `AbsoluteConfig` but only `border_color` is exposed in settings.

---

## Related Documents

- [doc.md](doc.md) — product description, user-facing behavior, and product decisions
- [macos-window-lifecycle.md](macos-window-lifecycle.md) — how we detect minimize/maximize/close/restore under Stage Manager (investigation process and signal reference)

