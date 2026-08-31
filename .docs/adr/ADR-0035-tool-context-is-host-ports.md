---
id: ADR-0035
title: ToolContext is host ports; Mode owns overlay policy
status: accepted
date: 2026-08-31
deciders: [architect]
supersedes: null
amends: [ADR-0033]
source: STORY-EP-069
---

# ADR-0035 — ToolContext is host ports; Mode owns overlay policy

## Context

[ADR-0033](./ADR-0033-tool-abstraction.md) split Router / Mode / Operation. Overlay **paint** moved
onto Operations and Modes, but `ToolCanvasContext` still classified chips (`isSelectionTool` /
`isEraserTool`), owned `ToolChrome`, and ran selection refresh for every tool. The name also implied
it *was* ToolCanvas. `ToolChrome` mixed generic overlay dirty-union with selection-only live-manip
and knob hits.

## Decision

1. **`ToolContext` / `ToolContextImpl`** are Qt host ports only: damage, visible, waveform,
   panel↔world, size. They forward `paintOverlay` / `syncOverlay` / `refreshChrome` to the active
   Mode. Zero exclusive-id compares. Zero knob / live-manip methods.

2. **Mode** owns overlay **policy**: when the overlay is attached, Pen vs Mono, when to paint
   settled vs live selection overlay, and what `refreshChrome` does per Mode.

3. **`SelectionOverlay`** (formerly ToolChrome) is ADR-0019 ToolCanvasLayer selection state
   (settled AABB, knobs, live subtree fill, refuse, knob hits). **Owned by `ToolCanvasItem`**,
   injected on `HostCaps`. Not a member of `ToolContextImpl`. Not inlined into `toolcanvasitem.cpp`.
   Generic overlay dirty-union lives on `ToolContextImpl`.

4. **Chip-string → `ModeId`** lives only on `ToolCanvasItem::syncActiveMode`. Session exclusive id
   for `Operation::match` is `DocContext::exclusiveTool()`.

Working contract: [tool-system/principles.md](../modules/epaper/tool-system/principles.md).

## Consequences

- Adding a tool must not grow `ToolContextImpl`. Policy goes on a Mode or on `SelectionOverlay` if
  it is selection ToolCanvasLayer.
- Changing this split requires a challenge and an ADR amend, same as forking SRS.

## Alternatives considered

| Approach | Why not |
|---|---|
| Keep `ToolCanvasContext` as SelectionOverlay host | Recreates the god-class |
| Merge `SelectionOverlay` into `ToolCanvasItem` | Recreates TRACK-006 mixed-logic file |
| Leave chip classification on ToolContext | Every new chip edits the generic adapter |
