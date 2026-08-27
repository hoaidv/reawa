---
feature: vector-document
parent_req: [REQ-02]
version: 0.4.0
lifecycle: active
---

# SRS — Vector document (Data)

## [SRS-IN-09] Persistence and transmit schemas

<!-- revised: 2026-08-13 — CHL-0008 / ADR-0015. Added the doc_change envelope and the load shape;
     ownership table re-stated for one-writer sessions. Same id, content revised.
     revised: 2026-08-27 — ADR-0032. `compound` + `set_ink_samples`; undo is not wholesale replace. -->

**Logic:** [SRS-IN-04](./srs-logic.md). **ADR:** [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md),
[ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md).
**Shared semantics:** [domain/vector-document](../../../../domain/vector-document.md).

Schemas here are the **shared** ones — the device implements the same shapes in C++. Do not fork a
device-side dialect; extend this file.

### Ownership

| Data | Owner at rest | Owner live (in-session) |
|---|---|---|
| Tree snapshot | SVG file on disk (**Infini**) | **Epaper** working document; Infini mirror follows |
| Change stream | N/A (ephemeral; optional journal later) | Emitted by **Epaper**, applied by Infini |
| Full load | Serialized from the Infini mirror | Sent **once** per session epoch, handshake-gated |

### Node JSON shape (canonical in-memory / fixtures)

```json
{
  "version": 1,
  "rootChildren": [
    {
      "id": "frm_1",
      "kind": "frame",
      "bounds": { "minX": 0, "minY": 0, "maxX": 800, "maxY": 600 },
      "children": [
        {
          "id": "ink_1",
          "kind": "ink",
          "samples": [
            {
              "x": 10,
              "y": 20,
              "pressure": 0.42,
              "tiltX": 0.1,
              "tiltY": -0.05,
              "t": 0
            },
            {
              "x": 12,
              "y": 24,
              "pressure": 0.55,
              "tiltX": 0.12,
              "tiltY": -0.04,
              "t": 16,
              "extras": { "distance": 0 }
            }
          ],
          "style": { "stroke": "#1C2430", "strokeWidth": 2 }
        }
      ]
    },
    {
      "id": "grp_1",
      "kind": "group",
      "children": []
    },
    {
      "id": "conn_1",
      "kind": "connector",
      "from": { "nodeId": "rect_1", "port": "east" },
      "to": { "nodeId": "ell_1", "port": "left" }
    },
    {
      "id": "conn_2",
      "kind": "connector",
      "from": {
        "nodeId": "rect_1",
        "boundary": { "edge": "south", "t": 0.25 }
      },
      "to": {
        "nodeId": "ell_1",
        "boundary": { "angle": 0.785 }
      }
    }
  ]
}
```

Kinds: `ink` | `text` | `primitive` | `group` | `smart_group` | `frame` | `connector`.

### SmartGroup schema (pilot)

```json
{
  "id": "sg_1",
  "kind": "smart_group",
  "bounds": { "x": 0, "y": 0, "width": 200, "height": 80 },
  "transform": { "x": 100, "y": 50, "rotation": 0, "scaleX": 1, "scaleY": 1 },
  "inkScaleMode": "fixedInk",
  "children": [
    {
      "id": "ink_content_1",
      "kind": "ink",
      "role": "content",
      "layoutOffset": { "u": 0.25, "v": 0.5 },
      "samples": []
    },
    {
      "id": "ink_boundary_1",
      "kind": "ink",
      "role": "boundary",
      "samples": []
    }
  ]
}
```

| Field | Meaning |
|---|---|
| `bounds` | Recognized geometric box `(x,y,width,height)` for handles / connectors |
| `transform` | Local→world: translate, rotation (rad), scaleX/Y |
| `inkScaleMode` | `withBounds` \| `fixedInk` — applies to **content** ink only; boundary ink always transforms |
| `children[].role` | `content` (handwriting inside) \| `boundary` (preserved enclose / surround stroke) |
| `children[].layoutOffset` | **Content only (pilot) — locked as UV.** `{ u, v }` in unit interval relative to `SmartGroup.bounds`: `u = (cx − bounds.x) / bounds.width`, `v = (cy − bounds.y) / bounds.height`, where `(cx, cy)` is that ink’s AABB centroid in **group-local** space at seed time. Required on every `role: content` child; **omit / ignore** on `boundary`. Round-trip in JSON + SVG `data-infini-layout-offset="u,v"`. Do **not** store a local offset vector instead — UV is the single representation. |

### InkSample schema

| Field | Required | Meaning |
|---|---|---|
| `x`, `y` | yes | World position |
| `pressure` | no | 0…1 (or device-normalized) when reported |
| `tiltX`, `tiltY` | no | Tilt when reported |
| `t` / `timestamp` | no | Sample time (ms or device clock) |
| `distance`, proximity, buttons | no | When reported by tablet pipeline |
| `extras` | no | Map of additional ink channels — **preserve on round-trip** |

Geometry for bounds/cull uses `x,y` only; other channels ride along for brushes / future render.

### Anchor schema

| Field | Meaning |
|---|---|
| `nodeId` | Target node |
| `port` | Preferred snap: rect/AABB `north\|east\|south\|west`; ellipse `top\|right\|bottom\|left`; line `start\|end\|mid` |
| `boundary.edge` + `boundary.t` | Free point on rect/AABB edge (`t` 0→1 along edge) |
| `boundary.angle` or `boundary.t` | Free point on ellipse circumference |
| `boundary.t` (line) | Free point along line segment |

Exactly one of `port` | `boundary`. World position is **derived**, not stored as absolute x/y
(so movers keep connectors glued).

### SVG profile (v0 direction)

| Tree | SVG |
|---|---|
| Ink | `<polyline>`/`<path>` for `x,y` + `data-infini-kind="ink"` + id; per-sample channels in companion `data-infini-samples` JSON (or sidecar) so pressure/tilt/… survive |
| Text | `<text>` / `<foreignObject>` TBD — prefer `<text>` tspans; `data-infini-kind="text"` |
| Primitive line/rect/ellipse | matching SVG element + id attrs |
| Group | `<g data-infini-kind="group" data-infini-id="…">` |
| Frame | `<g data-infini-kind="frame" data-infini-id="…" data-infini-bounds="minX,minY,maxX,maxY">` |
| SmartGroup | `<g data-infini-kind="smart-group" … data-infini-bounds data-infini-transform data-infini-ink-scale-mode>`; content ink children carry `data-infini-role="content"` + `data-infini-layout-offset="u,v"`; boundary ink carries `data-infini-role="boundary"` |
| Connector | `<path data-infini-kind="connector" data-infini-from="id" data-infini-to="id" data-infini-from-port="east" data-infini-to-boundary="…">` |

Root `<svg>` documents Infini profile version: `data-infini-doc-version="1"`.

Exact attribute grammar may tighten in an appendix without changing ADR-0010.

### Transmit ops (illustrative)

| `type` | Payload (min) |
|---|---|
| `append_ink` | `{ id, parentId?, samples, style }` |
| `insert_node` | `{ node, parentId? }` |
| `reparent` | `{ id, newParentId, index }` |
| `remove_node` | `{ id }` |
| `set_text` | `{ id, runs }` |
| `create_connector` | `{ id, from, to, warpStyle, body?, restShape? }` — `from`/`to` carry `kind` (`edge`\|`centre`), `nodeId`, `edge`, `t`, `drawnEdgeLocal`; missing node uses last live pose ([ADR-0020](../../../../adr/ADR-0020-connector-ink-geometry.md)). Geometry is **derived** on apply; never streamed as samples. |
| `create_smart_group` | `{ id, bounds, transform?, inkScaleMode?, children: InkNode[] }` (code) — older fixtures may use `childIds` |
| `set_smart_transform` | `{ id, transform }` |
| `set_ink_scale_mode` | `{ id, inkScaleMode }` |
| `set_connector_end_style` | `{ connectorId, end: "start"\|"finish", style }` — closed styles in [domain/vector-document](../../../../domain/vector-document.md) |
| `bind_endpoint_ink` | `{ connectorId, end, samples }` — rest spine not rebaked ([ADR-0026](../../../../adr/ADR-0026-endpoint-ink-membership.md)) |
| `bind_attachment` | `{ connectorId, nodeId, t, d }` — `t` on rest spine S ([ADR-0027](../../../../adr/ADR-0027-attachment-t-rest-spine.md)) |
| `duplicate_subtree` | `{ nodes: Node[], dxy: {x: 24, y: 24} }` ([ADR-0024](../../../../adr/ADR-0024-in-document-clipboard.md)) |
| `create_frame` | `{ id, bounds }` root Frame |
| `create_primitive` | `{ id, kind: "line"\|"rect"\|"ellipse", geometry }` — not a polyline stand-in |
| `set_ink_samples` | `{ id, samples }` — Path A erase and its inverse ([ADR-0032](../../../../adr/ADR-0032-inverse-op-undo.md)) |
| `compound` | `{ ops: [ … ] }` — atomic multi-op gesture (undo/redo of N inverses). Unknown type still marks the mirror **suspect** |
| `restore_snapshot` | `{ document }` — **last-resort, non-undo** wholesale replace (tests / emergency). Undo is **not** wholesale replace |

Op envelope: `{ opId, type, payload, ts?, source? }`. Apply is idempotent on `opId`.

<!-- lifecycle: retired -->
<!-- superseded-by: [ADR-0032] -->
<!-- note: 2026-08-27 — `restore_snapshot` as how device undo publishes is retired. Counterpart / compound is the undo path. -->

### Document-change envelope (Epaper → Infini)

```text
{ "type": "doc_change", "seq": n, "opId": "<uuid>", "op": { … op envelope … }, "baseSeq": n-1 }
```

| Field | Rule |
|---|---|
| `seq` | Monotonic per session epoch, assigned by the device |
| `opId` | Idempotency key — a duplicate apply is a no-op |
| `baseSeq` | The `seq` the device believed current at commit; a mismatch on the receiver is a **gap** |
| Ordering | Strict `seq`; the receiver must not reorder or interpolate |
| Gap | Mirror marked **suspect**; request explicit resync; do not save a suspect mirror |

### Document-load envelope (Infini → Epaper)

```text
{ "type": "doc_load", "document": { … full tree … }, "seq": 0 }
```

Replaces `doc_snapshot` and its `pickables` array (both retired with
[SRS-IN-13](../tablet-sync/srs-logic.md#srs-in-13-tool-intent-transport)). Legal only after the
device's change queue has drained.

**Sync bind:** the device emits ops, the desktop applies them — see
[tablet-sync SRS-IN-07](../tablet-sync/srs-logic.md) and
[epaper SRS-EP-08](../../../epaper/features/device-document/srs-logic.md).

### Fixtures

Golden trees for QA live under feature `bdd/` or `infini/tests/fixtures/` when implement starts
(must include ink + group + frame + text + primitive + connector).
