---
id: STORY-IN-002
title: "Scaffold Electron React Infini shell and canvas host"
kind: implement
parent_srs: [SRS-IN-01]
parent_req: [REQ-01]
status: done
priority: P1
iter: iter-002
estimate: 3
owner: dev
depends_on: [STORY-IN-001]
acceptance_criteria:
  - "Given ADR-0008, When the Infini app boots on macOS, Then an Electron window loads a React root that hosts a full-pane canvas surface (scene.canvas)."
  - "Given the shell, When the window opens, Then there is a stable mount point for the infinity canvas renderer (no production Reawa Swift code reused)."
  - "Given design package infinity-canvas, When implementing chrome, Then layout follows approved hi-fi regions/tokens (not invented layout)."
design_package: ".plan/iter-002/design/infinity-canvas/"
ui_spec: ".plan/iter-002/design/infinity-canvas/ui-spec.md"
scenes:
  - ".plan/iter-002/design/infinity-canvas/infinity-canvas-empty.html"
  - ".plan/iter-002/design/infinity-canvas/infinity-canvas-populated.html"
  - ".plan/iter-002/design/infinity-canvas/infinity-canvas-gesturing.html"
  - ".plan/iter-002/design/infinity-canvas/infinity-canvas-resized.html"
hifi: ".plan/iter-002/design/infinity-canvas/infinity-canvas-populated.html"
wireframe: ""
---

# STORY-IN-002 — Scaffold Electron React Infini shell and canvas host

Implements shell for [SRS-IN-01](../../../.docs/modules/infini/features/infinity-canvas/srs-logic.md)
per [ADR-0008](../../../.docs/adr/ADR-0008-electron-react-infini.md). **Blocked on** [STORY-IN-001](./STORY-IN-001.md).

Status stays `draft` until design story is `done`; then set `ready` and copy `ui_spec`/`hifi`.
