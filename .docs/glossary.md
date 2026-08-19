---
title: Glossary
lifecycle: active
owner: architect
---

# Glossary

Durable ubiquitous language. Anatomy lives in `.docs/domain/`; this file is vocabulary.

| Term | Meaning | Aliases | Related entities |
|---|---|---|---|
| Rest spine | Connector polyline `S` baked once at recognition, never rewritten from a warp | rest shape | [vector-document](./domain/vector-document.md) · [ADR-0020](./adr/ADR-0020-connector-ink-geometry.md) |
| Endpoint decoration | Ink bound to a connector **end** (not spine, not empty canvas) | endpoint ink | [ADR-0026](./adr/ADR-0026-endpoint-ink-membership.md) |
| Attachment `t` | Normalized arc-length of an attached node on the **rest** spine | hang parameter | [ADR-0027](./adr/ADR-0027-attachment-t-rest-spine.md) |
| Clipboard slot | One in-document copy buffer on the device; not the OS pasteboard | in-document clipboard | [ADR-0024](./adr/ADR-0024-in-document-clipboard.md) |
| Viewport token | Last-writer ownership of the shared viewport channel (`infini` \| `epaper`) | last-writer | [ADR-0023](./adr/ADR-0023-viewport-last-writer.md) |
| Pen-button map | Per-button Click + Hold-move bindings published as settings | barrel map | [pen-button-map](./domain/pen-button-map.md) |
| Hold-move | Barrel button down + movement past threshold until release; temporary-tool catalogue | hold | [pen-button-map](./domain/pen-button-map.md) |
| Click (barrel) | Barrel button down+up with movement below threshold; discrete catalogue | barrel click | [pen-button-map](./domain/pen-button-map.md) |
| Eraser nib | Distinct stylus **tool** report (hardware invert); not a barrel button | eraser end | [ADR-0025](./adr/ADR-0025-barrel-vs-eraser-nib.md) |
| Finger-eligible | Hit target ≥ **64 du** (primary ToolChip tile) | 64 du rule | [CHL-0019](../.plan/iter-004/challenges/CHL-0019-toolchip-tile-size.md) |

## Notes

- Prefer one canonical term per concept; list aliases so later docs stay consistent.
- Behaviour IDs remain `[REQ-NN]` / `[SRS-*]`; this file does not allocate them.
