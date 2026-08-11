---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.2.0
lifecycle: active
---

# SRS — Tablet sync Infini (Logic)

Binds Infini to the shared session in [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md).
Document tree + ops: [vector-document SRS-IN-04/09](../vector-document/srs-logic.md) ·
[ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) · [ADR-0011](../../../../adr/ADR-0011-smart-group.md).

## [SRS-IN-07] Session roles and channel binding

### Endpoint(s)

Local network session to RM2 (USB Ethernet). Framing baseline: **JSON-lines** messages
(EXP-0001 spike). Auth: none in v0 (trusted local link).

### Channels

| Channel | Owner (writer) | Consumer | Priority |
|---|---|---|---|
| **Viewport** | Infini | Epaper | High — apply before next pen sample |
| **Document** | Both (see emit matrix) | Both | Normal — ordered op-log |

### Viewport message (Infini → Epaper)

| Field | Required | Meaning |
|---|---|---|
| `type` | yes | `viewport` |
| `translate` | yes | `{ x, y }` world |
| `scale` | yes | uniform > 0 |
| `drawingRegion` | yes | AABB `{ minX, minY, maxX, maxY }` of visible world (or derived) |
| `seq` | yes | monotonic |

Infini publishes on pan/zoom (may coalesce frames; **must not** delay map apply beyond
[SRS-IN-08](./srs-quality.md) budget on Epaper).

### Document op envelope (either direction)

Matches [SRS-IN-09](../vector-document/srs-data.md):

```text
{ opId, type, payload, ts?, source: "epaper"|"infini" }
```

Apply is **idempotent** on `opId`. Peers maintain the same materialised tree.

### Emit matrix (v0 sync wave)

| Op `type` | Epaper may emit | Infini may emit | Notes |
|---|---|---|---|
| `append_ink` | **yes** (primary) | later (desktop ink) | Samples in **world** space after current viewport map; full tablet channels |
| `insert_node` / `create_*` structure | no (v0) | yes (when desktop edits) | Groups, frames, text, primitives, connectors |
| `create_smart_group` / enclose | no (v0) | **yes** (Infini-first pilot) | Epaper keeps emitting raw ink; Infini may promote |
| `set_smart_transform` / `set_ink_scale_mode` | no | yes | |
| `reparent` / `remove_node` | no | yes | |
| Snapshot / hello | optional | optional | Reconnect catch-up TBD |

**v0 editing rule:** While Epaper is the active pen editor, Infini is **viewer + viewport
owner** for ink capture — it applies incoming `append_ink` and does not fight Epaper on the
same stroke. Smart Group / structure edits on Infini are allowed when not racing an in-flight
stroke (product: pause or queue).

### Response fields that drive UI

| Field | Required | Drives UI |
|---|---|---|
| `session.connected` | yes | optional status (chrome later) |
| `doc` tree | yes | WorldLayer projection ([SRS-IN-01](../infinity-canvas/srs-logic.md)) |
| `viewport` | yes | canvas transform |

### Closed ids (message types)

| id | Channel | Direction |
|---|---|---|
| `viewport` | viewport | Infini → Epaper |
| `doc_op` | document | bidirectional |
| `doc_ack` | document | optional ack of `opId` |
| `hello` / `snapshot` | document | optional reconnect |

### Errors / partial failure

| Case | Behavior |
|---|---|
| Duplicate `opId` | Ignore (idempotent) |
| Unknown op type | Log; do not crash; optional NACK |
| Gap in seq / missing op | Request snapshot or mark session degraded (CHL if product needs hard fail) |
| Apply fails validation (e.g. bad parentId) | Reject op; keep prior tree; surface debug |

### Other logic

- Drawing-region AABB = inverse of Infini viewport over CSS window ([SRS-IN-01](../infinity-canvas/srs-logic.md)).
- Render path: apply doc ops → flatten tree (incl. SmartGroup transforms) → spatial cull → paint.
- **Implement deferred** to W4 with Epaper region-sync (SM board) — this SRS is the bind contract.

---

## Superseded

_None._ Content thickened 2026-08-11 for sync readiness.
