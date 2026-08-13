---
feature: tool-modes
parent_req: [REQ-03]
version: 0.2.0
lifecycle: active
---

# SRS — Tool modes Epaper (Logic)

Device-side rules for [REQ-03](../../prd.md#tool-modes).
Decision: [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) §1 (device-local tool state),
as amended by [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md).
What the tools act on: [SRS-EP-07](../device-document/srs-logic.md) (document),
[SRS-EP-10](../ink-box/srs-logic.md) (creation), [SRS-EP-11](../ink-box/srs-logic.md) (manipulation).

**Implementation status (code SoT, 2026-08-11):** ToolChip + `toolMode` + `stroke_begin.intent`
+ `pickables` ingest + `tool_intent` emit in `tabletcanvasitem` / `Main.qml` (STORY-EP-005).
Touch-on-chip uses MouseArea; pen-on-chip press is ignored for ink (fallback path).
**The intent-emission half of that code is superseded** — the chip and routing survive.

## [SRS-EP-04] Tool state and input routing

<!-- revised: 2026-08-13 — CHL-0008 / ADR-0014. Enclose-intent and Selection-intent tables retired;
     tools now invoke local document operations. Same id, content revised. -->

> **Revised 2026-08-13.** This section is now only about **what the pen does next** — tool state and
> input routing. The two intent tables are gone: there is no `intent` flag, no `pickables`, no
> `tool_intent`, and no advisory ghost. A tool invokes a local operation and the document changes
> immediately.

### Tool state

| Rule | Value |
|---|---|
| Tools | `selection` \| `pen` \| `ink_box` |
| Default on launch | `pen` — the device must still be a notebook if nothing else works |
| Ownership | **Device-local UI state.** Never sent to Infini, never set by Infini (ADR-0013 §1) |
| Persistence | Not persisted across restarts in v0 |
| Input | Finger touch on the ToolChip (pen-on-chip fallback). Pen events on the chip are not ink |

### Input routing

| Tool | Pen down on canvas | Finger / pen on ToolChip | Finger on canvas |
|---|---|---|---|
| `pen` | Local ink → ingest as an `Ink` node at pen-up ([SRS-EP-07](../device-document/srs-logic.md)); draw-into membership evaluated ([SRS-EP-10](../ink-box/srs-logic.md)) | Switch tool | Ignored (no on-device pan — PRD Non-Goal) |
| `ink_box` | Local ink → evaluate enclose at pen-up ([SRS-EP-10](../ink-box/srs-logic.md)) | Switch tool | Ignored |
| `selection` | Pick / move / resize against the local document ([SRS-EP-11](../ink-box/srs-logic.md)) | Switch tool | Ignored |

`ink_box` and `pen` share one ink path — same Round 19 map, same paint, byte-identical while the pen
is down. They differ only in **what the device does at pen-up**, so ink latency cannot regress by
tool ([SRS-EP-01](../local-pen-ink/srs-logic.md)).

### Arming

| Rule | Value |
|---|---|
| `ink_box` stays armed | Repeated boxes without re-tapping, until the creator switches tool |
| Arming is the confirmation | No propose/accept step; creation is immediate and undoable (ADR-0013, ADR-0011 §4A as amended) |
| Refused enclose | The stroke stays ordinary ink; the tool stays armed; no banner |

### Tool independence

Tool mode is device-local and now trivially so: the peer has no tools this campaign
([infini REQ-04](../../../infini/prd.md#smart-group) deprecated) and never sees the device's.

### Errors / partial failure

| Case | Behavior |
|---|---|
| Touch layer unavailable at runtime | Fall back to `pen` permanently; surface it in the status line; never trap the creator in a non-drawing tool |
| Session down | **All three tools stay fully available** — editing is local ([REQ-04](../../prd.md#device-document)). Only publishing waits; the status affordance shows changes are queued |
| Pen-down starts on the ToolChip bounds | Not ink; may arm a tool when pen-on-chip fallback is active |
| Tool switched mid-gesture | The in-flight gesture completes under the tool it started with; the new tool applies from the next pen-down |

### Other logic

- **Exclusion rect = ToolChip bounds** (floating chip), not a full edge band. A stroke must never
  begin inside that rect; `InkSurface` stays full-bleed ([SRS-EP-05](./srs-ui.md), CHL-0003).
- Tool switching must not invalidate the full panel — partial refresh of the **chip** only
  (the ink area keeps its content, [SRS-EP-06](./srs-quality.md)).
- Chip anchor follows **gut orientation top** (see SRS-EP-05); exclusion rect moves with it.
