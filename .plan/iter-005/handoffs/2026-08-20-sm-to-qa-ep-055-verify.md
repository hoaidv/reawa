---
from: sm
to: qa
date: 2026-08-20
iter: iter-005
---

# Hand-off: Scrum Master → Quality Assurance Engineer — verify STORY-EP-055

Developer set [STORY-EP-055](../stories/STORY-EP-055.md) to **in-review**. Feature: [viewport-follow-epaper.feature](../../../.docs/modules/epaper/features/region-sync/bdd/viewport-follow-epaper.feature). Host: `epaper/tests/viewport_follow_test.cpp`. Also confirm `hand_touch_test` still passes (EP-038/039).

Spot-check vs [UI-EP-07](../design/viewport-follow-epaper/ui-spec.md): FollowToggle sibling of ToolChip, not a fourth exclusive.

Pass → `done`. Fail → next free DEF + `blocked`.

Handoff: `.plan/iter-005/handoffs/2026-08-20-qa-to-sm-ep-055-verify.md`
