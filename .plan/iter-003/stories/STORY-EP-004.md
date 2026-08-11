---
id: STORY-EP-004
title: "RM2 capacitive touch reachability spike"
kind: implement
parent_srs: [SRS-EP-04]
parent_req: [REQ-03]
status: done
priority: P0
iter: iter-003
estimate: 2
owner: dev
depends_on: []
acceptance_criteria:
  - "Given the Qt epaper app on RM2 (or equivalent harness), When the spike runs, Then it records whether capacitive touch events are reachable from the app (yes/no + API path)."
  - "Given touch unreachable, When the spike closes, Then a written fallback recommendation exists (pen-on-strip vs hardware button) for designer/PM — no silent assumption."
  - "Given the spike result, When handed to SM/designer, Then STORY-EP-003 may leave draft."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-004 — RM2 capacitive touch reachability spike

**In review.** Probe in `epaper/tabletappfilter.cpp`; write-up
[`.plan/iter-003/memory/ep-004-rm2-touch-spike.md`](../memory/ep-004-rm2-touch-spike.md).
On-device yes/no still needs a finger tap on RM2; fallback recommendation is recorded so EP-003 can design.
