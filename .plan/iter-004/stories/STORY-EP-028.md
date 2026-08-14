---
id: STORY-EP-028
title: ToolChip inventory: 3 exclusive tools + 2 recognizer toggles
kind: implement
parent_srs: [SRS-EP-04, SRS-EP-05]
parent_req: [REQ-03]
status: draft
priority: P0
iter: iter-004
estimate: 3
owner: dev
depends_on: [STORY-EP-026]
acceptance_criteria:
  - "Given launch, When the chip is shown, Then exclusive tools are exactly sel_rect, sel_freeform, pen; recog.ink_box and recog.connector are independent toggles default on; Undo/Redo remain actions (ADR-0021 / ADR-0018)."
  - "Given a Selection tool, When active, Then both toggles are dimmed and keep armed state; switching to pen restores them."
  - "Given a toggle flip mid-stroke, When pen-up runs, Then dispatch uses the latched pen-down tuple (D15)."
  - "Given Pen, When inking, Then REQ-01 p95 ≤30 ms is unchanged and the chip exclusion rect still eats no ink."
design_package: ".plan/iter-004/design/toolchip-recognizers/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-028 — ToolChip inventory: 3 exclusive tools + 2 recognizer toggles

Implements [SRS-EP-04](../../.docs/modules/epaper/features/tool-modes/srs-logic.md) /
[SRS-EP-05](../../.docs/modules/epaper/features/tool-modes/srs-ui.md).
**Blocked on** [STORY-EP-026](./STORY-EP-026.md) (`depends_on`; do not set `ready` until that design story is `done`).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-026 |
