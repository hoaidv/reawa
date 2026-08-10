---
feature: vector-document
parent_req: [REQ-02]
version: 0.1.0
lifecycle: active
needs_design: true
---

# SRS — Vector document (UI)

## [SRS-IN-05] Open / save

| State | UI |
|---|---|
| `doc.none` | Empty canvas; affordance to open or new |
| `doc.open` | Title/path in chrome; save enabled |
| `doc.dirty` | Unsaved indicator |
| `doc.error` | Inline error on failed open/parse; canvas unchanged |

Minimal chrome only — no illustration-suite panels in v0.
