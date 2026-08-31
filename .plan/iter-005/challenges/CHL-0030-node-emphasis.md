---
id: CHL-0030
author: dev
target: [SRS-EP-12, SRS-EP-01]
severity: high
status: adopted
opened: 2026-08-31
iter: iter-005
expedite: true
interrupts_track: TRACK-005
---

# CHL-0030 — NodeEmphasis owns blink / stroke-stamp / AABB highlight

## Context

[STORY-EP-032](../../iter-004/stories/STORY-EP-032.md) parked an architect chrome-state machine.
Stopgap: `TabletCanvasItem` `m_blink*` / `m_highlightInkIds` baked 2× width into the document
FullClear ([CHL-0020](../../iter-004/challenges/CHL-0020-recog-width-blink.md), UI-EP-06). That
second FullClear is part of the dense-page ink hitch.

Blink, last-join bold, future dotted-stamp, and AABB highlight are **not** selection chrome and
**not** an in-flight Operation (they outlive `InkStroke` unlock).

## Proposal

Host-owned `NodeEmphasis` (sibling of `SelectionOverlay`):

- `ToolCanvasItem` owns it; `HostCaps.emphasis`; Operations / ingest hint call the API.
- Every Mode `paintOverlay` paints it (emphasis first, then in-flight Operation).
- Overlay 2× halo on ToolCanvas (Mono, partial AABB/spine damage). Document stays 1×.
- Must **not** live on: TabletCanvas flags, SelectionOverlay, ToolContextImpl, InputHub,
  `ToolCanvasItem::paint`, DocContext.

Object-erase deletion rects stay on `ObjectEraseOperation` this slice ([ADR-0036](../../../.docs/adr/ADR-0036-toolcanvas-live-overlay.md));
`showAabb` exists for later migration.

## Resolution

**Adopted** 2026-08-31 with the hitch interrupt. Principles + catalog updated in the same change.
**Field-verified 2026-08-31:** membership Bold must not keep ToolCanvas Mono visible during the next
ink stroke; paint uses `includeIds` (not HierarchyCull of the group AABB).

## Product doc updates

- [`.docs/modules/epaper/tool-system/principles.md`](../../../.docs/modules/epaper/tool-system/principles.md)
- [`.docs/modules/epaper/tool-system/catalog.md`](../../../.docs/modules/epaper/tool-system/catalog.md)
- [`.docs/memory/ink-path-density-hitch.md`](../../../.docs/memory/ink-path-density-hitch.md)

## Interrupt / expedite

TRACK-005. Slice of STORY-EP-032: one explicit owner for blink / highlight / AABB.
