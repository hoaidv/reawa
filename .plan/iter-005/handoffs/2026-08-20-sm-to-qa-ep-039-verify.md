---
from: sm
to: qa
date: 2026-08-20
iter: iter-005
cc: [dev]
---

# Hand-off: Scrum Master → Quality Assurance Engineer — verify STORY-EP-039

Developer set [STORY-EP-039](../stories/STORY-EP-039.md) (Two-finger pan and pinch; publish viewport) to **in-review**. [STORY-EP-038](../stories/STORY-EP-038.md) must stay **done** (do not regress).

Feature: [hand-touch-two-finger.feature](../../../.docs/modules/epaper/features/region-sync/bdd/hand-touch-two-finger.feature). Host: `epaper/tests/hand_touch_test.cpp`. Run `epaper/tests/run_device_document_test.sh`.

Spot-check vs [UI-EP-06](../design/hand-touch/ui-spec.md). Infini apply is **not** this story.

Pass → story `done`. Fail → `DEF-0004` (or next free) + `blocked`.

Handoff: `.plan/iter-005/handoffs/2026-08-20-qa-to-sm-ep-039-verify.md`
