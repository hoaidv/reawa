---
source: .plan/iter-005/memory/erase-brush-duplicate-remnant-ids.md
captured: 2026-08-29
related:
  - STORY-EP-063
  - STORY-EP-064
  - STORY-EP-067
---

# Brush erase commit — remnant ids and clip

Product facts from the 2026-08-29 brush-erase field pass. Specs stay in [prd-erase.md](../modules/epaper/prd-erase.md) and [SRS-EP-55](../modules/epaper/features/erase/srs-logic.md#srs-ep-55-clip-remnants).

## What actually failed

Brush ghost on ToolCanvas can look correct while `commitEraseRegion` returns `applied: false`. The log line is `[erase] commit … path N radius … inks … reason …`.

| `reason` | Meaning |
|---|---|
| `noop` | Clip found no remnants (true miss or AABB skip) |
| `duplicate_id:{id}` | Clip hit; `append_ink` of a remnant reused an existing node id; compound rolled back |

The field failure was **`duplicate_id:s-3_r1`**. Longest remnant keeps `s-3`. First extra was always `{id}_r1`. A second nick of `s-3` tried `s-3_r1` again. Handwriting often vanished in one pass (no extra id). Nicking the leftover `s-3_r1` node minted `s-3_r1_r1` and appeared to “work after erasing something else.”

Stopgap: `allocEraseRemnantId` in `epaper/document/erase_commit.hpp` skips ids already in the tree and in the current gesture. Canonical fix: [STORY-EP-067](../../.plan/iter-005/stories/STORY-EP-067.md) (Singleton generateNodeId for all tree nodes).

## What was not the bug

A uniform 0.25 mm walk along a 2-point ink edge **can** miss a very thin grazing chord. At production brush radius (~3 mm world) a normal nick almost always hits. Logs with a large `path` count and `duplicate_id` mean geometry already succeeded. A stadium-interval clip was prototyped and **rolled back**.

## Other brush facts (do not regress)

- Ghost is a persistent overlay image (last segment stamp). Clip uses world `m_world` → `capsuleRegion`. Ghost width = diameter; clip radius = half of that.
- Do not document-rasterize to “make erase work.” Do not revive quality-idle settle / Content–Mono overlay present (that painted the panel black).
- Hover kill-switch: `chip.eraseBrushHover` / QSettings `epaper/eraseBrushHover`.
- Brush millimetres in code may be 6 mm diameter / 3 mm radius while the Software Requirements Specification still says 8 mm / 4 mm — keep diameter = 2× radius; do not treat that mismatch as a commit bug.

## Call sites that still mint ids privately

`stroke_capture.hpp` (`s-` + sequence), `erase_commit.hpp` remnants, `recognize_connector.hpp` (`conn_` + stroke), `surround_create.hpp` (`sg_sel_` + winner). All must go through one document singleton before tablet→desktop sync assumes stable ids.
