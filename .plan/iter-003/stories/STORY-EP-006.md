---
id: STORY-EP-006
title: "ToolChip capacitive touch routing"
kind: implement
parent_srs: [SRS-EP-04]
parent_req: [REQ-03]
status: done
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-EP-004, STORY-EP-005]
acceptance_criteria:
  - "Given RM2 finger touch on a ToolChip tile (Selection/Pen/Ink-box), When TouchBegin/End completes inside that tile, Then toolMode arms without starting ink on the canvas."
  - "Given finger touch on the ink canvas (outside ToolChip bounds), When the touch completes, Then no tool switch and no ink (SRS non-goal for finger pan unchanged)."
  - "Given pen press on a ToolChip tile, When pen-on-chip fallback is active, Then the tool still arms (regression guard for STORY-EP-005)."
  - "Given EP-004 spike path (QTouchEvent via TabletAppFilter), When wired, Then touch hit-test uses the same toolChipRect as pen exclusion."
design_package: ".plan/iter-003/design/epaper-tool-strip/"
ui_spec: ".plan/iter-003/design/epaper-tool-strip/ui-spec.md"
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-006 — ToolChip capacitive touch routing

**Human verify fail (2026-08-11):** toolbar not tappable on RM2. EP-004 proved `QTouchEvent`
reaches `TabletAppFilter`; EP-005 wired `MouseArea` only — finger never reaches `armTool`.

Fix: route touch (or synthesize to QML) using `toolChipRect` + tile hit-test. No design change.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-004 (spike), EP-005 (baseline chip) |

## Done when

- AC green `@SRS-EP-04`; human can switch tools with finger on device
