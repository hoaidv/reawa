---
id: STORY-EP-005
title: "Epaper device tool modes and intent emission"
kind: implement
parent_srs: [SRS-EP-04, SRS-EP-06]
parent_req: [REQ-03]
status: draft
priority: P1
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-EP-003, STORY-EP-004, STORY-IN-018]
acceptance_criteria:
  - "Given the designed three-tool strip, When Selection/Pen/Ink-box arm, Then input routing matches SRS-EP-04 (enclose intent on Ink-box strokes; pick+tool_intent on Selection)."
  - "Given tool switches, When timed, Then mode change stays within SRS-EP-06 latency budgets and ink latency does not regress past the quality floor."
  - "Given Infini pickables, When Selection drag completes, Then tool_intent is emitted and local ghost is discarded on ack/apply."
design_package: ".plan/iter-003/design/epaper-tool-strip/"
ui_spec: ".plan/iter-003/design/epaper-tool-strip/ui-spec.md"
scenes:
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-pen.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-ink-box.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-selection-selected.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-touch-unavailable.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-orient-gut-on-top.html"
hifi: ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-pen.html"
wireframe: ""
---

# STORY-EP-005 — Epaper device tool modes

Implements [SRS-EP-04](../../../.docs/modules/epaper/features/tool-modes/srs-logic.md)
+ quality [SRS-EP-06](../../../.docs/modules/epaper/features/tool-modes/srs-quality.md).
UI from [STORY-EP-003](./STORY-EP-003.md) — Spec [UI-EP-01](../design/epaper-tool-strip/ui-spec.md).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-003 (design), EP-004 (spike), IN-018 |

## Done when

- AC green `@SRS-EP-04` / `@SRS-EP-06`; Spec paths copied after design done
