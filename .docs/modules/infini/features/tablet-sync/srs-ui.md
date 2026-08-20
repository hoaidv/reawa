---
feature: tablet-sync
parent_req: [REQ-03, REQ-06]
version: 0.3.0
lifecycle: active
needs_design: true
---

# SRS — Tablet sync Infini (UI)

Debug chrome for [REQ-03](../../prd.md#tablet-sync) is [SRS-IN-18](#srs-in-18-device-log-panel) (`needs_design: false`).
Pen-button map **editor** is **not** Infini — [SRS-IN-24](#srs-in-24-pen-map-ui) is **retired**. Persist/restore is [SRS-IN-23](./srs-logic.md#srs-in-23-pen-map-publish) (`needs_design: no`).
Viewport-follow toggle is [SRS-IN-27](#srs-in-27-follow-toggle) (`needs_design: yes`) — **not** canvas chrome, **not** IN-034.

Logic: [SRS-IN-17](./srs-logic.md#srs-in-17-debug-log-channel). Quality: [SRS-IN-19](./srs-quality.md#srs-in-19-debug-log-isolation).

---

## [SRS-IN-18] Device Log button and overlay panel {#srs-in-18-device-log-panel}

Parent REQ: [REQ-03](../../prd.md#tablet-sync).

### Purpose

Open a **full-size in-app panel** (same Infini window — **not** a second `BrowserWindow`)
that shows the device console stream from TCP `:9878`. One job: inspect. Not editing, not
session status for the creator on the tablet.

### Layout regions (containment tree)

| # | Region id | Parent | Contents |
|---|---|---|---|
| 0 | WindowFrame | screen | Existing Electron client ([SRS-IN-02](../infinity-canvas/srs-ui.md)) |
| 1 | DeviceLogButton | WindowFrame | Toggle control; visible while the window is focused |
| 2 | DeviceLogOverlay | WindowFrame | Full-size panel covering the canvas **when open**; unmounted / `hidden` when closed |
| 3 | DeviceLogToolbar | DeviceLogOverlay | Title, filter field, close |
| 4 | DeviceLogStream | DeviceLogOverlay | Scrollable log lines (or empty / disconnected copy) |

**Containment:** `DeviceLogOverlay` is a child of `WindowFrame`, not of `CanvasStage` /
`WorldLayer`. It does not pan with the world. While open it covers the client area
(≥90% of `WindowFrame`; full-bleed is allowed). Canvas may stay mounted underneath but is
**not** interactive until the overlay closes.

`DeviceLogButton` sits in a window-chrome corner (top-leading or top-trailing). It must
not sit inside `WorldLayer`. Coexist with `StatusZoom` without covering it when the panel
is **closed**.

### Closed control inventory

| id | Label | Role |
|---|---|---|
| `btn.device_log` | Device Log | Toggle overlay open / closed |
| `btn.device_log_close` | Close | Same as toggle-to-closed (toolbar) |
| `btn.device_log_clear` | Clear | Empty the in-memory ring (view + process buffer) |
| `field.device_log_filter` | Filter | Substring search over the in-memory buffer |
| `list.device_log_stream` | (none) | Read-only lines |

No persist, no export, no second window, no on-device chrome. Clear is allowed (debug chrome).

### Copy table (en)

| Key | String |
|---|---|
| `copy.device_log.button` | `Device Log` |
| `copy.device_log.title` | `Device Log` |
| `copy.device_log.filter` | `Filter` |
| `copy.device_log.empty` | `No device log yet` |
| `copy.device_log.disconnected` | `Device log not connected` |
| `copy.device_log.disconnected_hint` | `Enable EPAPER_DEBUG_LOG on the tablet and keep Infini listening on TCP 9878.` |
| `copy.device_log.close` | `Close` |
| `copy.device_log.clear` | `Clear` |
| `copy.device_log.filter_empty` | `No lines match the filter` |

### Interaction map

| Control | Action | Destination | Side-effect | Feedback |
|---|---|---|---|---|
| `btn.device_log` (closed) | click / Enter | overlay open | send `debug_request` then `debug_start` | hover lift · press tint · focus ring |
| `btn.device_log` (open) / `btn.device_log_close` / Escape | click / Escape | overlay closed | send `debug_stop`; **buffer kept** | same |
| `btn.device_log_clear` | click | in-scene | empty Infini ring (main + overlay); device not asked | press tint · focus ring |
| `field.device_log_filter` | input | in-scene filter | view-only; buffer unchanged | caret · focus ring |
| `list.device_log_stream` | scroll | in-scene | none | native scroll |

### States matrix

| State id | Overlay | Stream body | Shipping |
|---|---|---|---|
| `dlog.closed` | hidden | — | `debug_stop` (or never started) |
| `dlog.open_empty` | visible | empty copy | `debug_start` sent; 0 lines in buffer |
| `dlog.streaming` | visible | lines appending | shipping on; ≥1 line in buffer |
| `dlog.disconnected` | visible | disconnected copy + hint | no `:9878` socket (or dropped) |
| `dlog.filtered` | visible | subset of buffer, or filter-empty copy | shipping may still be on; filter string non-empty |

`dlog.filtered` may stack with streaming or disconnected (filter still runs on whatever
is in the buffer). Designer scenes are **not** required.

### Filter / search

Case-insensitive substring on `msg` (and, if the implementer shows them, `level`). Clearing
the field restores the full buffer view. Filter never round-trips to the device.

### Anti-patterns

- Opening a second Electron window or OS terminal for this surface
- Mixing `debug_*` onto TCP `:9877` or into `TabletSession`'s document decoder
- Canvas-world chrome (markers, selection) inside the overlay
- On-panel Epaper undo / Device Log chrome (tablet has **0** of this UI)
- Persist-to-disk, upload, or apply-to-document from a log line
- Pen-button **map editor** — retired here ([SRS-IN-24](#srs-in-24-pen-map-ui)); on-device [SRS-EP-52](../../../epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor)

### Platform

Desktop Electron, pointer + keyboard. Overlay is a single scene with the states above —
not a multi-scene graph.

---

## [SRS-IN-24] Pen-button map settings {#srs-in-24-pen-map-ui}

<!-- lifecycle: retired -->
<!-- superseded-by: [SRS-EP-52] -->
<!-- retired: 2026-08-20 — desktop map-editor UI outcome of Infini REQ-05 retired in place; persist/restore remains [SRS-IN-23]. Id kept. -->

**Parent:** Infini [REQ-05](../../prd.md#pen-button-map). **Do not implement this surface.** Infini presents **0** map-editor screens. The on-device editor is [SRS-EP-52](../../../epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor) (Epaper [REQ-18](../../../epaper/prd.md#pen-buttons)). Persist/restore: [SRS-IN-23](./srs-logic.md#srs-in-23-pen-map-publish). **Platform of this retired section:** desktop Electron — **not** the shipping UI.

### Purpose (historical — do not paint)

Assign barrel slots on the **desktop**. Superseded: tablet authors; Infini only stores.

### Closed catalogues (historical — do not use)

Click included `undo`. Hold-move included `temp_sel_freeform` / `temp_sel_rect`. Current catalogues live in [domain/pen-button-map](../../../../domain/pen-button-map.md).

### States (historical)

`map.layout_0` · `map.layout_1` · `map.layout_2` · `map.slot_click` · `map.slot_hold` · `map.invalid_stale` · `map.offline_then_publish` — **do not design** these on Infini. Epaper editor states are on [SRS-EP-52](../../../epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor).

### Anti-patterns (still binding)

- Painting Infini slate/desktop map chrome
- Saving the map into the SVG
- Treating this id as `needs_design: yes`

---

## [SRS-IN-27] Viewport-follow Epaper toggle {#srs-in-27-follow-toggle}

<!-- lifecycle: active -->
<!-- needs_design: yes -->

**Parent:** [REQ-06](../../prd.md#viewport-follow). **Logic:** [SRS-IN-26](./srs-logic.md#srs-in-26-viewport-follow). **Quality:** [SRS-IN-28](./srs-quality.md#srs-in-28-follow-quality). **Platform:** **desktop** Electron (`data-platform: desktop`). **Do not parent on [SRS-IN-02](../infinity-canvas/srs-ui.md)** (canvas chrome) or [SRS-IN-24](#srs-in-24-pen-map-ui) (pen-button map). **Do not add this control to IN-034.**

### Purpose

One job: let the creator **opt in** to matching the connected tablet’s drawing region, or see that follow is off. Not a tool, not a recognizer, not a hand-tool tile, not Infini→Infini follow.

### Composition / containment (contract, not craft)

| Region | Parent | Role |
|---|---|---|
| WindowFrame | screen | Existing Electron client ([SRS-IN-02](../infinity-canvas/srs-ui.md)) |
| FollowToggle | WindowFrame | Icon toggle — **not** inside `WorldLayer`, **not** Device Log, **not** pen-map editor |
| StatusZoom | WindowFrame | Unchanged |

**Placement vs StatusZoom is a design story.** Binding: pointer hit ≥24 px. Hover **required** on desktop.

### Closed control inventory

| id | Kind | Notes |
|---|---|---|
| `btn.viewport_follow` | icon toggle | Off / following Epaper / peer-following-you (pressing on turns Epaper follow off) |

No extra follow-mode canvas overlay required. Designer may reuse the existing drawing-region marker only if it does not imply always-on match.

### States matrix (journeys from PRD — do not add)

| State id | When |
|---|---|
| `follow.off` | Default; both off; or after explicit off |
| `follow.following_epaper` | `direction = epaper_to_infini` |
| `follow.peer_following_you` | `direction = infini_to_epaper` — this toggle is off; enabling it turns Epaper follow off |
| `follow.local_nav_turns_off` | Infini pan/pinch while following → off |
| `follow.connection_lost` | Session dropped → forced off |
| `follow.reconnect_stays_off` | Link back; still off until opt-in |

### Interaction map

| Control | Action | Effect |
|---|---|---|
| `btn.viewport_follow` (off, session live) | click | → `follow.following_epaper` |
| `btn.viewport_follow` (following) | click | → `follow.off` |
| `btn.viewport_follow` (peer following you) | click | → `follow.following_epaper` (peer off) |
| No session | click or look | off or unavailable; 0 follow-on |

### UI-driving fields

`follow.direction`, `session.connected` — Designer must not invent Infini→Infini follow or a ToolChip tile.

### Anti-patterns

- ToolChip exclusive tool / recognizer / hand-tool tile on either peer
- Restoring follow on reconnect without a click
- Painting last-writer / token chrome
- Dual-on presentation (both toggles “on”)
- Mixing this control into Device Log or pen-button map

### Dual-ask

`/designer` Spec + one scene HTML per state id. `/qa` BDD from [REQ-06](../../prd.md#viewport-follow) AC + [SRS-IN-28](./srs-quality.md#srs-in-28-follow-quality).

---

## Superseded

v0.1.0 "no dedicated UI" is replaced by [SRS-IN-18](#srs-in-18-device-log-panel) for debug
chrome only. Connection-status-on-canvas remains Could / out of this section.
