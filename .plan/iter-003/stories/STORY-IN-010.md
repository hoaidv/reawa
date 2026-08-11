---
id: STORY-IN-010
title: "Tool-armed enclose recognition (immediate Smart Group)"
kind: implement
parent_srs: [SRS-IN-10]
parent_req: [REQ-04]
status: done
priority: P0
iter: iter-003
estimate: 5
owner: dev
depends_on: [STORY-IN-012, STORY-IN-013, STORY-IN-014, STORY-IN-015]
acceptance_criteria:
  - "Given a stroke with intent enclose that fits an axis-aligned rect with shorter side ≥ 48 world units and ≥1 ink ≥80% inside, When stroke_end runs on Infini, Then create_smart_group commits immediately (no propose/accept): enclose → role boundary, content reparented, bounds fitted, each content layoutOffset UV seeded."
  - "Given a non-enclose stroke (intent ink/absent), When stroke_end runs, Then enclose recognition does not run."
  - "Given guards fail (too small / no content / already grouped only), When enclose ends, Then the stroke stays ordinary ink (no-op, no banner)."
  - "Given a successful create, When undo runs, Then the pre-op snapshot is restored."
design_package: ".plan/iter-003/design/ink-box-ui/"
ui_spec: ".plan/iter-003/design/ink-box-ui/ui-spec.md"
scenes:
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-ink-box-armed.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-selection-selected.html"
hifi: ".plan/iter-003/design/ink-box-ui/ink-box-ui-ink-box-armed.html"
wireframe: ""
---

# STORY-IN-010 — Tool-armed enclose recognition

Implements revised [SRS-IN-10](../../../.docs/modules/infini/features/vector-document/srs-logic.md#srs-in-10-enclose-recognition-smart-group-pilot)
per ADR-0013. **Rewritten 2026-08-11** — propose/accept AC withdrawn.
UI arming chrome: [STORY-IN-013](./STORY-IN-013.md) / [UI-IN-02](../design/ink-box-ui/ui-spec.md).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | IN-012, IN-013, IN-014, IN-015 |

## Done when

- AC green `@SRS-IN-10`
- Old propose/accept BDD scenarios removed or replaced
