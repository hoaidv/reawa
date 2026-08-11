---
id: STORY-IN-010
title: "Smart Group enclose recognition pilot"
kind: implement
parent_srs: [SRS-IN-10]
parent_req: [REQ-04]
status: draft
priority: P2
iter: iter-003
estimate: 3
owner: dev
depends_on: [STORY-IN-007, STORY-IN-009]
acceptance_criteria:
  - "Given a near-closed roughly 4-sided stroke enclosing ≥80% of samples of ≥1 ink, When recognize_enclose runs on Infini, Then a transient SmartGroup preview is proposed."
  - "Given user accept, When create_smart_group applies, Then enclose stroke is role:boundary, content ink reparented, geometric bounds set, and the op is undoable."
  - "Given user reject or undo, When applied, Then the prior tree is restored and ink is untouched."
  - "Given Epaper v0, When enclose runs, Then recognition is Infini-first only (Epaper keeps emitting raw append_ink)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-010 — Smart Group enclose recognition pilot

Implements [SRS-IN-10](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-10-enclose-recognition-smart-group-pilot)
per [ADR-0011](../../../.docs/adr/ADR-0011-smart-group.md) (**Could** / pilot).

**Carried from iter-002.** Stays `draft` until human briefs **`/pm`** with Smart Group
requirements and a `kind: design` story exists (REQ-04 Needs design: yes) — or PM
explicitly waives design for a library-only slice.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-IN-007, STORY-IN-009 (done in iter-002) |

## Done when

- AC green under BDD `@SRS-IN-10`
- Design dependency satisfied or waived in writing by PM
