---
captured: 2026-08-31
verified: 2026-08-31
related:
  - SRS-EP-01
  - SRS-EP-03
  - SRS-EP-24
  - STORY-EP-072
---

# Camera pan/zoom — blit preview + LatestJob sharpen

Session index: [2026-08-31-field-latency.md](./2026-08-31-field-latency.md).

## Field

| When | Result |
|---|---|
| First blit slice | Smooth pan/zoom; sharpen **random**; zoom-in stayed blocky; newly revealed strips often **white forever** |
| After LatestJob pipeline (same day) | Human: **better now**. Stress on a **full page** of handwriting + ink-box still needed — [STORY-EP-072](../../.plan/iter-005/stories/STORY-EP-072.md) |

## Why the blit slice failed

1. Blit committed `m_rasterCam`, then settle classified `cam=none` and **skipped** — vector never ran.
2. Two-finger moves are `panzoom` — no strip fill. A blit cannot invent newly revealed content.
3. Zoom-in only magnifies old pixels.
4. Every ~200 ms nav tick **cancelled** in-flight vector. Dense page ~700 ms render ⇒ job almost never finished mid-gesture.

## Pipeline (landed)

```
pointer / 200 ms nav
  → blit m_image (ghost OK; white strips / stretched zoom OK)
  → LatestJob.submit(camera snapshot + shared tree)
     pending = latest camera; in-flight is *not* cancelled
worker
  → vector FullClear into a private QImage (cancel on destruction only)
GUI deliver
  → drop if document epoch moved (ink) or cancelled
  → hold if a stroke is active (do not wipe Pen stamps)
  → if camera still matches: swap m_image (strips + AA present)
  → if camera moved: warp that sharp buffer toward now, queue another job
settle / pointer-up
  → always submit the final camera (never skip)
```

Do **not** FullClear ordinary ink ([CHL-0029](../../.plan/iter-005/challenges/CHL-0029-settle-is-not-fullclear-on-ink.md)).
Do **not** swap a camera job during a stroke. Transform/erase stay GUI InPlaceDirty.

## Probe

`blit=1` = preview or a warped job. `blit=0 sharp=1` with `render_ms` in the
hundreds = worker vector delivered. `cam=none` on that line means the job **matched**,
not that paint was skipped. After settle, a `blit=0` line for that camera must exist;
white strips without that line is a miss.

```bash
ssh root@10.11.99.1 'tail -f /tmp/epaper-raster.log'
```
