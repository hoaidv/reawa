---
id: STORY-EP-026
title: Design ToolChip: 3 tools, 2 recognizer toggles, Undo/Redo
kind: design
parent_srs: [SRS-EP-05]
parent_req: [REQ-03]
status: done
priority: P0
iter: iter-004
estimate: 3
owner: designer
depends_on: []
acceptance_criteria:
  - "Given the primary chip, When shown, Then inventory is sel_rect | sel_freeform | pen, gap, recog.ink_box | recog.connector, gap, cta.undo | cta.redo — 0 ink_box exclusive tool (ADR-0021 / SRS-EP-05)."
  - "Given sel_rect or sel_freeform active, When the chip is shown, Then both recognizer toggles are dimmed and retain armed state."
  - "Given each required state (pen+both armed; a toggle off; selection+dimmed; undo empty no-op), When the package ships, Then there is one scene HTML per state and ui-spec-gate passes."
design_package: ".plan/iter-004/design/toolchip-recognizers/"
ui_spec: ".plan/iter-004/design/toolchip-recognizers/ui-spec.md"
scenes:
  - ".plan/iter-004/design/toolchip-recognizers/toolchip-recognizers-pen-armed.html"
  - ".plan/iter-004/design/toolchip-recognizers/toolchip-recognizers-connector-off.html"
  - ".plan/iter-004/design/toolchip-recognizers/toolchip-recognizers-sel-rect-dimmed.html"
  - ".plan/iter-004/design/toolchip-recognizers/toolchip-recognizers-undo-empty.html"
  - ".plan/iter-004/design/toolchip-recognizers/toolchip-recognizers-recog-rejected.html"
hifi: ".plan/iter-004/design/toolchip-recognizers/toolchip-recognizers-pen-armed.html"
wireframe: ""
---

# STORY-EP-026 — Design ToolChip: 3 tools, 2 recognizer toggles, Undo/Redo

Design package for [REQ-03](../../.docs/modules/epaper/prd.md#tool-modes) /
[SRS-EP-05](../../.docs/modules/epaper/features/tool-modes/srs-ui.md) as amended by
[ADR-0021](../../.docs/adr/ADR-0021-connector-toolchip.md).

Replaces UI-EP-01 four-tool chip. Undo/Redo stay actions ([ADR-0018](../../.docs/adr/ADR-0018-undo-redo-chip-actions.md)).
Enclose stays off this row ([ADR-0016](../../.docs/adr/ADR-0016-selection-create-enclose-cta.md)).

**Output:** `.plan/iter-004/design/toolchip-recognizers/`

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |

## Done when

- `ui-spec-gate` pass; scenes cover armed / disarmed / dimmed / undo-empty
- Linked [STORY-EP-028](./STORY-EP-028.md) `depends_on` this id
