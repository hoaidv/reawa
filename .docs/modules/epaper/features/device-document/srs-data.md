---
feature: device-document
parent_req: [REQ-04, REQ-07]
version: 0.2.0
lifecycle: active
---

# SRS — On-device working document (Data)

Architect-owned device data shapes for [REQ-04](../../prd.md#device-document) and
[REQ-07](../../prd.md#one-way-sync). Logic: [SRS-EP-07 / SRS-EP-08](./srs-logic.md).

**This file does not define the wire grammar.** Node schemas, the SVG persistence profile, the op
envelope, `doc_change`, and `doc_load` are canonical in
[infini SRS-IN-09](../../../infini/features/vector-document/srs-data.md), because the desktop owns
the file and a second grammar would be a second file format. This file binds the device to that
grammar and specifies what is **device-local** — the structures that never cross the wire.

---

## [SRS-EP-09] Device document data and wire binding {#srs-ep-09-device-data}

### Canonical, by reference — do not fork

| Shape | Canonical source |
|---|---|
| Node kinds, roles, `inkScaleMode`, `layoutOffset { u, v }`, anchors | [SRS-IN-09](../../../infini/features/vector-document/srs-data.md) + [domain/vector-document](../../../../domain/vector-document.md) |
| Op envelope `{ opId, type, payload, ts?, source? }` | [SRS-IN-09](../../../infini/features/vector-document/srs-data.md) Transmit ops |
| `doc_change { type, seq, opId, op, baseSeq }` | [SRS-IN-09](../../../infini/features/vector-document/srs-data.md) · [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md) §2 |
| `doc_load { type, document, seq }` | [SRS-IN-09](../../../infini/features/vector-document/srs-data.md) · ADR-0015 §4 |
| `viewport`, `stroke_begin` / `stroke_point` / `stroke_end` | [SRS-IN-07](../../../infini/features/tablet-sync/srs-logic.md) — payload shape unchanged; **direction follow-gated** ([ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md)). Up `source: epaper` only while Infini follow is on ([SRS-EP-24](../region-sync/srs-logic.md)) |
| `viewport_follow` | Session `{ direction, seq }` — [SRS-EP-49](../region-sync/srs-logic.md#srs-ep-49-viewport-follow). Not document |
| Handshake `hello` / `drain_ack` / `queue_empty` / `load_ack` | ADR-0015 §7 |
| `pen_capability` | Optional HID telemetry T→D — not document, not persist ([ADR-0031](../../../../adr/ADR-0031-device-settings-persist-on-epaper.md)). **`pen_button_map` withdrawn** (0 persist-up, 0 restore-down) |

A device change to any row above is a change to the shared grammar and belongs in SRS-IN-09, with
this section following. **The device must not add fields the desktop cannot parse** — an unknown
field arriving on the desktop is a mirror-suspect condition, not a graceful extension.

### Device-local structures (never on the wire)

| Structure | Shape | Notes |
|---|---|---|
| `DeviceDocument` | Tree per the domain doc, world coordinates | In-memory only; no file form on device |
| Undo ring entry | `{ forwardOpId, seq, inverses, targets: [{ nodeId, prevLastOpId }] }` × depth 20 | Inverse of the committed gesture, not a whole-tree snapshot. See [SRS-EP-07](./srs-logic.md#srs-ep-07-device-document). `prevLastOpId` is the node’s last **forward** id before this gesture. Undo restores it; redo restamps `forwardOpId`. Publish may use `undo:N` / `redo:N` as wire `opId` only. `inverses` are ordinary tree ops with absolute pre-op values |
| Publish queue entry | `{ seq, baseSeq, opId, op, committedAt }` | Ordered; drains on `drain_ack` |
| Session epoch | `{ epochId, lastSeq, queuedCount, loadState }` | Reset on an accepted `doc_load` |
| Selection | `{ nodeId?, handle? }` | UI state, not document state ([SRS-EP-11](../ink-box/srs-logic.md)) |
| Tool mode | `pen \| ink_box \| selection` | Device-local, never transmitted ([ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) §1) |
| Opaque carry-through | Raw form of node kinds the device does not author | Preserved verbatim from `doc_load`, re-emitted unchanged |
| Clipboard slot | `{ nodes: Node[], sourceIds: string[] } \| empty` | **Process-global singleton** — not on `DeviceDocument`, never on the wire ([ADR-0037](../../../../adr/ADR-0037-device-clipboard-singleton.md), [SRS-EP-31](./srs-logic.md#srs-ep-31-clipboard)) |
| Pen-button map (live + durable) | Domain [pen-button-map](../../../../domain/pen-button-map.md) | **Authored and persisted on this device** ([REQ-20](../../prd.md#device-settings), [SRS-EP-53](../tool-modes/srs-logic.md#srs-ep-53-pen-map-author)); survives Epaper restart; **never** Infini app settings; **never** SVG / VectorDocument; **0** `pen_button_map` on the wire |
| Viewport follow | `none` \| `infini_to_epaper` \| `epaper_to_infini` | [domain/viewport-follow](../../../../domain/viewport-follow.md) — session, not document. Last-writer token withdrawn |

### Ink sample retention

The wire preview is lossy by design; the **node is not**.

| Channel | Preview (`stroke_point`) | Document node |
|---|---|---|
| `x`, `y` | Panel coords | **World** coords |
| `pressure` | Sent | Stored |
| `tilt`, `distance`, `timestamp`, proximity/button flags | May be omitted | **Stored when the digitizer reports them** |
| Unmodelled channels | Not sent | `extras: { [channel]: number \| bool \| string }` |

Rationale: samples are the only irreplaceable data in the system. A preview can be regenerated from
the node; a node cannot be regenerated from a preview. Store first, transmit second.

Ink `style.strokeWidth` is in **world units**
([ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)); panel width is derived at
paint, never stored.

### Numeric agreement

Two implementations of one geometry ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §6)
agree only if their number handling agrees.

| Rule | Value |
|---|---|
| Coordinate precision on the wire | Round-trip safe for `double`; the device must not narrow to `float` before publishing |
| `bounds` normalization | Non-negative `width` / `height`; an inverted resize normalizes **before** commit |
| Comparison tolerance | ±1 world unit at 100% zoom for parity assertions ([SRS-EP-13](./srs-quality.md)) |
| Containment threshold | ≥80% of samples — computed on stored samples, not on a resampled preview |
| Min enclose size | 48 **world** units ([ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) §6, kept by ADR-0014 §7) |

### Shared fixtures (the divergence guard)

`features/vector-document/fixtures/ops/` is the shared corpus named in ADR-0014 §6. Both peers run
it; disagreement is a `CHL-*`, not a tolerance to widen.

| Fixture set | Asserts |
|---|---|
| `ops/` op → tree | Same tree from the same op sequence, device and desktop |
| `enclose/` | Same guard verdict and same fitted `bounds` for the same stroke |
| `fixed-ink/` | Same content placement under `fixedInk` resize (CHL-0004 / CHL-0005 regression) |
| `round-trip/` | Device-authored document → `doc_change` stream → desktop mirror → save → `doc_load` → device tree, unchanged |

The `round-trip/` set is the one that proves the whole contract: it is the only test that exercises
both directions of the one-way design in sequence.

---

## Superseded

New section. The wire shapes it binds to were extended in
[SRS-IN-09](../../../infini/features/vector-document/srs-data.md) on 2026-08-13 (CHL-0008); the
retired `doc_snapshot` / `pickables` shapes are listed there.
