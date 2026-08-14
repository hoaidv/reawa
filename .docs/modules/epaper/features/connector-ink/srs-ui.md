---
feature: connector-ink
parent_req: [REQ-09]
version: 0.1.0
lifecycle: active
needs_design: true
---

# SRS — Connector-ink (UI)

Durable UI contract for recognition blink and selected-connector chrome.
ToolChip inventory lives in [SRS-EP-05](../tool-modes/srs-ui.md) ([ADR-0021](../../../../adr/ADR-0021-connector-toolchip.md)).
Logic: [SRS-EP-17 / SRS-EP-18](./srs-logic.md). Quality: [SRS-EP-20](./srs-quality.md).
Platform: **reMarkable 2 e-ink** (touch + pen; 1-bit; partial refresh) — same profile as
[SRS-EP-12](../ink-box/srs-ui.md).

## [SRS-EP-19] Connector chrome {#srs-ep-19-connector-chrome}

### Purpose

Announce that a connector was created, then let the creator change Ink/Curve and per-end
kind — without a fifth exclusive tool and without naming the auto-picked style at create time.

### Closed control inventory

| id | Kind | Notes |
|---|---|---|
| `ovl.conn_blink` | one-shot flash | Connector stroke + both bound SmartGroups, **once**. Partial refresh. No copy. |
| `tgl.conn_style` | two-state | **Ink** \| **Curve** on a **selected** connector |
| `tgl.conn_end_kind` | per end | **Edge** \| **Centre** on a selected connector |
| `recog.connector` | ToolChip toggle | Label "Connector recognition" — owned by SRS-EP-05 |

No toast, no persistent badge, no style name in the blink.

### States matrix

| State id | Blink | Style control | End-kind | Notes |
|---|---|---|---|---|
| `conn.created` / `conn.blink` | once | hidden | hidden | Then idle document |
| `conn.selected` | off | visible, shows current | visible both ends | ToolLayer |
| `conn.style.ink` / `conn.style.curve` | off | selected side | — | One undo |
| `conn.live_warp` | off | hidden unless selected | — | Follows live box on ToolCanvasLayer |
| `conn.rejected` | none | none | none | Stroke is ordinary ink |
| `conn.orphan` | off | still selectable | ends may show missing node | Still drawn |

### Interaction map

| Control | Action | Destination | Feedback |
|---|---|---|---|
| `ovl.conn_blink` | none | — | One waveform pulse; designer owns duration |
| `tgl.conn_style` | set `warpStyle` | stay selected | Press + Mono settle |
| `tgl.conn_end_kind` | set that end's kind | stay selected | Press + Mono settle |
| Empty canvas | deselect | `sel.none` | Chrome gone next settled frame |

### Anti-patterns

- Naming Ink/Curve in the create blink
- A fifth exclusive "connector tool"
- Full-panel flash
- Banner / modal on guard fail

### Out of scope

Arrowheads, dash picker, width presets, routing-style names other than Ink/Curve.

### Trace

- Design package: `.plan/iter-004/design/connector-chrome/`
- ToolChip package: `.plan/iter-004/design/toolchip-recognizers/`
