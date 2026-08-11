---
id: ADR-0011
title: Smart Group (ink-box) pilot
status: accepted
date: 2026-08-11
deciders: [architect, pm]
supersedes: null
---

# ADR-0011 — Smart Group (ink-box) pilot

## Context

Creators often handwrite a thought on Epaper, then **draw a rectangle around it** wanting
that cluster to behave like a **text box**: move together, scale/stretch/rotate the frame,
optionally keep ink size while the box grows, and attach **connectors** to the box — without
OCR (ink stays ink).

[ADR-0010](./ADR-0010-tree-of-vectors.md) already has `Group` (nest anywhere) and `Text`
(typed paragraphs). Neither is an **ink-preserving box with an explicit boundary and
scale policy**. ADR-0010 also deferred **local transforms** for ordinary groups (world-space
children in v0). Smart Group needs a local transform for the pilot to feel right.

Parent: [REQ-04](../modules/infini/prd.md#smart-group) (pilot) · tree model [REQ-02](../modules/infini/prd.md#vector-document).

## Decision

1. **New node kind: `SmartGroup`** (aka ink-box)  
   - Contains one or more **`Ink`** children (and optionally other leaves later; v0 pilot =
     ink only). Children are tagged `role: content | boundary`.  
   - **Ink is never converted to text** by this feature (content or boundary).  
   - Has an explicit geometric **`bounds`** rectangle `(x, y, width, height)` in local space —
     **recognized once** from the enclose stroke (or AABB on explicit create). Bounds drive
     handles, hit-testing, and connector ports; they are updated when the user transforms the
     SmartGroup.  
   - **Boundary ink** = the preserved hand-drawn enclose stroke (and any later drawn box ink).
     It is rendered and transformed with the group; it is not replaced by a synthetic vector
     rect unless the user chooses a clean chrome later (out of pilot).  
   - Is a first-class **connector target** (anchors use geometric `bounds`, not the wiggly
     ink path).
2. **Local transform (pilot exception to ADR-0010 world-only children)**  
   SmartGroup stores `{ translate, rotation, scaleX, scaleY }` (or equivalent affine). Child
   ink is authored in **group-local** coordinates. World draw = `groupTransform ∘ localSample`.
   Ordinary `Group` / `Frame` remain world-space-children until a later ADR; **only
   `SmartGroup` gets local transform in this pilot**.

3. **Ink scale policy (`inkScaleMode`) — content ink only**  
   | Mode | Feel | Behavior when bounds/scale change |
   |---|---|---|
   | `withBounds` (default) | Object scale | **Content** ink scales/rotates with the SmartGroup transform |
   | `fixedInk` | Text-box chrome | Geometric bounds + **boundary ink** still transform; **content** ink sample size stays fixed (box grows like padding around handwriting) |

   Exact `fixedInk` layout rule: keep **content** ink’s local geometry unchanged while `bounds`
   width/height change (letterbox / padding); do **not** OCR or reflow glyphs.

   **Boundary ink always transforms** with the SmartGroup (same as the geometric frame). It is
   **not** gated by `inkScaleMode`. Only `role: content` ink respects the mode.
4. **Creation paths (pilot)**  
   A. **Enclose recognition:** user hand-draws a roughly rectangular stroke that surrounds
      existing ink → system proposes SmartGroup. On accept:
      - **Recognize** initial axis-aligned `bounds` `(x, y, width, height)` from the enclose
        stroke (fitted rect / AABB of that stroke — used for handles, hit-testing, connectors).
      - **Preserve** the enclose stroke as **boundary ink** inside the SmartGroup (do **not**
        discard it). It always follows group transforms (never `fixedInk`-exempt).
      - Reparent **content ink** (strokes inside) as siblings under the SmartGroup (`inkScaleMode`
        applies to these only).   B. **Explicit:** multi-select ink → “Smart Group” → bounds = AABB; optional drawn boundary
      ink may be added later. Any ink set can become a SmartGroup — rectangle gesture is the
      delightful path, not the only path.

5. **Recognition quality (pilot bar)**  
   Enclosure detection is **best-effort**. False positives must be undoable. Misses fall back
   to explicit create. No requirement for perfect RM→geometry in v0 — measurable accept rate
   in SRS-quality.

6. **Persistence / ops**  
   `kind: smart_group` in tree + SVG `data-infini-kind="smart-group"` with bounds, transform,
   `inkScaleMode`, children. Ops: `create_smart_group`, `set_smart_group_transform`,
   `set_ink_scale_mode`, `recognize_enclose` (optional client-side).

## Consequences

- Pilot can ship without OCR, text layout, or full Figma-like groups.  
- Viewport remains translate+uniform scale only; **node** rotation lives on SmartGroup.  
- Epaper may only draw ink; recognition/create may run on Infini after sync (or on-device later).  
- Connector glue uses SmartGroup world bounds after transform.  
- Follow-up: promote local transforms to ordinary Group; richer enclose shapes (ellipse).

## Alternatives Considered

| Approach | Ink fidelity | Text-box feel | Sync complexity | Notes |
|---|---|---|---|---|
| **SmartGroup + local TF + inkScaleMode** | + | + | 0 | **Chosen** for pilot |
| OCR to Text node | − handwriting | + | 0 | Rejected — user wants ink preserved |
| Plain Group + no local TF | + | − | + | Rejected — cannot stretch/rotate box independently |
| Always scale ink with bounds only | + | − fixedInk | + | Rejected — loses text-box padding feel |
| Treat enclosure rect as Frame | 0 | − | 0 | Frames are root-only artboards — wrong metaphor |
