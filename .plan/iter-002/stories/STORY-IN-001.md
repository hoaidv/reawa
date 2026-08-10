---
id: STORY-IN-001
title: "Design Infini infinity canvas"
kind: design
parent_srs: [SRS-IN-02]
parent_req: [REQ-01]
status: ready
priority: P1
iter: iter-002
estimate: 3
owner: designer
depends_on: []
acceptance_criteria:
  - "Given SRS-IN-02 states canvas.empty, canvas.populated, canvas.gesturing, canvas.resized, When the design package ships, Then each state has a package-contained scene HTML (or documented combo) traced in ui-spec."
  - "Given gesture map (trackpad pan, mouse drag pan, wheel pan, modifier+wheel zoom, pinch zoom), When ui-spec is written, Then each input→action is shown or annotated on the primary scene."
  - "Given scene.canvas is the composition, When hi-fi is produced, Then no card chrome dominates the first viewport; transform = translate + uniform scale is visible via grid/primitives."
  - "Given platform Electron desktop (macOS first), When ui-spec records platform profile, Then resize anchor choice (center vs top-left) is decided and noted for SRS-IN-02 canvas.resized."
design_package: ".plan/iter-002/design/infinity-canvas/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-001 — Design Infini infinity canvas

Designs UI for [SRS-IN-02](../../../.docs/modules/infini/features/infinity-canvas/srs-ui.md)
([REQ-01](../../../.docs/modules/infini/prd.md#infinity-canvas)).

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | designer |
| Package | `.plan/iter-002/design/infinity-canvas/` |

## Acceptance (design)

- Platform: Electron desktop, macOS-first; responsive = window resize, not mobile web.
- States: `canvas.empty` | `populated` | `gesturing` | `resized` each covered.
- Gesture legend for pan/zoom/pinch inputs from SRS gesture map.
- `ui-spec-gate` pass; set `ui_spec`, `scenes`, `hifi` on this story when done.
