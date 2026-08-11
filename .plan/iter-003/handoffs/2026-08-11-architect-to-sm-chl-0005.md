---
from: architect
to: sm
date: 2026-08-11
re: CHL-0005 tablet fixedInk resize ghost
verdict: READY-WITH-CONCERNS
---

# Handoff — Architect → SM

## State

Human re-verify after CHL-0004: **desktop + tablet commit OK** (content fixed, box scales). **Tablet during-resize ghost wrongly scales content** under default `fixedInk`.

## Diagnosis

Not a commit/authority bug. `paintInkGhost` uniformly scales all AABB-overlapping paths; snapshot/pickables lack `role` + `inkScaleMode`, so the stub cannot follow SRS-IN-11.

## Actions taken

| Artifact | Change |
|---|---|
| `CHL-0005` | Open — proposed Adopt |
| SRS-EP-04 | Resize ghost mode-correct rules (forbidden uniform scale under fixedInk) |
| SRS-EP-06 | Quality scenario: fixedInk resize ghost |
| SRS-IN-13 | Pickables `inkScaleMode` + `members[{id,role}]` |
| `viewport-sync.md` | Protocol example updated |

## Review verdict: **READY-WITH-CONCERNS**

| | Finding |
|---|---|
| ✅ | Commit path / Infini flatten already correct (human confirmed) |
| ✅ | Spec gap closed with measurable ghost scenario |
| ⚠️ | Needs PM Adopt before Dev; stories likely EP-010 (ghost) + IN-025 (pickable enrich) |
| ⚠️ | Until members ship, device may show bounds-only ghost as safe fallback |

## Suggested stories (after Adopt)

1. **IN-*** — `buildPickables` emit `inkScaleMode` + `members` from SmartGroup children ids/roles.
2. **EP-*** — `paintInkGhost` resize: fixedInk content translate-only (UV via centroid÷origin AABB); boundary scale; move unchanged.

## Next

PM triage CHL-0005 → **`/pm`**. After Adopt → SM stories → Dev.
