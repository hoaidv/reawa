---
from: architect
to: sm
iter: iter-002
date: 2026-08-11
subject: smart-group-adr0011
verdict: READY-WITH-CONCERNS
---

# Handoff: Architect → SM — Smart Group pilot specified

## Verdict: **READY-WITH-CONCERNS**

| Artifact | |
|---|---|
| [ADR-0011](../../../.docs/adr/ADR-0011-smart-group.md) | **accepted** — SmartGroup + local TF + inkScaleMode |
| [SRS-IN-10](../../../.docs/modules/infini/features/vector-document/srs-logic.md) | Enclose recognition rules |
| Tree / data / quality | SmartGroup kind + ops + metrics |
| [REQ-04](../../../.docs/modules/infini/prd.md#smart-group) | Could pilot |

## Concerns

1. **Scheduling:** vertical `wip: 1` — do **not** open a second feature track. Pilot stories
   belong under TRACK-002 / vector-document **after** IN-006 design (or as implement slices
   of F2), Priority Could.
2. **Local transforms** only on SmartGroup; ordinary Group still world-space (ADR-0010).
3. Enclose recognition is best-effort — budget explicit-create UX in design.

## Ask of SM

- Keep STORY-IN-006 on DocChrome as NOW.
- Optionally add a **draft** design/implement story for Smart Group handles + enclose affordance
  (`depends_on` tree model stories), status draft until F2 chrome clears — or backlog until
  Must F2 verified.

## Next

**`/sm`** (replan pilot) or **`/designer`** (continue IN-006; annotate Smart Group as future
populated scene if easy).
