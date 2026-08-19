---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.2.0
lifecycle: active
needs_design: false
---

# SRS — Tablet sync Infini (UI)

Debug chrome for [REQ-03](../../prd.md#tablet-sync) is [SRS-IN-18](#srs-in-18-device-log-panel) (`needs_design: false`).
Pen-button map editor is [SRS-IN-24](#srs-in-24-pen-map-ui) (`needs_design: yes`) — not document chrome.

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
- Pen-button **map editor** — that is [SRS-IN-24](#srs-in-24-pen-map-ui), not this overlay

### Platform

Desktop Electron, pointer + keyboard. Overlay is a single scene with the states above —
not a multi-scene graph.

---

## [SRS-IN-24] Pen-button map settings {#srs-in-24-pen-map-ui}

<!-- lifecycle: active -->
<!-- needs_design: yes -->

**Parent:** Infini [REQ-05](../../prd.md#pen-button-map). **Logic:** [SRS-IN-23](./srs-logic.md#srs-in-23-pen-map-publish). **Quality:** [SRS-IN-25](./srs-quality.md#srs-in-25-map-publish-quality). **Do not parent on [SRS-IN-05](../vector-document/srs-ui.md)** (open/save). **Platform:** **desktop** Electron (`data-platform: desktop`). Catalogues: [domain/pen-button-map](../../../../domain/pen-button-map.md).

### Purpose

Assign each **present** barrel slot (Click and Hold-move) to **exactly one** closed-catalogue item and save. Not document chrome.

### Closed catalogues (Designer must not add items)

**Click:** `toggle_pen_freeform` · `toggle_pen_eraser` · `undo` · `off`  
**Hold-move:** `temp_sel_freeform` · `temp_sel_rect` · `temp_erase` · `drag_node_under_tip` · `off`

### Layout (contract, not craft)

| Region | Role |
|---|---|
| MapEditor | One row per present button (1 or 2) |
| SlotClick | Closed list for Click |
| SlotHoldMove | Closed list for Hold-move |
| Save | Persist + publish |

0-button: slots **absent or disabled** (0 fake bindings). Hover **required** on desktop. Targets ≥24 px.

### States (PRD)

`map.layout_0` · `map.layout_1` · `map.layout_2` · `map.slot_click` · `map.slot_hold` · `map.invalid_stale` · `map.offline_then_publish`

### UI-driving fields

`pen.buttonCount`, `pen.map.buttons[].click`, `pen.map.buttons[].holdMove` — Designer must not invent a third slot type (e.g. “click-hold-move”).

### Anti-patterns

- 5-way radio on the tablet chip
- Saving the map into the SVG
- Document open/save mixed into this surface

---

## Superseded

v0.1.0 "no dedicated UI" is replaced by [SRS-IN-18](#srs-in-18-device-log-panel) for debug
chrome only. Connection-status-on-canvas remains Could / out of this section.
