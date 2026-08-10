---
feature: vector-document
parent_req: [REQ-02]
version: 0.1.0
lifecycle: active
---

# SRS — Vector document (Logic)

## [SRS-IN-04] Three representations

### Other logic

1. **In-memory model** — ordered collection of vector objects (id, type, geometry in world space, stroke style). Source of truth during a session ([ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md)).
2. **Persistence** — SVG profile (subset TBD in open question): enough to round-trip paths/primitives Infini can draw. Open/save maps SVG ↔ model.
3. **Transmit encoding** — compact op list (append stroke, ack, snapshot optional) shared with Epaper. Must round-trip to an equivalent in-memory model.

### Invariants

- Opening persistence into Infini yields the same world geometry (±1 px @ scale=1).
- Transmit deserialize ∘ serialize ≡ identity on supported ops (golden fixtures).
