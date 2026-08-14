---
feature: connector-ink
parent_req: [REQ-09]
version: 0.1.0
lifecycle: active
owner: pm
needs_design: true
---

# SRS — Connector-ink (Experience)

Journeys for [REQ-09](../../prd.md#device-connectors). Chrome contract:
[SRS-EP-19](./srs-ui.md). Logic: [SRS-EP-17](./srs-logic.md) / [SRS-EP-18](./srs-logic.md).

## Journey: `journey.connector_ux1` — Single polyline

| # | Step | State id | Notes |
|---|---|---|---|
| 1 | Two ink-boxes exist; Pen + Connector recognition armed | `tool.pen` + `recog.connector.on` | Default launch |
| 2 | Draws an open line from near A into C | `ink.live` | Same ink path as REQ-01 |
| 3 | Pen-up; device recognizes a connector | `conn.created` | ≤500 ms; 0 peer messages |
| 4 | Connector + A + C blink once | `conn.blink` | Does not name Ink/Curve |
| 5 | One undo restores the three original inks | `conn.reverted` | One entry |

## Journey: `journey.connector_ux2` — Chain

| # | Step | State id | Notes |
|---|---|---|---|
| 1 | Draws X1 from A into void | ordinary ink | No pending connector node |
| 2 | Draws X2 continuing X1 | ordinary ink | Chrome-only free-end tick allowed |
| 3 | Draws X3 onto C | `conn.created` | Merge in one op; style from merged spine |

## Journey: `journey.connector_ux3` — Move a bound box

| # | Step | State id | Notes |
|---|---|---|---|
| 1 | Drags box A | `conn.live_warp` | ≥5 Hz; partial refresh; ToolCanvasLayer |
| 2 | Pen-up | `conn.settled` | Committed = last previewed |

## Journey: `journey.connector_select` — Change style / end

| # | Step | State id | Notes |
|---|---|---|---|
| 1 | Selects the connector | `conn.selected` | Ink/Curve control; per-end Edge/Centre |
| 2 | Taps Curve | `conn.style.curve` | Rest shape unchanged; one undo |

## Journey: `journey.connector.alt_stays_ink` — Guards fail

| # | Step | State id | Notes |
|---|---|---|---|
| 1 | Draws a line that misses a second box | `ink.live` | |
| 2 | Pen-up: stays ordinary ink | `conn.rejected` | No banner |

## Dual-ask states

| Journey step | State id | Designer | QA |
|---|---|---|---|
| UX1 4 | `conn.blink` | required | required |
| select 1 | `conn.selected` | required | required |
| alt 2 | `conn.rejected` | required (no extra chrome) | required |
| UX3 1 | `conn.live_warp` | required | required |
