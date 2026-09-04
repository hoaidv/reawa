---
feature: erase
parent_req: [REQ-11]
lifecycle: active
module: epaper
version: 0.1.0
---

# SRS — Erase

**Parent:** [REQ-11](../../prd.md#erase). **Product (normative UI/UX):** [prd-erase.md](../../prd-erase.md). **Decision:** [ADR-0034](../../../../adr/ADR-0034-erase-clip-remnants.md) (clip + remnants), [ADR-0036](../../../../adr/ADR-0036-toolcanvas-live-overlay.md) (live overlay). **Mode:** [ADR-0033](../../../../adr/ADR-0033-tool-abstraction.md). **Undo:** [ADR-0032](../../../../adr/ADR-0032-inverse-op-undo.md).

This file binds algorithms, closed ids, and measures. It does not repeat the PRD job.

**Units:** erase millimetres in the PRD are **document world millimetres**. Panel **226 dpi** → **1 mm ≈ 8.90 du**. At 100% when 1 world unit = 1 mm: brush diameter **8 mm** ≈ **71 du**, radius **4 mm** ≈ **36 du**, remnant **1 mm**, hover stroke **0.5 mm**. Paint width still `world × panelScale` ([ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)).

---

## [SRS-EP-54] Eraser mode, ToolChip, routing, barrel, nib {#srs-ep-54-erase-mode}

**Parent:** [REQ-11](../../prd.md#erase). **Product (normative UI/UX):** [prd-erase.md](../../prd-erase.md). **Decision:** [ADR-0034](../../../../adr/ADR-0034-erase-clip-remnants.md), [ADR-0036](../../../../adr/ADR-0036-toolcanvas-live-overlay.md). **Mode:** [ADR-0033](../../../../adr/ADR-0033-tool-abstraction.md). **Undo:** [ADR-0032](../../../../adr/ADR-0032-inverse-op-undo.md).

| Rule | Value |
|---|---|
| Mode | One `Eraser` InteractionMode |
| Chip exclusives | `erase_brush` · `erase_area` · `erase_object` (order in eraser group) |
| Operations | `BrushErase` · `AreaErase` · `ObjectErase` — `acceptPrimary` only |
| Secondary | Navigation only (no Select / Move / Resize) |
| Chip layout | [prd-erase.md §3](../../prd-erase.md) — HT toggle **on** first cluster; trailing HT tile **gone** |
| Recognizers | Dimmed while any eraser armed; latch at pointer-down |
| Last-used | `{erase_brush, erase_area, erase_object}`; default `erase_brush`; persist device-local ([REQ-20](../../prd.md#device-settings)), not SVG |
| Click `toggle_pen_eraser` | `pen` ↔ last-used |
| Hold `temp_erase` | From `pen` only → last-used until release; chip mirrors; already-in-eraser → no-op |
| Nib | Brush mutation + chip shows `erase_brush` while inverted; restore exclusive on un-invert |
| Icons | `icon-epaper-erase-brush` · `icon-epaper-erase-area` · `icon-epaper-erase-object` (1-bit, 48×48) |
| Default launch | `pen` |

Empty erase gesture: 0 tree ops, 0 undo entries.

---

## [SRS-EP-55] Geometric clip and remnant split {#srs-ep-55-clip-remnants}

**Parent:** [REQ-11](../../prd.md#erase). **Decision:** [ADR-0034](../../../../adr/ADR-0034-erase-clip-remnants.md). Product: [prd-erase.md §10–11](../../prd-erase.md).

| Rule | Value |
|---|---|
| Clip | Polyline ∩ region in world; **not** sample-in-region drop |
| Brush region | Capsule radius **4 mm** along gesture |
| Area Ink region | Even-odd interior of auto-closed freeform (last→first); no min area |
| Remnant floor | Arc length ≥ **1 mm** |
| Split | Longest remnant keeps original `id`; extras `append_ink` as unused `{id}_rN` (skip ids already in the tree); 0 remnants `remove_node` |
| Gesture | One `compound` if >1 op; never `restore_snapshot` |
| Content remnants | `role: content`; reseed `layoutOffset` |
| Boundary ink remnants | `role: boundary`; broken surround allowed |
| Boundary polyline | Seeded at SmartGroup create (closed enclose copy); transforms with boundary ink; **persisted**; **never clipped** |
| Last visible ink | 0 ink children → `remove_node` SmartGroup (polyline does not keep an empty box) |
| Chord | **0** |

---

## [SRS-EP-56] Brush erase {#srs-ep-56-brush}

**Parent:** [REQ-11](../../prd.md#erase). Product: [prd-erase.md §7](../../prd-erase.md).

| Phase | Rule |
|---|---|
| Near (pen Primary) | Circle diameter **8 mm**, fill white, stroke **0.5 mm** black; **0** erase. Enter proximity **and** up→near. Kill-switch for field test |
| Down | White ghost polyline, width = eraser size, ToolCanvas |
| Up | Commit clip ([SRS-EP-55](#srs-ep-55-clip-remnants)); drop ghost **in the same refresh** as document damage |
| Kinds | Ink (incl. SmartGroup content/boundary ink) **and connector endpoint `styleInk`**. Primitive, Text, Frame, Connector **spine**: no-op |

---

## [SRS-EP-57] Area erase {#srs-ep-57-area}

**Parent:** [REQ-11](../../prd.md#erase). Product: [prd-erase.md §8](../../prd-erase.md).

| Phase | Rule |
|---|---|
| Down | Dotted freeform, ToolCanvas; no cover |
| Up | Auto-close last→first |
| Ink | Clip interior even-odd ([SRS-EP-55](#srs-ep-55-clip-remnants)) |
| Other (Primitive, Text, SmartGroup, Connector, Group) | `remove_node` if **fully inside** the closed polygon. SmartGroup → whole group. Connector → [SRS-EP-58](#srs-ep-58-object) attachment unbind |
| Frame | Never remove |
| Connector partial | Spine unchanged (no clip, no convert). Endpoint `styleInk` clips like Ink |

---

## [SRS-EP-58] Object erase and 80% table {#srs-ep-58-object}

**Parent:** [REQ-11](../../prd.md#erase). Product: [prd-erase.md §9](../../prd-erase.md).

| Phase | Rule |
|---|---|
| Down | Dotted freeform on ToolCanvas. Pointer-move **only appends**: stamp one dash-continuous segment onto an overlay raster (running `dashOffset`) and dirty that segment. **Never** rebuild or restroke all samples; **never** `drawLine` each sample with a fresh `DotLine` (looks solid). `paintOverlay` **blits** the raster, then deletion-rects. **Deletion-rect dirty is outline strips**, not the AABB interior (so add/remove of a candidate does not restroke the lasso). Live 80% is a **low-priority** worker ([`latest_job.hpp`](../../../epaper/util/latest_job.hpp)), timer-sampled, never on the pointer path. Walk **Frame** children only — do not enter SmartGroup or Group. Overlay skips Ink and Connector. SmartGroup uses a downsampled **boundary polyline**. Decision: [ADR-0036](../../../../adr/ADR-0036-toolcanvas-live-overlay.md). |
| Up | Auto-close; remove whole nodes that pass the table on the **full** lasso; SmartGroup boundary may be downsampled for the 8×8 area test; 0 remnants |

| Kind | Remove when |
|---|---|
| Ink | ≥80% **arc length** inside (even-odd) |
| SmartGroup | ≥80% **polygon area** of **boundary polyline** |
| Primitive, Text | ≥80% **world AABB area** |
| Connector | ≥80% **warped `V` length** removes the connector (decoration goes with it) |
| Connector endpoint `styleInk` stroke | ≥80% **arc length** inside → drop that stroke; connector stays |
| Frame | Never |

Connector remove (this section **or** area fully-inside): unbind attachments; reparent to connector’s parent; last derived world pose; adjacent paint order; undo rebinds. **0** convert-to-Ink.

---

## [SRS-EP-59] Erase quality {#srs-ep-59-erase-quality}

**Parent:** [REQ-11](../../prd.md#erase). Constrains SRS-EP-54…58. Product: [prd-erase.md §13–14](../../prd-erase.md).

| Scenario | Target |
|---|---|
| Pointer-up → settled tree | p95 ≤ **50 ms** |
| Ghost gone | Same refresh as document damage |
| Undo | One entry; ±1 px @ 100% zoom when `lastOpId` matches; skip/no-op [SRS-EP-07](../device-document/srs-logic.md) |
| Chip arm | p95 ≤ **300 ms** |
| Pen-tip ink | [SRS-EP-01](../local-pen-ink/srs-logic.md) unchanged |
| Live object-erase overlay | Pointer-move appends one dashed segment; 80% is timer + worker. Deletion-rect add/remove **blits** the raster — do not restroke the lasso ([ADR-0036](../../../../adr/ADR-0036-toolcanvas-live-overlay.md)) |
| Chords | 0 |
| Frame removed | 0 |
| Brush vs connector | 0 mutations |
| No session | Same local tree |
