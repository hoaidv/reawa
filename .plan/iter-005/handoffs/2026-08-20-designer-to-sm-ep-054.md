---
from: designer
to: sm
date: 2026-08-20
iter: iter-005
cc: [pm, qa]
---

# Hand-off: Designer → Scrum Master — STORY-EP-054

## Story

[STORY-EP-054](../stories/STORY-EP-054.md) (Revise hand-touch: palm-rest vs empty local pan) is **done**. Package [hand-touch/](../design/hand-touch/) ([UI-EP-06](../design/hand-touch/ui-spec.md)).

## Gate

`ui-spec-gate` **pass** for this package (9 Keep scenes + states showcase; inlined-scene `index.html`; iframe `src="about:blank"`).

Mechanical `adlc gate` design rows for this package: interactivity, CSS allowlist, tokens/common links, navigator — **pass**. Platform allowlist still rejects `data-platform="epaper"` ([CHL-0002](../../iter-003/challenges/CHL-0002-epaper-platform-gate.md), adopted, engine patch pending) — Spec follows [SRS-EP-22](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#srs-ep-22-hand-touch-ui).

## Package delta

Replaced `hand.one_finger_empty` no-op-only with:

| State | File | Paint |
|---|---|---|
| `hand.one_finger_empty_palm` | `hand-touch-one-finger-empty-palm.html` | Travel **8 mm** (≤ 10 mm / 89 du); world **unshifted**; 0 pan / 0 selection / 0 tool switch |
| `hand.one_finger_empty_pan` | `hand-touch-one-finger-empty-pan.html` | Travel **36 mm** (> 10 mm) + 10 mm tick; **local** world translate; Infini unchanged (`follow=none`) |

Deleted `hand-touch-one-finger-empty.html`. Two-finger pan/pinch status no longer says `source epaper` / Infini match. No follow-toggle. No new product chrome (travel ticks are GestureAnnotate preview only).

## SM stitch (this lane could not edit other stories)

- [STORY-EP-037](../stories/STORY-EP-037.md) `scenes` still lists the deleted empty file → mechanical artifacts FAIL until you replace it with palm + pan.
- [STORY-EP-038](../stories/STORY-EP-038.md) / [STORY-EP-039](../stories/STORY-EP-039.md) still cite the old empty path — copy EP-054 `scenes` / `hifi` / `ui_spec`.

## Next

`/sm` then `/qa` for EP-038 (after BDD). Do not `/dev` until design + BDD and `/init` (`paths.src` empty).
