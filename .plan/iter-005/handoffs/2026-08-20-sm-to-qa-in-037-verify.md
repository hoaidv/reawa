---
from: sm
to: qa
date: 2026-08-20
iter: iter-005
---

# Hand-off: Scrum Master → Quality Assurance Engineer — verify STORY-IN-037

Developer set [STORY-IN-037](../stories/STORY-IN-037.md) to **in-review**. Feature: [viewport-follow-infini.feature](../../../.docs/modules/infini/features/tablet-sync/bdd/viewport-follow-infini.feature). Tests: `infini/tests/viewport-follow-infini.test.ts`. Run `cd infini && npm test`.

Spot-check vs [UI-IN-04](../design/viewport-follow-infini/ui-spec.md). Do not require STORY-IN-033 continuous tablet-pan apply.

Pass → `done`. Fail → next free DEF + `blocked`.

Handoff: `.plan/iter-005/handoffs/2026-08-20-qa-to-sm-in-037-verify.md`
