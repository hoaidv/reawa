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
| Tools | `sel_rect` \| `sel_freeform` \| `pen` |
| Recognizer toggles | `recog.ink_box` \| `recog.connector` — independent; both default **on**; dimmed (state kept) while a Selection tool is active ([ADR-0021](../../../../adr/ADR-0021-connector-toolchip.md)) |
| Latch | Exclusive tool **and** both toggles latch at pen-down for the whole stroke |
| Default on launch | `pen`, both recognizers armed — the device must still be a notebook if nothing else works |
| Ownership | **Device-local UI state.** Never sent to Infini, never set by Infini (ADR-0013 §1) |
| Persistence | Not persisted across restarts in v0 |
| Input | Finger touch on the ToolChip (pen-on-chip fallback). Pen events on the chip are not ink |

### Input routing

| Tool | Pen down on canvas | Finger / pen on ToolChip | Finger on canvas |
|---|---|---|---|
| `pen` | Local ink → **dispatch** at pen-up ([ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md)): enclose / membership / connector / ordinary ink | Switch tool or flip a toggle | Ignored (no on-device pan — PRD Non-Goal) |
| `sel_rect` | Rect marquee / pick / move / resize against the local document ([SRS-EP-10](../ink-box/srs-logic.md), [SRS-EP-11](../ink-box/srs-logic.md)) | Switch tool | Ignored |
| `sel_freeform` | Freeform lasso / pick / move / resize ([SRS-EP-10](../ink-box/srs-logic.md), [SRS-EP-11](../ink-box/srs-logic.md)) | Switch tool | Ignored |

`pen` is the only inking exclusive tool — same Round 19 map, same paint. Recognizers differ only
in **what the device does at pen-up**, so ink latency cannot regress by toggle
([SRS-EP-01](../local-pen-ink/srs-logic.md)).

### Arming

| Rule | Value |
|---|---|
| Recognizers stay armed across strokes | Until the creator toggles them off |
| Dimmed under Selection | Armed state retained; they do not run |
| Refused enclose or connector | The stroke stays ordinary ink (or falls through per ADR-0022); no banner |

### Tool independence

Tool mode is device-local and now trivially so: the peer has no tools this campaign
([infini REQ-04](../../../infini/prd.md#smart-group) deprecated) and never sees the device's.

### Errors / partial failure

| Case | Behavior |
|---|---|
| Touch layer unavailable at runtime | Fall back to `pen` permanently; surface it in the status line; never trap the creator in a non-drawing tool |
| Session down | **All three exclusive tools and both toggles stay fully available** — editing is local ([REQ-04](../../prd.md#device-document)). Only publishing waits; the status affordance shows changes are queued |
| Pen-down starts on the ToolChip bounds | Not ink; may arm a tool when pen-on-chip fallback is active |
| Tool switched mid-gesture | The in-flight gesture completes under the tool it started with; the new tool applies from the next pen-down |

### Other logic

- **Exclusion rect = ToolChip bounds** (floating chip), not a full edge band. A stroke must never
  begin inside that rect; `InkSurface` stays full-bleed ([SRS-EP-05](./srs-ui.md), CHL-0003).
- Tool switching must not invalidate the full panel — partial refresh of the **chip** only
  (the ink area keeps its content, [SRS-EP-06](./srs-quality.md)).
- Chip anchor follows **gut orientation top** (see SRS-EP-05); exclusion rect moves with it.
