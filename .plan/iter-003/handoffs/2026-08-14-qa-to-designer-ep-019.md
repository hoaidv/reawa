---
from: qa
to: designer
date: 2026-08-14
iter: iter-003
---

# Hand-off: QA → Designer — EP-019 BDD walk

## Verdict

**READY-FOR-DESIGN then DEV.** Coverage for [STORY-EP-019](../stories/STORY-EP-019.md) is in
[smart-group-selection.feature](../../../.docs/modules/epaper/features/ink-box/bdd/smart-group-selection.feature).

## Walk vs AC

| AC | Scenario |
|---|---|
| Hit-select p95 ≤100 ms, both tools | Hit selects … with `sel_rect` / `sel_freeform` |
| Four-tool chip during manipulation | Manipulation chrome uses the four-tool chip |
| Move ≥5 Hz, 1 op, 0 ghost | Drag moves the real ink… + partial-refresh budget |
| fixedInk / withBounds | existing resize scenarios |
| No negative size | Inverted resize… |
| Deselect 0 residue | Deselect leaves no residue |
| LOD unavailable | Below the LOD cutoff… |
| Capability descriptor | Manipulation dispatches through the capability descriptor |
| Shared fixtures | (quality SRS-EP-14 — keep on existing fixture scenarios / host tests) |

## Asks of designer

Rebase UI-EP-02 ToolChip onto ADR-0017 (`sel_rect` · `sel_freeform` · pen · ink_box). Do not add a third selection mode.

## Next

`/designer` EP-023, then `/dev` EP-019.
