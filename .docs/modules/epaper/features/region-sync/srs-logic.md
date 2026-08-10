---
feature: region-sync
parent_req: [REQ-02]
version: 0.1.0
lifecycle: active
---

# SRS — Region sync Epaper (Logic)

## [SRS-EP-02] Viewport map vs panel refresh

### Other logic

- On viewport message: update input→world and ink transform **immediately**; enqueue region refresh.
- On local pen: ink locally with current map ([SRS-EP-01](../local-pen-ink/srs-logic.md)); append ops to document channel.
- On refresh: rasterise **current document ∩ current drawing region** — never a stale document with a new map or vice versa for the paint pass.
- Ghosting / laggy pixels allowed; divergent document content for that region is not.

[ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md).
