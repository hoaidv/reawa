---
from: dev
to: qa
date: 2026-08-14
story: STORY-EP-017
---

# Hand-off: Dev → QA — EP-017 draw-into local (x,y)

## What

Hotfix on done [STORY-EP-017](../stories/STORY-EP-017.md): new Pen ink joining a **resized** `fixedInk` box stored local samples as `(world − translate) / scale`. Device paint is `world = local + translate` (no scale / top-left). After re-render the stroke jumped inward (looked centered).

Join now stores `local = world − parent.translate` for `fixedInk`, matching paint. `withBounds` still divides by scale. Infini `join_smart_group` matches.

## Host

`epaper/tests/run_device_document_test.sh` PASS (new `test_fixed_ink_join_ignores_parent_scale`). Infini `draw-into-membership.test.ts` PASS.

## On-panel

1. Enclose a box, switch to `fixedInk` if needed, resize the box (content stays size).
2. Pen-draw inside, especially away from the top-left.
3. Pen-up then any re-render (select / second stroke): new ink stays where drawn.

## EP-025

Unchanged this turn — still `in-review`.
