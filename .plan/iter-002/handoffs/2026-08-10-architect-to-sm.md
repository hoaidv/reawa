---
from: architect
to: sm
iter: iter-002
date: 2026-08-10
subject: design-ready-infini
---

# Handoff: Architect → SM — Infini design baseline

## Review verdict — **READY-WITH-CONCERNS**

| Area | Finding |
|---|---|
| ✅ | ADR-0008 Electron+React accepted (velocity + cross-platform; spike gestures) |
| ✅ | ADR-0009 op-log document + viewport channel — answers “same picture / laggy refresh” |
| ✅ | Module architecture + SRS-IN-01..08, SRS-EP-02..03 drafted |
| ⚠️ | SVG element subset not frozen — spike fixtures next |
| ⚠️ | Wire framing still EXP JSON-lines baseline until schema pass |

## Electron + React — recommendation

**Agree** with the human’s effort rationale. For Infini’s job (2D canvas + net session),
Electron+React is the right default: one UI stack, three desktops, fast iteration.
Accept the memory/weight cost; **mitigate** with an early pan/pinch frame-budget spike
before betting on a renderer library. Reawa stays Swift; do not merge.

## Consistency mechanism (short)

Do **not** dual-edit two independent DOMs. Keep one **append-only vector op-log** both
peers apply, and a separate **viewport channel** Infini owns. Epaper applies viewport to
the **input map immediately**, refreshes pixels when it can. Same document ∩ same region
⇒ same paint; ghosting ≠ divergence. See ADR-0009.

## Next

`/sm` — commit design stories for infinity-canvas + vector-document UI; implement stories
for F1 only under vertical WIP; then `/designer` → `/qa` → `/dev`.
