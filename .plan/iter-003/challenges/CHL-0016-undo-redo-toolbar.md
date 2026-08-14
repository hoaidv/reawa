---
id: CHL-0016
title: Undo and Redo on the primary toolbar
author: sm
target: [SRS-EP-05]
severity: medium
status: resolved
resolution: adopted
resolved_by: pm
resolved: 2026-08-14
opened: 2026-08-14
iter: iter-003
expedite: false
interrupts_track: ""
raised_by: human
source: human layout — Selection Rect | Selection Freeform | Pen | Ink-box | gap | Undo | Redo
---

# CHL-0016 — Undo and Redo on the primary toolbar

## Context

[CHL-0010](./CHL-0010-undo-vs-selection-create-chrome.md) deferred **on-panel undo chrome** so the
closed tool inventory would not grow a fifth drawing tool. The undo **ring** already ships
([STORY-EP-015](../stories/STORY-EP-015.md)). Redo was out of scope in [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md).

Human (2026-08-14) specified the primary bar:

**Selection Rect | Selection Freeform | Pen | Ink-box | ⟨space⟩ | Undo | Redo**

## Proposal

Adopt history **actions** on the same floating strip, visually grouped after a gap. They are not
exclusive tools (they do not steal the armed tile). Enclose stays off-chip ([ADR-0016](../../../.docs/adr/ADR-0016-selection-create-enclose-cta.md)).

## Resolution

**Adopted** — 2026-08-14 (human / PM).

1. Tool inventory stays four exclusive arms. Undo and Redo are **actions** after a 32 du gap.
2. Redo is **in scope** as the inverse of the snapshot ring (depth 20). A new structural commit
   clears redo. Empty undo/redo is a no-op. Mid-gesture requests latch like undo.
3. [CHL-0010](./CHL-0010-undo-vs-selection-create-chrome.md) chrome deferral is **superseded** for
   this layout only. Enclose still must not join the exclusive tool row.

## Product doc updates

- [SRS-EP-05](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md) — history cluster
- [SRS-EP-07](../../../.docs/modules/epaper/features/device-document/srs-logic.md) — redo stack
- [ADR-0018](../../../.docs/adr/ADR-0018-undo-redo-chip-actions.md)

## Interrupt / expedite (when applicable)

Not expedite. Parallel to W11b (EP-019). Does not invent Enclose-as-tool.
