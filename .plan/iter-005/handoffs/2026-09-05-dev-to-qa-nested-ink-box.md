---
from: dev
to: qa
date: 2026-09-05
iter: iter-005
---

# Hand-off: Dev → QA

## Context

Built nested ink-boxes for [CHL-0032](../challenges/CHL-0032-nested-ink-box.md) after PM adopt + Architect bind. Human skipped `/sm`; slices EP-074 / EP-075 / EP-076.

| Story | Status | Covers |
|---|---|---|
| [STORY-EP-074](../stories/STORY-EP-074.md) | in-review | RenderingContext compose, nested tap-hit, marquee stays top-level |
| [STORY-EP-075](../stories/STORY-EP-075.md) | in-review | Enclose/CTA capture nested boxes; empty flatten; paste flatten |
| [STORY-EP-076](../stories/STORY-EP-076.md) | in-review | Move-commit reparent at 80% natural area |

BDD: [nested-ink-box.feature](../../.docs/modules/epaper/features/ink-box/bdd/nested-ink-box.feature). Host map: `epaper/tests/nested_inkbox_test.cpp`.

## Ask

Verify the five product rules on device (paste-into-box tap-select, empty-letter flatten, nested move/resize own-transform only, marquee top-level, reparent at move end). Then **next `/qa`**.

## Host tests

`nested_inkbox_test`, enclose, surround, clipboard, rendering, manipulate, membership, live overlay: **OK**.

Pre-existing (not this slice): `dispatch_test` / `enclose_shape_test` still assert `kMinEncloseWorld == 28` while `enclose_shape.hpp` is 36/42; `recog_warp_bench` membership vs empty SG without boundary ink.
