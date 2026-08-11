---
from: architect
to: sm
iter: iter-003
date: 2026-08-11
subject: verify-bugs-enclose-selection-chrome
cc: [dev, qa, pm]
verdict: READY-WITH-CONCERNS
---

# Architect → SM — Enclose sync + tablet selection chrome

## Verdict

**READY-WITH-CONCERNS** — Dev implemented hotfixes aligned with existing SRS-EP-04 / SRS-IN-10.
Product docs updated for selection ghost + tool independence. No new ADR (chrome placement already covered by ADR-0013).

## Findings

1. **Enclose disappear** — root cause: `doc_snapshot` pushed **before** `rebuildWithRmInk`, so tablet received stale free-ink nodes while Infini tree already held the Smart Group. Fixed: rebuild → push. Also converted enclose geometry to **local bounds + world transform** (same model as selection/resize).
2. **Intermittent missing live stroke** — dropped `stroke_begin` ignored all points; live paint only every 3rd sample. Fixed: synthesize stroke on first point; rebuild every sample.
3. **Tool independence** — confirmed device-local; documented in SRS-EP-04. Peer tool must not gate enclose.
4. **Selection chrome** — SRS already required local ghost; implementation was missing. Epaper now composites bounds + 8 handles + dashed move ghost at ≥20 Hz dirty-rect (no full white clear).

## Concerns

- Surround-create path still uses world-space bounds + identity transform (pre-existing); migrate when next touch that path.
- Tablet resize handles are visual-only for now (move/select ship); resize still Infini-primary.

## Ask SM

- Slice/close stories for this hotfix wave if not already tracked (IN-023 enclose snapshot order, EP-008 selection chrome).
- Human re-verify enclose E2E + tablet select/move ghost before iter close.
