---
from: designer
to: sm
iter: iter-003
date: 2026-08-11
subject: epaper-tool-strip-done
cc: [pm, qa, dev]
verdict: READY-WITH-CONCERNS
---

# Designer → SM — STORY-EP-003 done `[UI-EP-01]`

## Delivered

| Artifact | Path |
|---|---|
| Package | `.plan/iter-003/design/epaper-tool-strip/` |
| Spec | `ui-spec.md` (1-bit · pen-on-strip fallback) |
| Navigator | `index.html` (mobile 80%) |
| Primary | `epaper-tool-strip-pen.html` |
| Fallback | `epaper-tool-strip-touch-unavailable.html` |

Scenes cover SRS-EP-05 matrix: pen · ink_box · selection idle/selected/dragging/empty · session.down · touch.unavailable · states.

EP-005 has Spec/scene links.

## Concerns

- [CHL-0002](../challenges/CHL-0002-epaper-platform-gate.md) — `data-platform=epaper` vs gate allowlist
- On-device touch still unconfirmed — design documents fallback, does not assume finger-only

## Ask

Board: tablet design lane **done**. Continue `/qa`→`/dev` W4 Infini; EP-005 waits on IN-018.
