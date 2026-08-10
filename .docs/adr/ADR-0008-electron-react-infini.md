---
id: ADR-0008
title: Infini desktop shell: Electron + React
status: accepted
date: 2026-08-10
deciders: [architect, pm]
supersedes: null
---

# ADR-0008 — Infini desktop shell: Electron + React

## Context

Infini ([REQ-01](../modules/infini/prd.md#infinity-canvas)–[REQ-03](../modules/infini/prd.md#tablet-sync))
needs a desktop app that pans/zooms an infinity canvas, opens SVG-ish documents, and
talks to Epaper over the local network. The team wants **fast iteration** and a path to
macOS / Windows / Linux without three native UI stacks. Quality drivers: **developer
velocity**, **cross-platform reach**, **gesture smoothness** (60 Hz), acceptable binary size.

## Decision

Build Infini as **Electron + React** (TypeScript). Canvas rendering uses a web
stack suitable for 2D transforms (prefer Canvas2D or WebGL via a thin scene layer —
exact renderer left to implement SRS). Native modules only where Node/Electron APIs
are insufficient (e.g. raw TCP to RM if needed).

## Consequences

- One codebase for desktop targets; packaging via electron-builder (or equivalent).
- Trade-off: higher memory vs Swift/AppKit or Qt; gesture path must be measured early
  (trackpad pinch via Electron/Chromium — validate on macOS first).
- Reawa stays Swift; Infini is a **sibling** app, not a rewrite of the pen driver.
- Follow-up: spike pan/pinch frame timing before locking renderer library.

## Alternatives Considered

| Approach | Velocity | Cross-platform | Gesture / canvas perf | Notes |
|---|---|---|---|---|
| **Electron + React** | + | + | 0 (must verify) | Chosen — matches effort constraint |
| SwiftUI / AppKit (macOS-only) | 0 | − | + | Best macOS fit; blocks Windows/Linux; duplicates web canvas skills |
| Qt / QML desktop | 0 | + | + | Team already on Qt for Epaper; heavier desktop UX investment |
| Tauri + React | + | + | 0 | Smaller binary; less mature native APIs; revisit if Electron weight hurts |

**Sensitivity:** canvas frame budget under continuous pinch. **Trade-off:** memory/weight vs ship speed and multi-OS.
