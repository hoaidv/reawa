---
from: architect
to: sm
date: 2026-09-05
iter: iter-005
---

# Hand-off: Architect → Scrum Master

## Context

Bound [CHL-0032](../challenges/CHL-0032-nested-ink-box.md) nested ink-boxes.

| ID | Title |
|---|---|
| [SRS-EP-75](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-75-nested-membership) | Nested membership, flatten, enclose capture |
| [SRS-EP-76](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-76-nested-render) | RenderingContext compose |
| [SRS-EP-77](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-77-nested-hit-reparent) | Nested tap-hit + move reparent |
| [ADR-0039](../../../.docs/adr/ADR-0039-nested-ink-box-rendering.md) | Nested RenderingContext (accepted) |

SRS-EP-10 / SRS-EP-11 / SRS-EP-12 / SRS-EP-14 / SRS-EP-31 / domain SmartGroup / glossary updated in place. `smartgroup_in_selection` retired.

Human asked `/dev` next (skip sequential SM ping). Slice suggestion below.

## Review verdict

**READY-WITH-CONCERNS**

| Class | Finding |
|---|---|
| Strength | Compose-at-paint keeps own-transform; flatten is one gesture; marquee UX unchanged |
| Strength | Affine is 6-float, O(depth); status-quo `walkFlat(..., &node)` named as the defect |
| Concern | Infini mirror must compose the same affine or nested SVG/files drift — add a shared fixture in the implement story |
| Concern | `CreateSmartGroupEdit` today `detachInk` only — must `detachAny` or nested capture silently drops boxes |
| Risk | none blocking slice |

## Asks

1. Slice **three** implement stories (no design story — needs_design: no):
   - EP-074 — RenderingContext + nested tap-hit (SRS-EP-76 + SRS-EP-77 tap) — unblocks the paste defect
   - EP-075 — Enclose/Enclose-CTA capture + flatten (SRS-EP-75)
   - EP-076 — Move-commit reparent 80% (SRS-EP-77 Rule 5)
2. `/qa` BDD on those three before or with implement (human-is-QA still fine).
3. `/dev` EP-074 first.

## Constraints

- Do not recurse `collectPickable` for marquee.
- Do not flatten top-level empty boxes until they are captured.
- Rotation UI still out.

## Out of scope

CHL-0012 sizing; REQ-08 enter/exit group; Infini authoring; EP-070…072.
