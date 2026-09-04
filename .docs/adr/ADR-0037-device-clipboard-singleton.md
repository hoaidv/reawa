---
id: ADR-0037
title: Device clipboard singleton and tap-origin paste
status: accepted
date: 2026-09-04
deciders: [architect, pm]
supersedes: [ADR-0024]
source: TRACK-005 / [REQ-12]
---

# ADR-0037 — Device clipboard singleton and tap-origin paste

## Context

[REQ-12](../modules/epaper/prd.md#clipboard) needs copy / cut / paste **on the tablet**. [ADR-0024](./ADR-0024-in-document-clipboard.md) (proposed) put the slot on `DeviceDocument`, pasted at source AABB + **(24 u, 24 u)**, and published `duplicate_subtree` this track. First lock-in 2026-09-04: paste at a **long-press**. Same-day [CHL-0031](../../.plan/iter-005/challenges/CHL-0031-clipboard-tap-paste.md): paste is on the **normal** context toolbar from a **tap location**; long-press menu retired. Slot is **not** document state. Actions own orchestration. Infini apply is postponed.

Quality goals: local sufficiency, undo exactness (±1 px @ 100% zoom), ink-path untouched, paste invocable after cut (empty selection) when a tap location exists.

## Decision

1. **One process-global clipboard singleton** (not on `DeviceDocument`, not `DocContext`, not OS pasteboard). Survives `doc_load`; dies with the process.
2. **`CopyAction` / `CutAction` / `PasteAction`** clone, cut, and paste. They may use low-level `DocContext` (`commitEdit`, dirty, history, hit-test). They must **not** grow `copySelection` / `pasteClipboard` on `DocContext`.
3. **Copy** = slot only, 0 undo, 0 wire. **Cut** = slot then `remove_node` (empty groups **left**), 1 undo, 0 wire. **Paste** = remint + translate so union AABB top-left = **tap** world point + parent walk, 1 undo, 0 wire. Empty slot = 0 nodes. Parent a SmartGroup with `insertAt` (SmartGroup is not an `insertUnder` container). Free ink into a SmartGroup uses join-style local samples.
4. **Chrome:** copy/cut on the Selected strip; paste on that strip when the slot is non-empty **and** a tap location exists (tap empty or tap a node). Freeform / marquee clear the tap location → no paste. Empty-canvas paste strip sits at the tap panel point, then clamped (clamp ≠ paste origin).
5. **Tap vs travel** in SelectionMode: lock may happen on down, but **no document mutate** until travel > 1 mm. Lift with ≤ 1 mm: **select** (Primary and Secondary), record paste origin, 0 nudge. **No** 500 ms hold menu.
6. **No `duplicate_subtree` on the wire this track.** Local undo still uses ordinary tree edits (`remove_node` / insert / `compound`). Infini apply is a later track.
7. **Clone grain:** a selected root with no selected descendants is a **full subtree** (ink-box tap-select copies children).

## Consequences

- [SRS-EP-31](../modules/epaper/features/device-document/srs-logic.md#srs-ep-31-clipboard) / [SRS-EP-32](../modules/epaper/features/ink-box/srs-ui.md#srs-ep-32-clipboard-ui) / [SRS-EP-11](../modules/epaper/features/ink-box/srs-logic.md#srs-ep-11-hold-still) bind the numbers.
- `DeviceDocument::copyToClipboard` is retired.
- Sensitivity: **tap vs move** (1 mm). Wrong numbers reintroduce tap-nudge or steal lasso.

## Alternatives Considered

| Approach | Undo | Place | Routing | Why |
|---|---|---|---|---|
| ADR-0024 (+24, slot on document, publish paste) | + | − (not “anywhere”) | 0 | Rejected — human lock-in |
| OS / Qt pasteboard | 0 | 0 | 0 | Rejected — PRD Non-Goal |
| Paste verbs on DocContext | + | 0 | − | Rejected — complexity belongs on Actions |
| New ClipboardMode | 0 | 0 | − | Rejected — still a ToolAction |
| Long-press hold toolbar | + | + | − | Retired same day — [CHL-0031](../../.plan/iter-005/challenges/CHL-0031-clipboard-tap-paste.md) |
| **Singleton + tap origin (this ADR)** | + | + | + | Winner |

Trade-off point: **tap is paste origin** vs **toolbar may clamp**. Geometry stays on the unclamped tap.
