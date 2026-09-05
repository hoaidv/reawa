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
| Endpoint decoration | Ink bound to a connector **end** (not spine, not empty canvas); stored as `ConnectorAnchor.styleInk` in the live face frame | endpoint ink | [ADR-0038](./adr/ADR-0038-endpoint-ink-face-frame.md) |
| Attachment `t` | Normalized arc-length of an attached node on the **rest** spine | hang parameter | [ADR-0027](./adr/ADR-0027-attachment-t-rest-spine.md) |
| Clipboard slot | One process-global in-document copy buffer on the device; not the OS pasteboard; not on DeviceDocument | in-document clipboard | [ADR-0037](./adr/ADR-0037-device-clipboard-singleton.md) |
| Node lastOpId | `opId` of the last **forward** document-semantic mutation still in effect on that node. Undo restores the captured `prevLastOpId`; redo restamps the original forward id. Mismatch vs an undo entry’s forward `opId` ⇒ skip (no undo-through). History publish ids (`undo:N` / `redo:N`) are not stored here | node revision | [vector-document](./domain/vector-document.md#node-revision) · [ADR-0032](./adr/ADR-0032-inverse-op-undo.md) |
| Session undo stack | One device document-epoch LIFO pair (undo ring + redo), depth 20 each; emptied on accepted `doc_load` or process death. Infini is not a second author and has no stack | undo session | [ADR-0032](./adr/ADR-0032-inverse-op-undo.md) · [SRS-EP-07](./modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document) |
| Viewport follow | Optional exclusive coupling of two independent cameras (`none` \| `infini_to_epaper` \| `epaper_to_infini`) | follow | [viewport-follow](./domain/viewport-follow.md) · [ADR-0029](./adr/ADR-0029-independent-cameras-viewport-follow.md) |
| Viewport token | **Retired 2026-08-20.** Last-writer ownership of a shared viewport. Do not implement. | last-writer | [ADR-0023](./adr/ADR-0023-viewport-last-writer.md) superseded by [ADR-0029](./adr/ADR-0029-independent-cameras-viewport-follow.md) |
| Pen-button map | Per-button Click + Hold-move bindings; tablet authors the live map and persists it on this Epaper device; not Infini, not the document | barrel map | [pen-button-map](./domain/pen-button-map.md) · [ADR-0031](./adr/ADR-0031-device-settings-persist-on-epaper.md) |
| Hold-move | Barrel button down + movement past threshold until release; temporary-tool catalogue | hold | [pen-button-map](./domain/pen-button-map.md) |
| Click (barrel) | Barrel button down+up with movement below threshold; discrete catalogue | barrel click | [pen-button-map](./domain/pen-button-map.md) |
| Eraser nib | Distinct stylus **tool** report (hardware invert); not a barrel button | eraser end | [ADR-0025](./adr/ADR-0025-barrel-vs-eraser-nib.md) · [prd-erase.md](./modules/epaper/prd-erase.md) |
| Boundary polyline | Invisible closed SmartGroup polygon for object-erase area; not visible ink | — | [vector-document](./domain/vector-document.md) · [ADR-0034](./adr/ADR-0034-erase-clip-remnants.md) |
| Last-used eraser | Last armed of `erase_brush` \| `erase_area` \| `erase_object`; barrel Click/Hold target | — | [prd-erase.md](./modules/epaper/prd-erase.md) |
| Finger-eligible | Hit target ≥ **primary ToolChip tile** (64 du / CHL-0019). Resize knobs must meet this floor so finger can resize ([CHL-0024](../.plan/iter-005/challenges/CHL-0024-finger-resize-knobs.md)) | 64 du rule | [CHL-0019](../.plan/iter-004/challenges/CHL-0019-toolchip-tile-size.md) |
| Natural area | Area of a node’s own frame (SmartGroup local `bounds`, or Ink sample AABB) after its full world outcome; used for 80% capture / reparent | — | [vector-document](./domain/vector-document.md) · [ADR-0039](./adr/ADR-0039-nested-ink-box-rendering.md) |
| RenderingContext | Affine passed down while painting or world-hitting a subtree; world starts at identity | compose context | [ADR-0039](./adr/ADR-0039-nested-ink-box-rendering.md) |
| Own-transform | A SmartGroup’s stored `{ translate, rotation, scaleX, scaleY }`; nested move/resize mutates this only | group transform | [ADR-0011](./adr/ADR-0011-smart-group.md) · [ADR-0039](./adr/ADR-0039-nested-ink-box-rendering.md) |
| Operation | One locked pointer gesture (not a chip tile) | Op | [tool-system](./modules/epaper/tool-system/concepts.md) |
| PointerRole | Routing axis Primary/Secondary from DeviceMap; not physical Pen vs Finger | role | [tool-system](./modules/epaper/tool-system/routing.md) |
| ToolModifier | Orthogonal chip toggle; never exclusive | Modifier | [tool-system](./modules/epaper/tool-system/concepts.md) |

## Notes

- Prefer one canonical term per concept; list aliases so later docs stay consistent.
- Behaviour IDs remain `[REQ-NN]` / `[SRS-*]`; this file does not allocate them.
