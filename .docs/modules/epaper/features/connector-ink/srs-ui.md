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

<!-- CHL-0022: “Arrowheads” below is superseded as a product out-of-scope for REQ-13. Parent of endpoint styles is [SRS-EP-36](#srs-ep-36-endpoint-toolbar), not this section. Dash picker / width / extra routing names remain out. -->

Dash picker, width presets, routing-style names other than Ink/Curve. **Endpoint styles** are specified in [SRS-EP-36](#srs-ep-36-endpoint-toolbar).

### Trace

- Design package: `.plan/iter-004/design/connector-chrome/`
- ToolChip package: `.plan/iter-004/design/toolchip-recognizers/`

---

## [SRS-EP-36] Endpoint style toolbar {#srs-ep-36-endpoint-toolbar}

<!-- lifecycle: active -->
<!-- needs_design: yes -->
<!-- campaign: not Epaper this lock — Infini / web-desktop Path A later -->

**Parent:** [REQ-13](../../prd.md#connector-ends) Path A. **Logic:** [SRS-EP-34](./srs-logic.md#srs-ep-34-end-styles). **Quality:** [SRS-EP-37](./srs-quality.md#srs-ep-37-endpoint-quality).

**Epaper this campaign: do not implement.** Path B has no chrome ([SRS-EP-74](./srs-product.md#srs-ep-74-endpoint-ink-product) BR-E07). This section is Infini / web-desktop.

### Purpose

Set **each end** independently after create or on selected connector (desktop). Epaper Path B does not announce accept/refuse.

### Closed style set (ids — do not invent)

`none` · `arrow` · `arrow_empty` · `star` · `one` · `many`

### Closed controls

| id | Notes |
|---|---|
| `toolbar.conn_end_style` | Post-create or selected-connector; **per end**; items from the closed set |
| `ind.endpoint_ink_ok` | Path B accepted |
| `ind.endpoint_ink_refuse` | Stroke stayed ordinary ink / connector — no banner required |

Hit: style chips ≥64 du if finger; otherwise pen-only. End-kind Edge/Centre stays [SRS-EP-19](#srs-ep-19-connector-chrome) (`tgl.conn_end_kind`).

### States (PRD)

`conn.post_create_toolbar` · `conn.end_style_start` / `conn.end_style_finish` · `endpoint_ink.accepted` · `endpoint_ink.refused` · `conn.warp_decorated_ends`

### Anti-patterns

- One style applied to both ends with no per-end control
- Stealing spine ink as an “end” in the UI copy

---

## [SRS-EP-39] Attachment bind chrome {#srs-ep-39-attachment-ui}

<!-- lifecycle: active -->
<!-- needs_design: yes -->

**Parent:** [REQ-14](../../prd.md#connector-attachments). **Logic:** [SRS-EP-38](./srs-logic.md#srs-ep-38-attachment-t). **Quality:** [SRS-EP-40](./srs-quality.md#srs-ep-40-attachment-quality). **Platform:** epaper-device.

### Closed controls

| id | Notes |
|---|---|
| `cta.attach_to_connector` | Place/bind — Designer entry; ≥64 du if finger |
| `ovl.attachment_on_spine` | Selected attachment riding the connector |

No rebake affordance. Designer must not invent a “reshape rest” control.

### States (PRD)

`attach.place` · `attach.selected_on_connector` · `attach.live_box_drag` · `attach.empty_connector`

### UI-driving fields

`attachment.t`, `attachment.d` — not Designer-invented.
