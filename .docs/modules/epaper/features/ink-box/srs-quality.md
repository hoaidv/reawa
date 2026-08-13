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
| Guard correctness — negative set (empty area, below 48 world units, drawn in `pen` mode) | Boxes created | **0** |
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
| `fixedInk` resize | Content sample size change | ≤1 px; each UV preserved ±1 world unit (**CHL-0004 / CHL-0005 regression**) |
| `fixedInk` resize | Boundary ink follows the frame | Always — `inkScaleMode` never governs boundary |
| `fixedInk` resize | Unrelated content inks displaced | **0** |
| `withBounds` resize | Content vs expected transform | ±1 world unit @ 100% zoom |
| Inverted resize | Negative-size states entering the document or the wire | **0** |
| Op granularity | Ops per completed gesture | Exactly **1** |
| Undo granularity | Undo entries per completed gesture | Exactly **1**; one undo reverts exactly that gesture |
| Feedback rate during a drag | Update rate / worst stall | ≥5 Hz / ≤200 ms |
| Refresh discipline during a drag | Full-panel invalidations | **0** |
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

## Superseded

New section. Replaces, for the device, the Smart Group budgets of infini
[srs-quality](../../../infini/features/vector-document/srs-quality.md) that assumed desktop
execution, and the round-trip enclose budget formerly in
[SRS-EP-06](../tool-modes/srs-quality.md).
