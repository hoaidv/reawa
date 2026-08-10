---
from: sm
to: designer
iter: iter-002
date: 2026-08-10
subject: design-pickup-infinity-canvas
---

# Handoff: SM → Designer — Infini infinity canvas

## Cursor

**STORY-IN-001** — Design Infini infinity canvas  
Package: `.plan/iter-002/design/infinity-canvas/`  
SRS: [SRS-IN-02](../../../.docs/modules/infini/features/infinity-canvas/srs-ui.md)  
REQ: [REQ-01](../../../.docs/modules/infini/prd.md#infinity-canvas)

## Lock (honor)

```
direction: vertical · wip 1 · NOW feature: infini/infinity-canvas
stop_line: verified · autonomy: bounded · out_of_scope: backlog
Do not start STORY-IN-006 (vector-document) in parallel.
```

## States to cover

`canvas.empty` · `canvas.populated` · `canvas.gesturing` · `canvas.resized`

## Gestures to annotate

Trackpad pan · mouse drag pan · wheel pan · modifier+wheel zoom · pinch zoom

## Done-when

- Package scenes + `ui-spec.md`; `ui-spec-gate` pass
- Story frontmatter `ui_spec` / `scenes` / `hifi` filled; status → `done`
- Decide resize world-anchor (center vs top-left) for implement handoff

## After you

SM marks implement stories `ready` (still gated on your `done`); then **`/qa`** BDD for
STORY-IN-002…005, then **`/dev`**.
