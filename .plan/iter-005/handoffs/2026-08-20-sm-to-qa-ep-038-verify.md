---
from: sm
to: qa
date: 2026-08-20
iter: iter-005
cc: [dev, pm]
---

# Hand-off: Scrum Master → Quality Assurance Engineer — verify STORY-EP-038

Developer set [STORY-EP-038](../stories/STORY-EP-038.md) (One-finger hit box: select freeform and move) to **in-review**.

## Verify

Load `verify-story`. Feature: [hand-touch-one-finger.feature](../../../.docs/modules/epaper/features/ink-box/bdd/hand-touch-one-finger.feature). Host mapping: `epaper/tests/hand_touch_test.cpp`. Run `epaper/tests/run_device_document_test.sh`.

UI spot-check vs [UI-EP-06](../design/hand-touch/ui-spec.md) — regions/components, not pixels. Do not edit design HTML.

## Pass

Set story `status: done`. Handoff `.plan/iter-005/handoffs/2026-08-20-qa-to-sm-ep-038-verify.md`.

## Fail

File `.plan/iter-005/defects/DEF-*.md`, set story `blocked`, do not mark done.

Do not start EP-039. Do not edit `src/` except if you only run tests. Do not rollup.
