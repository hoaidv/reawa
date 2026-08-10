---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.1.0
lifecycle: active
---

# SRS — Tablet sync Infini (Logic)

## [SRS-IN-07] Session roles

### Other logic

- Infini **owns viewport** (`translate`, `scale`, drawing-region AABB) and publishes viewport-channel messages on change (debounced only if needed for bandwidth; mapping priority > paint).
- Infini **applies document ops** from Epaper onto the in-memory model and renders them under the canvas transform.
- v0: Infini does not mutate strokes while Epaper is the active editor (viewer + viewport).
- Transport: local network to RM2; concrete framing TBD (EXP TCP JSON-lines is the spike baseline).

See [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md).
