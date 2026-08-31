---
captured: 2026-08-31
verified: 2026-08-31
related:
  - SRS-EP-01
  - SRS-EP-03
  - SRS-EP-24
---

# Camera pan/zoom — blit preview + LatestJob sharpen

Field (blit slice): pan/zoom **felt** smooth, but sharpen was random, zoom-in stayed
pixelated, and newly revealed strips often stayed white after the gesture. Cause:
GUI-thread strip fill usually painted nothing (or was skipped for `cam=panzoom`),
and settle **skipped** once `m_rasterCam` already matched (`cam=none`), so the
vector never ran. The same GUI vector/timer was also stealing the ink path.

## Pipeline

```
pointer / 200 ms nav
  → blit m_image (ghost OK; white strips / stretched zoom OK)
  → LatestJob.submit(camera snapshot + shared tree)
worker (one in-flight, one pending; newer camera replaces pending only —
in-flight is *not* cancelled, so a ~700 ms vector can finish mid-gesture)
  → vector FullClear into a private QImage (cancel only on destruction)
GUI deliver
  → drop if document epoch moved (ink) or cancelled
  → hold if a stroke is active (do not wipe Pen stamps)
  → if camera still matches: swap m_image (strips + AA present)
  → if camera moved: warp the sharp buffer toward now, queue another job
settle / pointer-up
  → always submit for the final camera (never skip)
```

Do **not** FullClear ordinary ink (CHL-0029). Do **not** swap a camera job during
a stroke. Transform/erase stay GUI InPlaceDirty.

## Probe

`blit=1` = preview or a warped job. `blit=0 sharp=1` with `render_ms` in the
hundreds = worker vector delivered. After settle, a `blit=0` line for that
camera must exist; white strips without that line is a miss.

```bash
ssh root@10.11.99.1 'tail -f /tmp/epaper-raster.log'
```
