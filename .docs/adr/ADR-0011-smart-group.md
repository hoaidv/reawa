---
id: ADR-0011
title: Smart Group (ink-box) pilot
status: accepted
date: 2026-08-11
deciders: [architect, pm]
supersedes: null
amended_by: ADR-0013
---

# ADR-0011 — Smart Group (ink-box) pilot

> **Amended 2026-08-11 by [ADR-0013](./ADR-0013-ink-box-tool-modes.md).** §4A's propose/accept
> step is **withdrawn**: creation is tool-armed and immediate (undoable), and `recognize_enclose`
> is an internal Infini step rather than a wire op. §1–§3 (node shape, local transform,
> `inkScaleMode`), §4B, §5, and §6 otherwise stand. See the amendment table in ADR-0013.
>
> **Clarified 2026-08-11 (same day, PM UX):** §3 `fixedInk` uses **per-content-ink**
> `layoutOffset: {u,v}` (UV locked — not a local offset vector); §4B selection create
> **requires** a surround stroke among the selection (open OK — artificial closed path,
> even-odd PIP); §7 covers draw-into membership and free layout.
>
> **Confirmed 2026-08-11 (architect):** SRS-IN-15 / SRS-IN-16 + UV field shape are
> implementable as written; SM may slice.

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
   | `fixedInk` | Text-box chrome | Geometric bounds + **boundary ink** still transform; **each** content ink keeps sample size fixed and tracks the box via **its own** relative offset / UV |

   Exact `fixedInk` layout rule (pilot) — **per content ink**, field locked:

   - Store `layoutOffset: { u, v }` on each `role: content` ink (SRS-IN-09).  
     `u = (cx − bounds.x) / width`, `v = (cy − bounds.y) / height` of that ink’s AABB centroid
     in group-local space at seed (create / membership / selection-create).
   - On bounds or `scaleX`/`scaleY` change under `fixedInk`: **do not scale** content samples;
     place so the centroid sits at `(bounds.x + u·width, bounds.y + v·height)`, then apply
     rotation + group translate only.
   - Newly appended content ink receives **its own** `{u,v}` and never adjusts siblings.
   - Do **not** OCR, reflow glyphs, or auto-align content. In-box alignment is a later feature.
   - **Rejected alternatives:** local offset vector (second representation); shared group
     centroid; freeze-at-top-left with no UV.

   Example: ink A’s center at `(50, 100)` from the box top-left → `{u:0.25,v:0.5}` in a
   200×200 box; box width×2 → A’s center at `(100, 100)` in local UV space; ink B drawn later
   keeps its own UV and is untouched when A’s placement updates.

   **Boundary ink always transforms** with the SmartGroup (same as the geometric frame). It is
   **not** gated by `inkScaleMode`. Only `role: content` ink respects the mode.

4. **Creation paths (pilot)**  
   A. **Enclose recognition (tool-armed):** hand-draw a roughly rectangular stroke with
      `intent: enclose` → immediate `create_smart_group` when guards pass (see ADR-0013):
      - **Recognize** initial axis-aligned `bounds` from the enclose stroke.
      - **Preserve** the enclose stroke as **boundary ink**.
      - Reparent contained ink as `content`.
   B. **Explicit selection:** multi-select ink → “Smart Group” **only if** one selected stroke
      **surrounds almost all of the others** (≥80% of each other stroke’s samples inside that
      surround stroke’s region). The surround stroke may be **open**; build an **artificial
      closed path** for the containment test only (do not mutate the stored samples). That
      stroke → `role: boundary`; others → `role: content`; `bounds` = fitted rect of the
      surround stroke. **If no surround stroke qualifies → refuse create** (no AABB-only /
      hint-only Smart Group).

5. **Recognition quality (pilot bar)**  
   Enclosure detection is **best-effort**. False positives must be undoable. Misses fall back
   to explicit create. No requirement for perfect RM→geometry in v0 — measurable accept rate
   in SRS-quality.

6. **Persistence / ops**  
   `kind: smart_group` in tree + SVG `data-infini-kind="smart-group"` with bounds, transform,
   `inkScaleMode`, children. Ops: `create_smart_group`, `set_smart_group_transform`,
   `set_ink_scale_mode`, `recognize_enclose` (optional client-side).

7. **Draw-into membership (pilot)**  
   On `stroke_end` for ordinary ink (`intent: ink` / Pen — **not** an enclose stroke):

   - Candidate Smart Groups: those whose **world** geometric `bounds` contain ≥80% of the new
     stroke’s samples.
   - If none → ink stays where `append_ink` would put it (document root / ordinary parent).
   - If one or more → reparent the stroke as `role: content` under the candidate with the
     **highest paint / z order**. Paint order is tree sibling order (later siblings paint above);
     there is **no separate z-index field**. Nested Smart Groups that each contain 100% of the
     stroke resolve the same way (topmost / later sibling wins).
   - Membership **never** shifts, realigns, or reflows existing content ink — freehand placement.
   - Runs on Infini only (same authority as enclose recognition).

## Consequences

- Pilot can ship without OCR, text layout, or full Figma-like groups.  
- Viewport remains translate + uniform scale only; **node** rotation lives on SmartGroup.  
- Epaper may only draw ink; recognition/create/membership run on Infini after sync.  
- Connector glue uses SmartGroup world bounds after transform.  
- `fixedInk` tracks the box via **per-content-ink** `layoutOffset: {u,v}` (UV locked in
  SRS-IN-09). Current library `smartLocalToWorld` omits that UV — implement stories must close
  the gap when landing [SRS-IN-11](../modules/infini/features/vector-document/srs-logic.md#srs-in-11-selection-manipulation).  
- Selection create without a surround stroke is refused — no hint-only Smart Group.  
- Follow-up: promote local transforms to ordinary Group; richer enclose shapes; **in-box
  alignment**.

## Alternatives Considered

| Approach | Ink fidelity | Text-box feel | Sync complexity | Notes |
|---|---|---|---|---|
| **SmartGroup + local TF + inkScaleMode** | + | + | 0 | **Chosen** for pilot |
| OCR to Text node | − handwriting | + | 0 | Rejected — user wants ink preserved |
| Plain Group + no local TF | + | − | + | Rejected — cannot stretch/rotate box independently |
| Always scale ink with bounds only | + | − fixedInk | + | Rejected — loses text-box padding feel |
| `fixedInk` freeze samples (no offset) | + | − (content sticks to TL as box grows) | + | Rejected — PM: content must track box |
| `fixedInk` shared content centroid | + | − (new ink shifts older UV) | + | Rejected — PM: per-ink offset |
| `layoutOffset` as local (dx,dy) vector | + | + | 0 | Rejected — second representation; UV alone is enough and zoom-stable in bounds space |
| Selection → AABB Smart Group (no surround) | − frame | − | + | Rejected — need surround stroke among selection |
| Treat enclosure rect as Frame | 0 | − | 0 | Frames are root-only artboards — wrong metaphor |
| Auto-align / reflow on append | − freehand | + typed feel | 0 | Deferred — free layout in pilot |
