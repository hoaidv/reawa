---
title: Domain — shared vocabulary
lifecycle: active
owner: architect
---

# Domain

Concepts that **more than one module implements**. A domain doc is the single semantic
authority for such a concept; module SRS files bind their implementation to it rather than
re-describing it.

Created 2026-08-13 by [ADR-0014](../adr/ADR-0014-document-ownership-inversion.md): once the device
and the desktop both hold the document tree, its anatomy stopped being an Infini-only SRS detail.

| Doc | Concept | Implemented by |
|---|---|---|
| [vector-document.md](./vector-document.md) | The document tree — node kinds, roles, transforms, ops, invariants; connector terminals + attachments | `epaper` (C++, device-side working document) · `infini` (TypeScript, mirror + persistence) |
| [pen-button-map.md](./pen-button-map.md) | Barrel-button Click / Hold-move map (settings, not document) | `infini` (author, persist, publish) · `epaper` (consume, dispatch) |

## Rules

- **Semantics here, mechanics in the module.** A domain doc says what a node *is* and what an op
  *means*. Wire framing, storage layout, and rendering belong to the module SRS.
- **Two implementations, one meaning.** Where two modules implement a domain concept, they must
  agree on shared fixtures. Divergence is a `CHL-*`, never a local workaround.
- **Domain docs do not carry `[SRS-*]` ids.** They are referenced *by* SRS sections, which keep the
  traceability ids.
