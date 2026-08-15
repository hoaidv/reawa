---
id: STORY-EP-032
title: Architect: device UI chrome state machine
kind: implement
parent_srs: [SRS-EP-12]
parent_req: [REQ-05]
status: draft
priority: P1
iter: iter-004
estimate: 5
owner: unassigned
depends_on: []
acceptance_criteria:
  - "Given growing overlay state (enclose blink, membership highlight, selection chrome, ToolChip latch), When /architect models it, Then an ADR names one explicit chrome/state owner (not ad-hoc flags on TabletCanvasItem)."
  - "Given membership highlight, enclose blink, selection overlay, and live manip, When they overlap, Then the ADR defines precedence, reset events, and which layer paints (CanvasLayer vs ToolCanvasLayer vs QML)."
  - "Given the ADR is accepted, When /dev implements, Then highlight/blink/selection no longer race Pen ingest (no timer on the ink path)."
design_package: ".plan/iter-004/design/recog-blink/"
ui_spec: ".plan/iter-004/design/recog-blink/ui-spec.md"
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-032 — Architect: device UI chrome state machine

**Parked for `/architect` — not NOW.** TRACK-004 still cursors EP-030.

Human 2026-08-15: membership blink lagged Pen. Interim UX is last-join **highlight** ([UI-EP-06](../design/recog-blink/ui-spec.md), [CHL-0020](../challenges/CHL-0020-recog-width-blink.md)). That is a stopgap on `TabletCanvasItem` flags. This story is the proper state/UI-architecture redesign.

Do **not** start until architect ADR exists. Do not steal WIP from EP-030.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` (after ADR) |
| First owner | **architect** |
| Then | `/dev` |
| Status | `draft` until ADR |
