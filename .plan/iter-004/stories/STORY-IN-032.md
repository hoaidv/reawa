---
id: STORY-IN-032
title: WorldLayer is device mirror only — no demo mix
kind: implement
parent_srs: [SRS-IN-07, SRS-IN-09]
parent_req: [REQ-03]
status: done
priority: P0
iter: iter-004
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given Infini starts or Cmd+R, When the electron session canvas loads, Then WorldLayer does not paint demoPrimitives (line, circle, rect, zigzag)."
  - "Given RM ink / boxes / connectors in the mirror, When paintMirror runs, Then demo figures are never re-injected."
  - "Given the user drags a Smart Group on epaper, When live samples move (throttled ~5 Hz), Then Infini WorldLayer moves that box and re-derives bound connectors without a new doc_change seq (manip_preview)."
  - "Given pen-up commit set_smart_transform, When Infini applies the doc_change, Then the mirror matches the committed pose (SRS-IN-09) and emits 0 connector ops."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-032 — WorldLayer is device mirror only + live IN-030 drag

Human 2026-08-16: demo figures on Cmd+R; dragging nodes on epaper did not update Infini boxes/connectors.

IN-030 host BDD stays `done`. This story is the live WorldLayer gap.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Priority | **P0** — **done** (human verified 2026-08-16 with IN-030 live drag) |

## Out of scope

EP-034 / EP-036 (HOLD soak). Desktop connector authoring.
