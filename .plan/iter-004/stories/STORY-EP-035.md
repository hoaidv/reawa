---
id: STORY-EP-035
title: Measure enclose area/length to separate handwriting from boundary ink
kind: implement
parent_srs: [SRS-EP-10]
parent_req: [REQ-05]
status: ready
priority: P1
iter: iter-005
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given a finished pen stroke, When dispatch logs enclose-why, Then it prints polyline length L, closed-ring shoelace area A, A/L, and A/L² (SRS-EP-10) without changing the enclose verdict."
  - "Given host polylines for a thin two-stroke word (his-like) and a fat closed outline (O / fat-W-like), When the same formulas run, Then A/L and A/L² are recorded in the test output so the bands can be compared."
  - "Given the human device corpus (handwriting words vs empty boundary shapes), When the [recog] lines are collected, Then .plan/iter-004/notes/ep-035-area-length.md lists each stroke's L, A, A/L, A/L² and class handwriting | boundary | acceptable-FP."
  - "Given the analysis, When A/L (or A/L²) separates his-like from fat-boundary, Then the notes file recommends a candidate bar; When it does not, Then the notes say so — this story does not ship a new enclose guard."
  - "Given this story, When it lands, Then enclose still uses existing closed-ish + size + content AABB rules — no containment / PIP change (human 2026-08-16)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-035 — Measure enclose area/length to separate handwriting from boundary ink

**Carried to iter-005** (human 2026-08-16): small enhancement beside the next feature wave. Not NOW until `/pm` picks that wave. File stays in iter-004/stories; `iter:` is iter-005.

Human 2026-08-16: handwriting `"his"` (two inks: `t` + `"his"`) became an ink-box. Do **not**
fix that with containment/PIP in this story. First question: does **created area / path length**
separate ordinary writing from empty **boundary-ink**?

Parent: [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) ·
[REQ-05](../../../.docs/modules/epaper/prd.md#device-ink-box). Empty-shape gate today
(`enclose_shape.hpp`) already exists for **empty** boxes; **with content** it is skipped — that
is how `"his"` + nearby `"t"` became a box. This story only **measures**; it does not retune
SRS-EP-10.

No design story — logging + corpus, no new UI.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Priority | **P1** — **iter-005 carry** — not NOW until the next wave is committed |

## Hypothesis

`"his"` (thin continuous glyph) has a different **area/length** than a fat empty boundary
(O, D, fat-W outline). If the bands do not overlap, a later story can add an A/L (or A/L²)
guard. If they overlap, drop this approach.

## Metric (lock the formula before capturing)

Treat the stroke as a **closed ring** (virtual edge last→first). Do **not** use AABB area as A.

| Symbol | Definition |
|---|---|
| `L` | Polyline length (same as dispatch `measureClosure` pathLen) |
| `A` | Absolute shoelace area of the ring (reuse `enclose_shape.hpp` `shoelace`) |
| `A/L` | Human-favored ratio (world length units) — compare **same-size** writing |
| `A/L²` | Dimensionless companion — compare small letters vs large O without rescaling |

Log both. Analysis picks which one (if either) separates the classes. Optional extra on the
same line, not the product claim: convex-hull area / L² (already in connector path-like).

## Acceptable vs unacceptable (product)

| Class | Examples | If recognized as empty-boundary / enclose |
|---|---|---|
| **Unacceptable FP** | Continuous `"his"` / thin handwriting words | Fail — must stay ink |
| **Acceptable FP** | Big **O, D, P, B, C, G**; **fat-C-like**, **fat-W-like** outlines | OK — letter-as-empty-box is allowed |

Fat-W-like: a **hollow calligraphic W** (wide interior, smooth outline) — human photo 2026-08-16.

## Human capture protocol (device)

`recog.ink_box` armed. Prefer **empty canvas** so content-AABB does not confound the log
(we are classifying the **stroke**, not what it captured). After each pen-up, copy the `[recog]`
line (needs `A=` `L=` `A/L=` `A/L2=`).

**A — handwriting (must stay ink)**

1. `"his"` as one continuous stroke (the FP case).
2. A few other small-to-size words the same way (`the`, `and`, `cat`, …).
3. Repeat at **small** and **medium** height (scale the writing, not the formula).

**B — empty boundary (real intent + acceptable letter-boxes)**

1. Hand-drawn empty box, circle, wiggle-box.
2. Empty **O, D, P, B, C, G**.
3. Empty **C-like** and **W-like** fat outlines.

Fill [ep-035-area-length.md](../notes/ep-035-area-length.md).

## Dev work (do not expand)

- Compute `A`, `L`, `A/L`, `A/L²` in enclose/dispatch; append to `[recog]` / `encloseWhy`.
- Host analog: his-like vs O vs fat-W-like polylines — print ratios, no threshold assert.
- `@implements [SRS-EP-10] enclose area/length probe` at the helper.
- **Do not** change closed-ish, size bars, content AABB, or the enclose verdict.

## Out of scope

Containment / even-odd PIP / concave-hull / no-self-intersect. Shipping a new SRS-EP-10 guard
(that is a follow-up after the table). EP-033 ingest. Connector `hullAreaOverLen2` retune.

## Follow-up (not this story)

If bands separate: PM/architect set a bar (likely A/L² so scale does not matter), then a
guard story. If `"his"` sits in the fat-W band: say so and pick another discriminator.
