---
id: UI-EP-01
screen: epaper-tool-strip
module: epaper
parent_srs: [SRS-EP-05]
parent_req: [REQ-03]
story: STORY-EP-003
fidelity: hifi
platform: epaper
version: 0.4.0
amends: CHL-0003
---

# UI Spec — Epaper floating tool chip `[UI-EP-01]`

**v0.4 (human verify 2026-08-11):** chip height **64px** (doubled from v0.3 32px — touch targets too small on device); icons = designer `icon-epaper-*.svg` (raster in app).
**v0.3 (human 2026-08-11):** chip height **32px**; navigator preview = **RM2 landscape tablet** (1872×1404), not mobile.
**PM adopted** [CHL-0003](../../challenges/CHL-0003-epaper-floating-toolchip.md) 2026-08-11 — Spec aligns with SRS-EP-05.

## Platform profile

| Field | Value |
|---|---|
| Profile | **epaper-device** (reMarkable 2) |
| `data-platform` | `epaper` |
| Input | Finger preferred; **pen-on-chip** fallback. No hover |
| Color | 1-bit |
| Motion | None |
| Preview | `tablet` @ 100% · **landscape** 1872×1404 (native panel 1404×1872 rotated) |

## Layout regions

| Region | Parent | Notes |
|---|---|---|
| DeviceScreen | panel | Landscape preview frame; `data-orientation` = active gut pose |
| InkSurface | DeviceScreen | **Full-bleed** (not shrunk) |
| ToolStrip | DeviceScreen | Floating **squared** chip — hug width, height **64px**, 64×64 icon tiles, radius 0 |
| SelectionOverlay | InkSurface | Bounds / handles / ghost |
| StatusLine | DeviceScreen | Existing status |

## Placement (binding — human)

| Rule | Value |
|---|---|
| Size | Height **64px**; each tool **64×64** square; chip + buttons `border-radius: 0` |
| Anchor | **Top of the activated orientation** — centered on that edge, inset ~8 px |
| gutToLeft / gutToRight / gutAtBottom | Chip on oriented **top** |
| gutOnTop | Oriented “top” is the opposite short edge → chip near **bottom** |
| Float | Over ink; hit-test excludes chip bounds from stroke start (logic) |

## Scenes

| State id | File |
|---|---|
| `tool.pen` | `epaper-tool-strip-pen.html` (**hifi**) |
| `tool.ink_box` | `epaper-tool-strip-ink-box.html` |
| `tool.selection.idle` | `epaper-tool-strip-selection-idle.html` |
| `tool.selection.selected` | `epaper-tool-strip-selection-selected.html` |
| `tool.selection.dragging` | `epaper-tool-strip-selection-dragging.html` |
| `tool.selection.empty` | `epaper-tool-strip-selection-empty.html` |
| `session.down` | `epaper-tool-strip-session-down.html` |
| `touch.unavailable` | `epaper-tool-strip-touch-unavailable.html` |
| `orient.gutOnTop` | `epaper-tool-strip-orient-gut-on-top.html` |
| states | `epaper-tool-strip-states.html` |

## Components

| Name | Path |
|---|---|
| ToolChip (ToolStrip) | `components/tool-strip.html` |
| ToolButton | `components/tool-button.html` |
| SelectionOverlay | `components/selection-overlay.html` |

## Icons

| Icon | Path |
|---|---|
| Selection rect / freeform / Pen / Ink-box | `../system/assets/icon-epaper-sel-rect.svg`, `icon-epaper-sel-freeform.svg`, `icon-epaper-pen.svg`, `icon-epaper-ink-box.svg` |

## SRS delta

| Topic | Prior SRS | This Spec (human) | Gap |
|---|---|---|---|
| Full-band strip | required | **retired** → floating chip | CHL-0003 |
| InkSurface shrink | required | **full-bleed** | CHL-0003 |
| No float | anti-pattern | **float allowed** for chip | CHL-0003 |
| ≥120 px targets | binding | **64px** chip (icon-only; human verify doubled from 32) | CHL-0003 + verify |
| Orientation-top | not stated | **binding** | CHL-0003 |
