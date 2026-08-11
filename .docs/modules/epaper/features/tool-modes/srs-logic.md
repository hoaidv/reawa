---
feature: tool-modes
parent_req: [REQ-03]
version: 0.1.0
lifecycle: active
---

# SRS — Tool modes Epaper (Logic)

Device-side rules for [REQ-03](../../prd.md#tool-modes).
Decision: [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md).
Wire peer: [SRS-IN-13](../../../infini/features/tablet-sync/srs-logic.md#srs-in-13-tool-intent-transport).

**Implementation status (code SoT, 2026-08-11):** ToolChip + `toolMode` + `stroke_begin.intent`
+ `pickables` ingest + `tool_intent` emit in `tabletcanvasitem` / `Main.qml` (STORY-EP-005).
Touch-on-chip uses MouseArea; pen-on-chip press is ignored for ink (fallback path).

## [SRS-EP-04] Tool state and intent emission

### Tool state

| Rule | Value |
|---|---|
| Tools | `selection` \| `pen` \| `ink_box` |
| Default on launch | `pen` — the device must still be a notebook if nothing else works |
| Ownership | **Device-local UI state.** Never sent to Infini, never set by Infini (ADR-0013 §1) |
| Persistence | Not persisted across restarts in v0 |
| Input | Finger touch on the ToolChip (pen-on-chip fallback). Pen events on the chip are not ink |

### Input routing

| Tool | Pen down on canvas | Finger / pen on ToolChip | Finger on canvas |
|---|---|---|---|
| `pen` | Local ink + `stroke_*` with `intent: ink` | Switch tool | Ignored (no on-device pan — PRD Non-Goal) |
| `ink_box` | Local ink + `stroke_*` with `intent: enclose` | Switch tool | Ignored |
| `selection` | Pick / move / resize (below) | Switch tool | Ignored |

`ink_box` and `pen` differ **only** in the emitted `intent` field — the local ink path, the
Round 19 map, and the paint are byte-identical, so ink latency cannot regress by tool
([SRS-EP-01](../local-pen-ink/srs-logic.md)).

### Enclose intent

| Step | Rule |
|---|---|
| Arm | Creator taps `Ink-box`; the tool **stays armed** for repeated boxes until switched |
| Emit | `stroke_begin` carries `intent: "enclose"`; points and end are unchanged |
| Device role | Draw the stroke locally as ordinary ink. **No** rectangle fitting, **no** containment test |
| Result | Arrives as a normal `doc_snapshot` re-raster; the device learns nothing about grouping |
| Guards fail on Infini | Nothing comes back; the stroke simply stays ink on the panel (already drawn) |

### Selection intent

| Step | Rule |
|---|---|
| Pickable source | `pickables[]` from the most recent `doc_snapshot` (id, kind, world bounds) |
| Hit-test | Local: `panelToWorld(pen)` inside a pickable's bounds; topmost (last in array) wins |
| Select | Draw selection affordance locally (bounds + 8 handles); emit `tool_intent { action: "select", nodeId }` |
| Move | Drag renders a **local ghost** of the bounds (dashed, composited chrome ≥20 Hz dirty-rect); on pen-up emit `tool_intent { action: "move", nodeId, delta }` |
| Resize | Drag on a handle band; on pen-up emit `tool_intent { action: "resize", nodeId, bounds }` |
| Authority | The ghost is advisory. The next `doc_snapshot` is truth, even if geometry jumps |
| No pickables | Selection tool is inert — pen does nothing on canvas; must be visible in the UI |
| Miss | Press outside every pickable clears selection |

### Tool independence

Device tool mode is **never synced**. Infini may be on Selection / Ink-box while Epaper is on
Ink-box — enclose recognition is driven solely by `stroke_begin.intent` from the drawing device
([SRS-IN-10](../../../infini/features/vector-document/srs-logic.md)). The peer tool does not
gate ingest or create.

### Errors / partial failure

| Case | Behavior |
|---|---|
| Touch layer unavailable at runtime | Fall back to `pen` permanently; surface it in the status line; never trap the creator in a non-drawing tool |
| Session down | `pen` still inks locally (REQ-01 is offline-capable); `selection` / `ink_box` show as unavailable |
| Snapshot older than the creator's edits | Ghost discarded on next snapshot; last write wins (no locking, ADR-0013 §4) |
| Pen-down starts on the ToolChip bounds | Not ink; may arm a tool when pen-on-chip fallback is active |

### Other logic

- **Exclusion rect = ToolChip bounds** (floating chip), not a full edge band. A stroke must never
  begin inside that rect; `InkSurface` stays full-bleed ([SRS-EP-05](./srs-ui.md), CHL-0003).
- Tool switching must not invalidate the full panel — partial refresh of the **chip** only
  (the ink area keeps its content, [SRS-EP-06](./srs-quality.md)).
- Chip anchor follows **gut orientation top** (see SRS-EP-05); exclusion rect moves with it.
