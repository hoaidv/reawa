---
feature: vector-document
parent_req: [REQ-02, REQ-04]
version: 0.4.0
lifecycle: active
---

# SRS — Vector document (Quality)

## [SRS-IN-06] Fidelity, structure, and dual-ask

<!-- revised: 2026-08-13 — CHL-0008 / ADR-0014. Desktop authoring scenarios move with SRS-IN-11/14;
     fidelity and round-trip stay; mirror-replay rows added. Same id, content revised. -->

> **Revised 2026-08-13.** The desktop no longer creates or manipulates Smart Groups, so the
> interaction rows (enclose, membership, pick-vs-pan, LOD guard, gesture economy) move to
> [epaper SRS-EP-14](../../../epaper/features/ink-box/srs-quality.md). What stays here is what the
> desktop still owns: **fidelity, round-trip, and faithful replay of the device's change stream**.
> The desktop must still *render* every Smart Group correctly — it just does not author one.

### Quality-attribute scenarios

| Scenario | Metric | Target |
|---|---|---|
| Save SVG → reopen (mixed tree) | Vertex / box / anchor error @ 100% zoom | ≤ 1 CSS px |
| Ink channel fidelity | pressure/tilt/extras present after reopen/transmit | 100% of channels that were on the fixture samples |
| Transmit encode → decode | Op equality on golden fixture set | 100% |
| Parenting preserved | Node `id` + parent/child relations | Exact match after round-trip |
| Connector endpoints | Resolved `from`/`to` node ids after reopen | 100% on valid fixtures |
| Preferred ports | Rect edge midpoints / ellipse cardinals round-trip as `port` | Exact enum match |
| Free boundary | Edge `t` / circumference angle round-trip | Param error ≤ 1e-6; world ≤ 1 px @ 100% |
| Glue on move | After translate/resize target, resolved anchor stays on boundary | Always |
| Smart Group round-trip | bounds, transform, inkScaleMode, ink samples | Exact + ≤1 px geom |
| `fixedInk` vs `withBounds` | After non-uniform scale: content ink fixed vs scaled; **boundary ink always scales** | Mode-correct |
| `fixedInk` per-ink UV | Two content inks with distinct UVs; after scale `s`, each UV preserved (±1 px @ 100%); sample sizes unchanged; no cross-ink move | Always |
| Boundary ink transform | After rotate/scale (any inkScaleMode), boundary ink samples transform with group | Always |
| Smart Group render fidelity | Device-authored group painted from the mirror | Matches the device panel figure (±1 px @ 100%) |
| **Mirror replay** | Apply the full change stream of a session from an empty mirror | Tree equals the device's final tree (0 divergent nodes) |
| **Replay idempotency** | Re-apply every `opId` in the stream a second time | Tree unchanged |
| **Out-of-order rejection** | A `doc_change` whose `baseSeq` skips | Mirror marked suspect; resync requested; **0** silent saves |
| **Preview isolation** | Preview strokes during a session | **0** written to the mirror or to disk |
| **Restore-snapshot apply** | Device undo published as `restore_snapshot` | Mirror equals the device's restored tree |
| Invalid connector apply | No crash; connector marked invalid | Always |
| Open failure | Prior tree bytes unchanged in memory | Always |
| Large ink reopen | 10k-point ink polyline open | ≤ 2 s cold open on reference Mac (advisory) |

Moved to [epaper SRS-EP-14](../../../epaper/features/ink-box/srs-quality.md) and
[SRS-EP-13](../../../epaper/features/device-document/srs-quality.md): enclose happy path, guards,
first-try rate and latency, selection-create, draw-into membership and tie-break, undo exactness /
depth / memory, pick correctness, pick-vs-pan, LOD guard, gesture op economy. They are not weakened
by the move — several got stricter, because the device can now be held to "0 jump" rather than "one
snapshot to converge".

`fixedInk`, boundary-transform, and Smart Group round-trip rows **stay**: the desktop must reproduce
device geometry exactly to save it, which makes this file half of the cross-implementation agreement
check ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §6).

### Correctness ties

- Flatten visitor emits every Ink/Text/Primitive/Connector drawable exactly once per paint
  (no duplicate leaves from group walks).
- Frames at non-root rejected on load (error or skip-with-error — **fail closed**).
- The mirror is never saved while marked suspect
  ([ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) §3).

### Dual-ask table (state → Designer + QA)

| State id | Designer scene / Spec | QA AC / BDD |
|---|---|---|
| `doc.none` | STORY-IN-006 package | open/save feature |
| `doc.open` | STORY-IN-006 | open/save + tree fixture visible |
| `doc.dirty` | STORY-IN-006 | dirty indicator |
| `doc.error` | STORY-IN-006 | error + canvas unchanged |
| tree round-trip | N/A (logic) | SRS-IN-06 scenarios / fixtures |
| mirror replay + idempotency | N/A (logic) | SRS-IN-06 replay rows · [SRS-IN-07](../tablet-sync/srs-logic.md) |
| mirror suspect (sequence gap) | DocChrome — needs a visible state | SRS-IN-06 out-of-order row |
| smart_group rendered from the mirror | N/A (paint) | render fidelity row |

The `ink-box design package` rows are **removed** with
[SRS-IN-14](./srs-ui.md#srs-in-14-ink-box-ui): there is no desktop authoring surface to design. The
device equivalents are in [epaper SRS-EP-12](../../../epaper/features/ink-box/srs-ui.md).

### A11y / resilience

- DocChrome controls keyboard-focusable.
- Error text not color-only (icon or prefix “Error:”).
