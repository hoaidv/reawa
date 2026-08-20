---
feature: ink-box
parent_req: [REQ-05, REQ-06]
version: 0.1.0
lifecycle: active
---

# SRS — Ink-box on the device (Quality)

Parent REQs: [REQ-05](../../prd.md#device-ink-box), [REQ-06](../../prd.md#device-manipulation).
Logic: [SRS-EP-10 / SRS-EP-11](./srs-logic.md). Document budgets:
[SRS-EP-13](../device-document/srs-quality.md).

## [SRS-EP-14] Recognition, manipulation, and CHL regression bars {#srs-ep-14-ink-box-quality}

### Creation

| Scenario | Metric | Target |
|---|---|---|
| Enclose → box visible | p95 after **pen-up** | ≤500 ms |
| Peer involvement in a create | Messages required from the desktop | **0** |
| Guard correctness — negative set (below 28 with content, below 36 empty, non-primitive empty, recog off) | Boxes created | **0** |
| Guard correctness — empty closed primitive ≥ 36 | Boxes created | **1** (boundary only) |
| Guard correctness — positive set | Boxes created | 1 per gesture, `bounds` matching the fitted rect ±1 world unit |
| Boundary ink | Creates whose box has the creator's own stroke as `role: boundary` | **100%** |
| Synthetic rectangles introduced | Count | **0** |
| Draw-into membership | p95 pen-up → reparented | ≤300 ms |
| Membership side effects | Existing content inks moved | **0** |
| Nested candidates | Dual-parented ink | **0**; highest paint order wins every time |
| Selection-create refusal | Boxes created without a qualifying surround | **0**, reason visible |
| Consecutive encloses (10 in sequence) | Correct boxes | 10 / 10 — **0** desync, **0** lost boxes (**CHL-0007 regression**) |

### Manipulation

| Scenario | Metric | Target |
|---|---|---|
| Selection feel | p95 pen-down → selection affordance | ≤100 ms |
| Commit fidelity | Committed geometry vs last previewed geometry, 20 scripted gestures | **0 px jump, 0 snap-backs** (**CHL-0006 / CHL-0007 regression**) |
| Ghost artifacts | Advisory outlines that move independently of the ink | **0** — the model no longer has one |
| Live node during move/resize | Where the moving pixels are painted | **ToolCanvasLayer** ([CHL-0018](../../../../../.plan/iter-003/challenges/CHL-0018-live-node-tool-canvas.md)); CanvasLayer origin hole only — not origin∪trail |
| Live node during move/resize | Duplicate box on CanvasLayer while overlay shows the live copy | **0** |
| Pen-up after move/resize | Settled copies of that box | **1** on CanvasLayer; ToolCanvasLayer live copy gone |
| Mid-gesture e-ink ghosting / dirty traces | Product fail? | **No** — refresh allowance; settled-frame mismatch **is** a fail (BR-B15) |
| `fixedInk` resize | Content sample size change | ≤1 px; each UV preserved ±1 world unit (**CHL-0004 / CHL-0005 regression**) |
| `fixedInk` resize | Boundary ink follows the frame | Always — `inkScaleMode` never governs boundary |
| `fixedInk` resize | Unrelated content inks displaced | **0** |
| `withBounds` resize | Content vs expected transform | ±1 world unit @ 100% zoom |
| Inverted resize | Negative-size states entering the document or the wire | **0** |
| Op granularity | Ops per completed gesture | Exactly **1** |
| Undo granularity | Undo entries per completed gesture | Exactly **1**; one undo reverts exactly that gesture |
| Feedback rate during a drag | Update rate / worst stall | ≥5 Hz / ≤200 ms |
| Refresh discipline during a drag | Full-panel invalidations | **0** |
| Lasso / marquee live stroke (`sel.lasso` / `sel.marquee`) | Full-panel invalidations; CanvasLayer `update()` with no rect | **0** ([CHL-0017](../../../../../.plan/iter-003/challenges/CHL-0017-selection-chrome-layers.md) / [ADR-0019](../../../../adr/ADR-0019-selection-chrome-layers.md)) |
| Lasso / marquee live stroke | Damage | ToolCanvasLayer only — last segment or old∪new AABB; waveform **Mono** |
| Deselect | Residual selection pixels on the next settled frame | **0** (**CHL-0007 regression**) |
| Below the LOD cutoff | Accidental transforms | **0**; unavailability stated |

### Offline parity

| Scenario | Metric | Target |
|---|---|---|
| Scripted 10-gesture create + manipulate set, link down vs up | Resulting documents | **Identical** — 100% parity |
| Ink-box or Selection unavailable with the link down | Count | **0** |
| Gesture started before a drop, released after | Ops lost | **0**; the change queues |

### Forward-compatibility conformance ([REQ-06](../../prd.md#device-manipulation) ↔ [REQ-08](../../prd.md#node-manipulation))

| Scenario | Metric | Target |
|---|---|---|
| Capability descriptor | `SmartGroup`'s declared verbs | Exactly `{select, move, resize, set-ink-scale-mode}` |
| Gesture routing | Node-kind branches in the router (`if SmartGroup`) | **0** — dispatch goes through the descriptor |
| Transform envelope | Ops validating against the shared envelope with `rotation` unset | **100%**, **0** bespoke op shapes |
| Bounds provider | Selection geometry computed by a node-kind-agnostic provider | Always |

A green suite with a hard-coded SmartGroup branch is a **failed** conformance run. This row exists
because that shortcut is cheap, invisible in the product, and would silently cost the next iteration
its whole premise.

### Cross-implementation agreement

| Scenario | Metric | Target |
|---|---|---|
| Shared fixtures `enclose/` | Device vs desktop guard verdict and fitted bounds | **100% agreement** |
| Shared fixtures `fixed-ink/` | Device vs desktop content placement | **100% agreement** |
| Divergence found | Handling | File a `CHL-*`; do not widen the tolerance ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §6) |

### Notes

- Four of these rows are named after challenges. They stay named that way: CHL-0004…CHL-0007 are
  retained as regression criteria, not as history
  ([CHL-0008](../../../../../.plan/iter-003/challenges/CHL-0008-architecture-rework.md) resolution).
- The **≤500 ms** create budget is unchanged in number from the pilot and completely different in
  meaning: it was a round trip, it is now local work after pen-up. If it is missed now, the fix is
  device-side optimization, not a faster link.
- Slow is acceptable; wrong is not. Every "0" in the manipulation table is a correctness bar, and
  every latency row is a comfort bar. When they conflict, correctness wins.

---

## [SRS-EP-25] One-finger hand-touch quality {#srs-ep-25-one-finger-quality}

<!-- lifecycle: active -->

**Parent:** [REQ-10](../../prd.md#hand-touch). **Constrains:** [SRS-EP-21](./srs-logic.md#srs-ep-21-one-finger), [SRS-EP-23](../tool-modes/srs-logic.md#srs-ep-23-finger-tool-switch). Does **not** steal [SRS-EP-14](#srs-ep-14-ink-box-quality) parent (REQ-05/06).

| Field | Value |
|---|---|
| Source | Creator finger |
| Stimulus | Down on box / drag / down on knob / empty canvas (≤10 mm vs >10 mm) |
| Artifact | Exclusive tool, selection, box transform, viewport |
| Environment | Normal; link up or down |
| Response | Tool switch, live move, live resize, palm no-op, or local pan |
| Response measure | See table |

| Scenario | Target |
|---|---|
| Finger-down on box → `sel_freeform` + chip + selected | p95 ≤**300 ms** |
| Finger move inside selected box | 0 px jump on lift; ≥**5 Hz** partial refresh; **0** viewport pan |
| Finger on resize knob | same live-direct bar as pen resize; **0** viewport pan |
| One-finger empty, travel ≤ **10 mm** (89 du @ 226 dpi) | Exclusive tool unchanged; **0** nodes selected; **0** lassos; **0** pans |
| One-finger empty, travel > **10 mm** | **Local** pan; tool unchanged; **0** selection; **0** lasso; Infini matches **only if** Infini follow on |
| Box / knob / chip hit vs would-be empty pan | Hit wins; **0** empty-canvas pan |
| ToolChip 64 du tile tap | REQ-03 still holds (this REQ does not steal) |

---

## Superseded

New section. Replaces, for the device, the Smart Group budgets of infini
[srs-quality](../../../infini/features/vector-document/srs-quality.md) that assumed desktop
execution, and the round-trip enclose budget formerly in
[SRS-EP-06](../tool-modes/srs-quality.md).
SRS-EP-25 is additive (REQ-10); it does not supersede SRS-EP-14.
