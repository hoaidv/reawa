---
id: TRACK-009
slug: document-forest
kind: planned
status: active
iter: iter-006
goal: "Build the document forest (ADR-0041) in the rebuilt epaper/: store, nodes, per-container R-tree, concurrent readers, manipulation, connector, progressive paint and local undo."
scope:
  - epaper/device-document
  - epaper/local-pen-ink
stories:
  - STORY-EP-082
  - STORY-EP-083
  - STORY-EP-084
  - STORY-EP-085
  - STORY-EP-086
  - STORY-EP-087
  - STORY-EP-088
  - STORY-EP-089
  - STORY-EP-090
  - STORY-EP-091
cursor: "STORY-EP-082 (Forest foundations) ready — the human codes; agents guide and review."
paused_reason: ""
interrupts: []
---

# TRACK-009 — Document forest

## Goal

Implement [ADR-0041](../../.docs/adr/ADR-0041-document-forest.md) (Document forest, concurrent
readers, progressive paint) in `epaper/`, following
[document-forest-implementation.md](../document-forest-implementation.md). One story per plan
step; step 3 is split so Milestone A (pen ink painted from the forest) is its own story.

**Mode.** The human writes the code. Agents explain the design, review changes and check the
evidence named in each story. Agents do not edit `epaper/src/`.

## Scope

- In: `epaper/` document core (`epaper/src/doc/`, plain C++17, no Qt), its host tests, and the
  render and pen glue that Milestone A and step 8 need.
- Features: [epaper/device-document](../../.docs/modules/epaper/features/device-document/index.md)
  (forest, queries, undo); [epaper/local-pen-ink](../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md)
  only for STORY-EP-085, where the existing pen path commits into the forest.
- Out: Infini and the sync wire ([CHL-0034](../iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md));
  recognizers and tools; product chrome; using `transform`; deleting `epaper_old/`.

## Stories

| ID | Plan step | Status | Pts | Milestone |
|---|---|---|---|---|
| [STORY-EP-082](../iter-006/stories/STORY-EP-082.md) | 1 Foundations | ready | 3 | A |
| [STORY-EP-083](../iter-006/stories/STORY-EP-083.md) | 2 Slots and handles | draft | 3 | A |
| [STORY-EP-084](../iter-006/stories/STORY-EP-084.md) | 3.1–3.4 Nodes and children | draft | 5 | A |
| [STORY-EP-085](../iter-006/stories/STORY-EP-085.md) | 3.5 Paint pen ink from the forest | draft | 5 | A |
| [STORY-EP-086](../iter-006/stories/STORY-EP-086.md) | 4 R-tree per container | draft | 8 | A (4.1), B |
| [STORY-EP-087](../iter-006/stories/STORY-EP-087.md) | 5 Concurrency | draft | 8 | B |
| [STORY-EP-088](../iter-006/stories/STORY-EP-088.md) | 6 Manipulation | draft | 8 | C |
| [STORY-EP-089](../iter-006/stories/STORY-EP-089.md) | 7 Connector | draft | 5 | D |
| [STORY-EP-090](../iter-006/stories/STORY-EP-090.md) | 8 Progressive paint | draft | 8 | E |
| [STORY-EP-091](../iter-006/stories/STORY-EP-091.md) | 9 Undo on the forest | draft | 5 | F |

Order: 082 → 083 → 084 → 085 → 086 → 087, then 088, 089 and 090 in any order (089 needs 088),
and 091 after 088. A story moves to `ready` when its predecessor is `done`.

## Cursor

**Next:** [STORY-EP-082](../iter-006/stories/STORY-EP-082.md) (Forest foundations) · the human
codes · agents guide with `/dev` (review) and `/architect-unified` (design questions).

## Execution board

[iter-006/execution-board-document-forest.md](../iter-006/execution-board-document-forest.md)

## Log

| Date | Event |
|---|---|
| 2026-10-04 | Opened after ADR-0041 was accepted and the iter-005 retro gate passed. Step 0 done: records ([CHL-0033](../iter-006/challenges/CHL-0033-forest-product-records.md)), sync deferred ([CHL-0034](../iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md)), stories EP-082…091. Replaces cancelled EP-078…080. |
