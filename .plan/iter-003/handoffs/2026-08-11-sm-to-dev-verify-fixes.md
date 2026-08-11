---
from: sm
to: dev
iter: iter-003
date: 2026-08-11
subject: verify-failed-expedite-fixes
cc: [qa, pm, human]
verdict: EXPEDITE
---

# SM → Dev — Human verify failed; expedite fixes

## Human report

1. **Cannot touch toolbar on Epaper** — blocked all tool-mode testing.
2. **Desktop connection status too late** — RM linked before UI caught up.
3. **Smart Group create paths untested** — blocked by (1)+(2); not a separate code defect yet.

TRACK-003 **does not pass** the execution-lock stop line (`verified`).

## Root cause (SM read of code)

| Issue | Likely cause | Story |
|---|---|---|
| Toolbar dead | EP-004: `QTouchEvent` reaches filter; EP-005: `MouseArea` only in `Main.qml` — finger never calls `armTool` | [STORY-EP-006](../stories/STORY-EP-006.md) |
| Late connected UI | Race: Epaper connects before `onRmClient` registered / `rmClientCount` resolves | [STORY-IN-019](../stories/STORY-IN-019.md) |

## Fix stories (P0, ready, parallel)

- **EP-006** — wire capacitive touch → ToolChip tile hit-test → `armTool` (same rect as pen exclusion)
- **IN-019** — eager sync: current count on renderer attach + no missed connect events

**Parallel:** lanes A (epaper) ∥ B (infini) — no file conflict.

## Ask Dev

1. Implement EP-006 + IN-019; `/qa` BDD for IN-019 session channel if scenarios missing.
2. Do **not** change Smart Group logic until human re-verify — paths may work once toolbar + sync land.

## Re-verify checklist (human, after fixes)

| # | Path | Steps | Pass? |
|---|---|---|---|
| 1 | **Enclose create** (Epaper) | Arm Ink-box → draw closed rect around ink → Infini creates Smart Group immediately | |
| 2 | **Surround create** (Infini) | Select ≥2 inks with surround → Create Smart Group → boundary + content roles | |
| 3 | **Connection** | Start Epaper first, then Infini → connected hint within ~1s, ink syncs | |
| 4 | **Tool switch** | Finger tap each ToolChip tile on RM2 → mode changes, status line updates | |

Report pass/fail per row → SM closes verify or slices follow-ups.

## Ask QA

- Extend `@SRS-IN-07` BDD if eager-sync AC needs automated coverage.
- EP-006 is device-primary; document manual RM2 finger tap in verify notes.

## Ask PM

- No new SRS/challenge unless re-verify exposes spec gap.
- Iter close remains blocked until checklist passes.
