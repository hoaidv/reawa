---
id: STORY-EP-029
title: Closure-first recognizer dispatch (ADR-0022)
kind: implement
parent_srs: [SRS-EP-10, SRS-EP-17]
parent_req: [REQ-05, REQ-09]
status: in-review
priority: P0
iter: iter-004
estimate: 5
owner: dev
depends_on: [STORY-EP-028]
acceptance_criteria:
  - "Given pen pen-up, When both recognizers are armed, Then exactly one verdict: enclose | membership | connector | ink, logged as one [recog] line (ADR-0022)."
  - "Given a closed-ish stroke that fails enclose guards inside an existing box, When pen-up runs, Then draw-into membership may run (D21) — 0 dual verdicts."
  - "Given EP-016 enclose and EP-017 membership fixtures, When replayed, Then 0 changed verdicts except the deliberate D21 fall-through (EXP-0002 G4)."
  - "Given ink latency, When dispatch runs, Then REQ-01 p95 ≤30 ms is unchanged."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-029 — Closure-first recognizer dispatch (ADR-0022)

Implements [ADR-0022](../../.docs/adr/ADR-0022-recognizer-dispatch.md) and the revised
[SRS-EP-10](../../.docs/modules/epaper/features/ink-box/srs-logic.md) table.
Needs the toggle latch from [STORY-EP-028](./STORY-EP-028.md). Dispatch is `epaper/document/recognizer_dispatch.hpp`. Connector commit remains [STORY-EP-030](./STORY-EP-030.md) (`guard=connector_pending`).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-028 |
