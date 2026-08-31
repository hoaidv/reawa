---
captured: 2026-08-31
verified: 2026-08-31
related:
  - SRS-EP-01
  - SRS-EP-03
  - SRS-EP-11
  - SRS-EP-12
  - SRS-EP-18
  - SRS-EP-20
  - SRS-EP-24
  - CHL-0029
  - CHL-0030
  - STORY-EP-070
  - STORY-EP-071
  - STORY-EP-072
---

# Field session 2026-08-31 — dense-page latency

TRACK-005 interrupt after erase (EP-062…068). Not clipboard. EP-069 was already
**done**. Human on RM2 USB `root@10.11.99.1`. Deploy: `cd epaper && ./scripts/deploy-rm2.sh --build`.

Topic pages (symptoms, log → revert):

| Page | Owns |
|---|---|
| [ink-path-density-hitch.md](./ink-path-density-hitch.md) | Pen-to-ink hitch; `/tmp/epaper-ink-path.log` |
| [camera-pan-zoom-rasterize.md](./camera-pan-zoom-rasterize.md) | Pan/zoom blit + LatestJob; `/tmp/epaper-raster.log` |
| [connector-live-manip-settle.md](./connector-live-manip-settle.md) | Move/resize connector punch (BR-B19) |

Landed as `33c0a5c` (ink skip, InPlaceDirty, NodeEmphasis, connector union) then
`35b9c15` (camera off the GUI thread). Docs: `a32dff3`.

## What we fixed (verified on device)

| Symptom | Cause | Fix |
|---|---|---|
| New stroke hitch ~100 ms then smooth, dense page, near **and** far from boxes | GUI `rasterizeVectors` FullClear (680–946 ms) between strokes. Next down: `behind=rasterizeVectors`. Ordinary `RecogOutcome::Ink` was treated as FullClear; 180 ms follow-up stole the next down after a pause. | Skip document rasterize on ordinary ink ([CHL-0029](../../.plan/iter-005/challenges/CHL-0029-settle-is-not-fullclear-on-ink.md)). Delete 180 ms follow-up. `documentMutated` after `noteDocumentDirty` is consumed. |
| After (1) gone: hitch `behind=toolPaint`; ink solid → dash-dash-dash → solid | ToolCanvas Mono over Pen. Draw-into membership still runs with recog **off**. Bold stamp kept overlay visible; `NodeEmphasis` HierarchyCull’d the group AABB and re-stroked every overlapping ink. | Host-owned NodeEmphasis; paint **`includeIds` only**; **hide ToolCanvas while an ink stroke is active** ([CHL-0030](../../.plan/iter-005/challenges/CHL-0030-node-emphasis.md)). |
| Pen-up stall 400+ ms or next down `slow_sample rasterize.render` | Camera-pending vector ran **inside** `endStroke` or the next sample. | Ordinary ink must not sync-run camera rasterize. |
| Pan/zoom felt smooth but sharpen random; zoom-in stayed blocky; revealed strips often white | Blit committed `m_rasterCam` so settle skipped (`cam=none`); `panzoom` had no strip fill; ~200 ms ticks cancelled in-flight ~700 ms vectors. GUI vector also stole ink. | Preview **blits**; sharpen is **LatestJob** (in-flight not cancelled). Settle always submits. Do not swap onto `m_image` during a stroke. |
| Move/resize: leftover origin connector, missing middle of new spine; ToolCanvas already correct | InPlaceDirty punched only the **SmartGroup AABB**. Suppress ids included connectors; pixels **outside** the box were never `clearRect`’d. | `TransformGesture` dirty = origin ∪ live box ∪ **bound connector spines** (`DocContext::boundConnectorsPanelUnion`). |

Human: packed dense-page hitch gone; camera **better now**. Connector live vs settle matches.

## What still does not (do not “fix” by FullClear)

| Path | Status |
|---|---|
| Residual **small random** pen-to-ink on a **moderate** page (~4 sentences + ink-boxes) | Open [STORY-EP-070](../../.plan/iter-005/stories/STORY-EP-070.md). Stroke op is cheap; attribute with ink-path. |
| sel_rect / sel_freeform: live Selecting smooth, **settle to knobs** slower than expected | Instrument first — [STORY-EP-071](../../.plan/iter-005/stories/STORY-EP-071.md). Do not merge SelectionOverlay into ToolCanvas. |
| Camera under a **full** handwriting + ink-box page | Pipeline landed; stress probe — [STORY-EP-072](../../.plan/iter-005/stories/STORY-EP-072.md). |
| Move/resize **down** | Still InPlaceDirty origin punch (`slow_sample rasterize.render` on that down is OK if they were moving a box). |
| `doc_load`, orientation, first camera, missing/huge AABB | GUI FullClear (not on the pointer stack for pan/zoom). |
| Enclose / connector **create** | One InPlaceDirty at pen-up. |
| Recog off | Does **not** disable draw-into membership. Overlay must hide on the next ink down. |
| Mid-gesture e-ink ghosting | Allowed. Fail only if the **settled** picture is wrong. |
| Very long connector spine | Union can exceed 50% panel → FullClear fallback. |

## Rules that must not regress

- Ordinary `append_ink` ≠ FullClear. Live Pen stamps **are** settle.
- Camera vector is a LatestJob, **never** on the pointer stack. Do not cancel in-flight every nav tick.
- Connector dirty is never box-only.
- Clipboard / Device Settings stay frozen. Do not mix EP-069 into this work.

Guided review of the first slice: [`.plan/iter-005/handoffs/2026-08-31-dev-guided-review-rasterize-dirty.md`](../../.plan/iter-005/handoffs/2026-08-31-dev-guided-review-rasterize-dirty.md).
Follow-up stories: [`.plan/iter-005/handoffs/2026-08-31-sm-to-human-field-latency.md`](../../.plan/iter-005/handoffs/2026-08-31-sm-to-human-field-latency.md).
