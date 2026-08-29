---
id: ADR-0025
title: Barrel-button channel vs hardware eraser nib
status: proposed
date: 2026-08-19
deciders: [architect, pm]
supersedes: null
source: TRACK-005 / [REQ-11] / [REQ-18]
---

# ADR-0025 — Barrel-button channel vs hardware eraser nib

**Amended 2026-08-29.** Channel isolation (nib vs barrel) still stands. Path A sample-delete and Path B selection-erase **mutation** are retired: [ADR-0034](./ADR-0034-erase-clip-remnants.md), [prd-erase.md](../modules/epaper/prd-erase.md), [CHL-0028](../../.plan/iter-005/challenges/CHL-0028-eraser-three-tools.md). Nib = brush; barrel Click `toggle_pen_eraser` / Hold `temp_erase` = last-used eraser.

## Context

[REQ-11](../modules/epaper/prd.md#erase) Path A is the **stylus eraser nib** (distinct tool report from the digitizer). Path B is selection-erase. Barrel **hold-move = temporary erase** is an accelerator of Path A’s *feel*, bound via [REQ-18](../modules/epaper/prd.md#pen-buttons) — “not a third grammar.” [REQ-18] also says eraser nib is **not** barrel 2.

Implementers will otherwise collapse “the other end of the pen” and “the barrel button” into one `eraser` tool flag. On Wacom EMR they are **different HID reports**. A 0-button pen can still have a nib; a 2-button pen can lack a distinct eraser tool.

Quality: Path A p95 ≤50 ms after gesture end; 0 accidental erases when there is no nib; 0 events that fire **both** click and hold-move; tool switch p95 ≤300 ms.

## Decision

Two **input channels**, one **erase mutation**.

| Channel | Source | Routes to | Must not |
|---|---|---|---|
| **Nib** | Digitizer `eraser` / inverted tool (when hardware reports it) | REQ-11 Path A stroke-erase in [SRS-EP-27](../modules/epaper/features/local-pen-ink/srs-logic.md) | Start ink; run barrel click/hold classifiers |
| **Barrel** | Button down/up + movement vs threshold | REQ-18 catalogues in [SRS-EP-41](../modules/epaper/features/tool-modes/srs-logic.md) | Pretend to be a nib; drive Path A unless Hold-move is bound to `temp_erase` |
| **Selection-erase** | Chip / bound Click `toggle_pen_eraser` completing as Erase command / explicit Erase CTA | REQ-11 Path B in [SRS-EP-28](../modules/epaper/features/device-document/srs-logic.md) | Run while selection empty (no-op) |

Shared mutation for stroke-erase (nib **or** `temp_erase` hold-move): remove intersecting **ink samples**; delete a node that has **no remaining samples**; **0** new Ink nodes; one undo restores the pre-erase document via the inverse of `set_ink_samples` and, when the forward also emptied a node, restore of that body (`remove_node` inverse) — not a whole-tree snapshot ([ADR-0032](./ADR-0032-inverse-op-undo.md)).

Classifier isolation:

1. Nib-down never enters the barrel Click vs Hold-move state machine.
2. Barrel-down never sets the digitizer eraser tool flag.
3. Latch catalogues at **button-down**; a rebind (on-device editor, or Infini restore) applies to the **next** gesture ([ADR-0030](./ADR-0030-tablet-authors-pen-button-map.md)).
4. Movement threshold splits Click vs Hold-move; **0** dual fire (REQ-18 20-gesture fixture).

## Consequences

- Tool-modes own barrel dispatch; local-pen-ink owns nib geometry; device-document owns the erase/undo op. Do not hang nib erase on [SRS-EP-04](../modules/epaper/features/tool-modes/srs-logic.md) as parent.
- On-device map editor ([SRS-EP-52](../modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor); [SRS-IN-24](../modules/infini/features/tablet-sync/srs-ui.md#srs-in-24-pen-map-ui) retired) lists barrel catalogues only — **no** “eraser nib” slot. UI home amended 2026-08-20 by [ADR-0030](./ADR-0030-tablet-authors-pen-button-map.md); routing in this ADR is unchanged.
- Missing nib: Path A does not fire; Path B + optional `temp_erase` remain.
- Missing buttons: 0 barrel gestures; ToolChip still complete.

## Alternatives Considered

| Approach | Accidental erase | Accelerator completeness | HID fidelity | Why |
|---|---|---|---|---|
| One `eraser` tool for nib + barrel | − | 0 | − | Rejected — 0-button + nib, or 2-button without nib, both misfire |
| Barrel only, ignore nib | − (on nib pens) | − | − | Rejected — REQ-11 Path A |
| Nib only, barrel never erases | + | − | + | Rejected — REQ-18 default B2 Hold-move = temporary erase |
| **Two channels, one mutation (this ADR)** | + | + | + | Winner |

Trade-off point: **one creator-facing erase feel** vs **two HID reports**. Feel is shared; routing is not.
