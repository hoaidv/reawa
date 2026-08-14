---
from: architect
to: sm
date: 2026-08-14
iter: iter-004
---

# Hand-off: Architect → SM — connectors are sliceable

## Context

PM [handoff](./2026-08-14-pm-to-architect.md) asked for ADRs + SRS. All are in.

### Decisions

| ADR | What it settles |
|---|---|
| [ADR-0020](../../../.docs/adr/ADR-0020-connector-ink-geometry.md) | Rest shape `(s,d)`; stored styles **Ink = morph**, **Curve = cubic**; auto-pick from rest spine (≤1 inflection → Curve); live re-warp; delete-keeps; amends ADR-0010 |
| [ADR-0021](../../../.docs/adr/ADR-0021-connector-toolchip.md) | ToolChip **3 exclusive tools + 2 recognizer toggles + Undo/Redo**; **supersedes ADR-0017** |
| [ADR-0022](../../../.docs/adr/ADR-0022-recognizer-dispatch.md) | Closure-first, one verdict per pen-up; failed enclose may fall through to membership |

### New / amended SRS

| Id | File | Covers |
|---|---|---|
| SRS-EP-17 | [connector-ink/srs-logic](../../../.docs/modules/epaper/features/connector-ink/srs-logic.md) | UX1/UX2 recognition |
| SRS-EP-18 | same | Ink/Curve warp + live drag |
| SRS-EP-19 | [connector-ink/srs-ui](../../../.docs/modules/epaper/features/connector-ink/srs-ui.md) | blink + selection chrome |
| SRS-EP-20 | [connector-ink/srs-quality](../../../.docs/modules/epaper/features/connector-ink/srs-quality.md) | ship gate / warp bars |
| SRS-EP-04 / 05 | tool-modes | 3+2 inventory; `tool.ink_box` retired |
| SRS-EP-10 | ink-box logic | dispatch table + D21 fall-through |
| SRS-IN-09 | infini vector-document data | `create_connector` envelope |

## Review verdict

**READY-WITH-CONCERNS**

| Class | Finding |
|---|---|
| Strength | Warp is a pure function — mirror stays 0 extra ops (ASR-1) |
| Strength | Closed ToolChip inventory is paint-ready (SRS-EP-05 v0.3.0) |
| Concern | Blink waveform / inflection cutoff / RM2 live-drag Hz are designer/QA follow-ups, not missing ADRs |
| Concern | Guard corpus still open — SM must **not** treat it as a lock blocker; it is a ship gate |
| Risk | D21 fall-through can regress EP-016/017 — slice a replay AC on EP-029 |

## Asks

1. Open TRACK-004; lock **horizontal · design-validated**.
2. Design stories first: ToolChip (REQ-03 / SRS-EP-05) and connector chrome (REQ-09 / SRS-EP-19).
3. Slice implement stories with `depends_on` those design ids; keep them `draft`.
4. Infini is viewer + persistence only this campaign.

## Constraints

- Do not copy the EXP-0002 worktree into production stories as "port this file".
- Do not schedule REQ-08.

## Out of scope

Same as PM handoff.
