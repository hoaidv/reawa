---
from: sm
to: designer
iter: iter-002
date: 2026-08-11
subject: story-in-006-ready
---

# Handoff: SM → Designer — STORY-IN-006 ready

## Context

- F1 infinity-canvas **verified** (PM gate READY-WITH-CONCERNS).
- TRACK-001 **done**; TRACK-002 **active**; board wave **W3**.
- Vertical wip 1 → **NOW feature:** `infini/vector-document`.

## Pickup

| Story | Status | Package |
|---|---|---|
| [STORY-IN-006](../stories/STORY-IN-006.md) | **ready** | `.plan/iter-002/design/vector-document/` |

## Sources

- [SRS-IN-05](../../../.docs/modules/infini/features/vector-document/srs-ui.md) — **thickened** DocChrome / states
- [srs-experience](../../../.docs/modules/infini/features/vector-document/srs-experience.md) · [srs-product](../../../.docs/modules/infini/features/vector-document/srs-product.md)
- [ADR-0010](../../../.docs/adr/ADR-0010-tree-of-vectors.md) — tree-of-vectors
- [REQ-02](../../../.docs/modules/infini/prd.md#vector-document)
- Companion canvas: `[UI-IN-01]` in `.plan/iter-002/design/infinity-canvas/`

## Note

SRS thicken **READY** (PM+Architect 2026-08-11). Paint chrome + show that populated content is a tree (ink/group/frame/connector/text/primitive), not a flat stroke bag. Full outliner deferred.
## Done-when

Package ships scenes (or annotated states) for all four doc.* states + `ui-spec.md` + desktop platform profile; story → `done` / `in-review` per Designer norms.

## Next

`/designer` on STORY-IN-006. After design `done`, SM slices implement stories → `/qa` BDD → `/dev`.
