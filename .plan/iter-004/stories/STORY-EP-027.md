---
id: STORY-EP-027
title: Design connector blink and Ink/Curve selection chrome
kind: design
parent_srs: [SRS-EP-19]
parent_req: [REQ-09]
status: done
priority: P0
iter: iter-004
estimate: 3
owner: designer
depends_on: []
acceptance_criteria:
  - "Given a connector is created, When chrome runs, Then ovl.conn_blink flashes the connector and both bound nodes once — no style name, no badge, no full-panel refresh (SRS-EP-19)."
  - "Given a selected connector, When shown, Then tgl.conn_style is Ink|Curve and each end has Edge|Centre; empty canvas deselects with 0 residual chrome."
  - "Given conn.rejected, When the stroke stays ink, Then no extra chrome (no banner)."
design_package: ".plan/iter-004/design/connector-chrome/"
ui_spec: ".plan/iter-004/design/connector-chrome/ui-spec.md"
scenes:
  - ".plan/iter-004/design/connector-chrome/connector-chrome-blink.html"
  - ".plan/iter-004/design/connector-chrome/connector-chrome-selected.html"
  - ".plan/iter-004/design/connector-chrome/connector-chrome-rejected.html"
  - ".plan/iter-004/design/connector-chrome/connector-chrome-live-warp.html"
  - ".plan/iter-004/design/connector-chrome/connector-chrome-orphan.html"
hifi: ".plan/iter-004/design/connector-chrome/connector-chrome-blink.html"
wireframe: ""
---

# STORY-EP-027 — Design connector blink and Ink/Curve selection chrome

[SRS-EP-19](../../.docs/modules/epaper/features/connector-ink/srs-ui.md) /
[REQ-09](../../.docs/modules/epaper/prd.md#device-connectors).
Journeys: [srs-experience](../../.docs/modules/epaper/features/connector-ink/srs-experience.md).

**Output:** `.plan/iter-004/design/connector-chrome/`

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |

## Done when

- Scenes for `conn.blink`, `conn.selected`, `conn.rejected`, `conn.live_warp`, `conn.orphan`
- Linked [STORY-EP-030](./STORY-EP-030.md) / warp chrome `depends_on` this id
