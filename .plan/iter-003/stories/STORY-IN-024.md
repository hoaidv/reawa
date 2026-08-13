---
id: STORY-IN-024
title: Ink-scale mode toggle on desktop
kind: implement
parent_srs: [SRS-IN-11]
parent_req: [REQ-04]
status: blocked
blocked_reason: "CHL-0008 adopted 2026-08-13 — desktop ink-box authoring deprecated (infini REQ-04). Not scheduled; SM re-slices."
priority: P2
iter: iter-003
estimate: 2
owner: dev
depends_on: []
acceptance_criteria:
  - "Given a selected Smart Group, when the user taps Fixed ink / Scale ink, then inkScaleMode toggles between fixedInk and withBounds"
  - "Given a mode toggle, when applied, then doc snapshot is pushed to RM and undo records set_ink_scale_mode"
---

# STORY-IN-024 — Ink-scale mode toggle on desktop

[SRS-IN-11](../../../.docs/modules/infini/features/vector-document/srs-logic.md) · CHL-0004

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner persona | `dev` |

## Done when (implement)

- ToolStrip shows `tgl.ink_scale_mode` when a Smart Group is selected
- Toggle applies `set_ink_scale_mode` + rebuild + RM snapshot
