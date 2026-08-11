---
iter: iter-003
goal: "Ink-box pilot — Infini REQ-04 + Epaper REQ-03 tool modes"
start: 2026-08-11
end: 2026-08-25
capacity: 15
committed_points: 47
status: active
---

# Iter 003 — Smart Group / ink-box pilot

Pilot shipped; human verify **failed** 2026-08-11. **NOW:** `/dev` EP-006 ∥ IN-019 → re-verify checklist.

## Committed

### W3a ∥ W3b — prerequisites (ready)

- [STORY-IN-012](./stories/STORY-IN-012.md) — implement — 5 pts — **ready** — tree-backed ink ingestion
- [STORY-EP-004](./stories/STORY-EP-004.md) — implement — 2 pts — **ready** — RM2 touch spike

### W3c — design Infini (ready)

- [STORY-IN-013](./stories/STORY-IN-013.md) — design — 5 pts — **ready** — ink-box-ui package

### W4 — Infini implement (draft until deps)

- [STORY-IN-014](./stories/STORY-IN-014.md) — implement — 3 pts — **ready** — undo (depends IN-012)
- [STORY-IN-015](./stories/STORY-IN-015.md) — implement — 5 pts — **draft** — selection + fixedInk UV
- [STORY-IN-010](./stories/STORY-IN-010.md) — implement — 5 pts — **draft** — tool-armed enclose (rewritten)
- [STORY-IN-016](./stories/STORY-IN-016.md) — implement — 3 pts — **draft** — draw-into membership
- [STORY-IN-017](./stories/STORY-IN-017.md) — implement — 3 pts — **draft** — selection surround create

### W5–W6 — epaper + transport

- [STORY-EP-003](./stories/STORY-EP-003.md) — design — 3 pts — **draft** — blocked on EP-004 spike
- [STORY-IN-018](./stories/STORY-IN-018.md) — implement — 5 pts — **draft** — tool intent transport
- [STORY-EP-005](./stories/STORY-EP-005.md) — implement — 5 pts — **done** — device tool modes

### W7 — verify-fix expedite (ready)

- [STORY-EP-006](./stories/STORY-EP-006.md) — implement — 3 pts — **ready** — ToolChip capacitive touch
- [STORY-IN-019](./stories/STORY-IN-019.md) — implement — 2 pts — **ready** — RM connection eager sync

## Risks

- Epaper design blocked on RM2 touch spike
- Three code prerequisites (ingestion, selection, undo) — order matters
- IN-010 AC rewritten; old propose/accept BDD must be replaced

## Links

- [REQ-04](../../.docs/modules/infini/prd.md#smart-group) · [REQ-03](../../.docs/modules/epaper/prd.md#tool-modes)
- [ADR-0011](../../.docs/adr/ADR-0011-smart-group.md) · [ADR-0013](../../.docs/adr/ADR-0013-ink-box-tool-modes.md)
- [execution-board](./execution-board.md)
