---
captured: 2026-08-31
verified: 2026-08-31
related:
  - SRS-EP-01
  - SRS-EP-03
  - SRS-EP-11
  - SRS-EP-12
  - SRS-EP-13
  - CHL-0029
  - CHL-0030
---

# Dense-page ink hitch — regression note

Session index: [2026-08-31-field-latency.md](./2026-08-31-field-latency.md).

Field interrupt on TRACK-005 after erase phase (EP-062…068): a dense page (many ink-boxes + free
inks) made **new** strokes hitch ~100 ms then run smooth. Near and far from boxes. Recognizers and
hand-touch off. Continuous pen-up / pen-down.

**Verified 2026-08-31 on RM2:** packed dense-page hitch (FullClear between strokes) is gone.
**Same day, later:** pan/zoom LatestJob pipeline made camera **better**; residual **small random
pen-to-ink lag** remains on a *moderately* dense page (~4 sentences + ink-boxes) —
[STORY-EP-070](../../.plan/iter-005/stories/STORY-EP-070.md).

## What works

| Path | Behaviour |
|---|---|
| Ordinary `RecogOutcome::Ink` | No Tablet `rasterizeVectors`. Live Pen stamps **are** the settle ([CHL-0029](../../.plan/iter-005/challenges/CHL-0029-settle-is-not-fullclear-on-ink.md)). |
| 180 ms settle follow-up | **Gone.** It stole downs after handwriting pauses. |
| Camera soft coalesce | Vector FullClear is a **LatestJob** off the GUI thread. Must **not** swap `m_image` during a stroke (wipes Pen stamps). Pan/zoom preview **blits**. |
| InkMode overlay | **Hidden while a stroke is active** so ToolCanvas Mono cannot cover live Pen ink. |
| NodeEmphasis blink / membership Bold | Enclose/connector blink on ToolCanvas with `includeIds`. **Draw-into does not Bold** — box-AABB damage stalled every pen-up. |
| Draw-into membership with recog off | Still **joins** (not an ink-box recognizer). Chrome must not stay Mono over the next stroke. |

## What does not (known leftover / do not “fix” by FullClear)

| Path | Behaviour |
|---|---|
| Move / resize pen-down | Still runs InPlaceDirty origin punch on that down (`slow_sample rasterize.render`). Correct; not an ink stroke. |
| `doc_load`, orientation, first camera | Still GUI vector FullClear (not on the pointer stack). Pan/zoom **preview** is a blit; **sharpen** is LatestJob. |
| Enclose / connector **create** | One InPlaceDirty of the group / spine∪boxes at pen-up. |
| Camera pending during a stroke | Job is held until pen-up, then a **new** snapshot (includes the stroke). Must not FullClear on the pointer stack. |
| Overlay paint after `setVisible(false)` | Sync `overlayPaintOk` + skip `paint()` while `strokeActive()`. Membership Bold is cleared on ink down. |
| Recognizers off | Does **not** disable draw-into membership. Overlay must hide on the next ink down. |
| Residual pen-to-ink on a 4-sentence + ink-box page | Small, random, noticeable. Stroke op is fine. Attribute with ink-path, then keep LatestJob / full-panel `update()` / overlay off the sample. [STORY-EP-070](../../.plan/iter-005/stories/STORY-EP-070.md). |

## Causes we hit (in order)

1. **FullClear between strokes** (680–946 ms). `reason=queued i=0 behind=rasterizeVectors`. GUI thread, not `InkStrokeOperation`. 180 ms follow-up + `documentMutated` + camera.
2. **ToolCanvas Mono over Pen ink.** After (1), downs were `behind=toolPaint` 77–357 ms. Ink went solid → dash-dash-dash → solid. Membership Bold kept overlay visible; `paint()` HierarchyCull’d the box AABB and re-stroked every neighbor in Mono.
3. **Sync rasterize on ordinary pen-up.** Camera-pending `rasterizeVectors` inside `endStroke` (438–624 ms) or the next down (`slow_sample rasterize.render` 24–120 ms, `inplace true`).
4. **(later same day)** GUI camera FullClear / 250 ms timer / strip fill on the pointer stack — replaced by blit + LatestJob. Residual lag after that is [STORY-EP-070](../../.plan/iter-005/stories/STORY-EP-070.md).

## How to attribute a regression

Always-on probe: `/tmp/epaper-ink-path.log` (stderr too). `EPAPER_INK_PATH=0` off. Stall: `/tmp/epaper-ui-stall.log`. Rasterize: `[raster] …` in `/tmp/epaper-raster.log` (camera pan/zoom vs dirty).

| Log | Meaning | Likely revert |
|---|---|---|
| `event=down … behind=rasterizeVectors` 100s of ms, `inplace false` | FullClear stole the first sample | Ordinary ink skip; 180 ms follow-up; `documentMutated` consume flag |
| `event=down … behind=toolPaint` | ToolCanvas Mono painted on the GUI thread | InkMode hide-while-stroke; `includeIds`; stamp clear → `syncOverlayPresence` |
| Solid ink becomes dash-dash then solid | Mono (or non-Pen) refresh over Pen stamps | Overlay visible during ink, or Tablet `update()` of a huge rect |
| `event=down … slow_sample slowest=rasterize.render` with `spans=rasterize.*` | Rasterize **on the pointer stack** | Camera/enclose running on ink down; transform punch is OK if they were moving a box |
| `endStroke` stall 400+ ms on ordinary ink | Sync rasterize or heavy ingest on up | Camera pending must LatestJob, not `rasterizeVectors` in `endStroke` |
| `[recog] … fail=recog_off` **and** `outcome=membership` | Draw-into still ran | Expected. Chrome, not join, is the hitch |

Guided review of the first slice: [`.plan/iter-005/handoffs/2026-08-31-dev-guided-review-rasterize-dirty.md`](../../.plan/iter-005/handoffs/2026-08-31-dev-guided-review-rasterize-dirty.md).

Connector origin punch (same InPlaceDirty machinery): [connector-live-manip-settle.md](./connector-live-manip-settle.md).
