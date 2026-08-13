---
from: sm
to: dev
date: 2026-08-13
iter: iter-003
cc: [qa]
---

# Hand-off: SM → Dev — W10b Device Log (after QA walk)

## Pickup

| Lane | Story | Status | Writes |
|---|---|---|---|
| **E** | [STORY-IN-029](../stories/STORY-IN-029.md) | **ready** | `infini/` Device Log overlay + listen `:9878` |
| **F** | [STORY-EP-021](../stories/STORY-EP-021.md) | **ready** | `epaper/` worker shipper; `[enclose]` qInfo after ingest |

No file conflict. Wait for QA BDD, then implement.

## Lane E — Infini

- Logic: SRS-IN-17; UI: SRS-IN-18; quality: SRS-IN-19
- Listen `INFINI_DEBUG_PORT` default **9878**. Separate decoder from `:9877`.
- Button **Device Log** in window chrome (not WorldLayer). Overlay covers canvas; not a second window.
- In-memory ring cap 10000. Filter is view-only.
- Open → `debug_request` then `debug_start`. Close / Escape → `debug_stop`; keep buffer.
- `debug_log` never applied to VectorDocument.

## Lane F — Epaper

- SRS-EP-15 / SRS-EP-16. Env `EPAPER_DEBUG_LOG=1` default off.
- Connect `{RM_SYNC_HOST}:9878`. Worker thread; **0** socket I/O on paint / `ingestPoint`.
- Qt handler required; stdout/stderr if capture succeeds.
- After `ingestStrokeAtPenUp` returns: `qInfo` `[enclose] created id=…` or `[enclose] ordinary reason=…`.
  Do **not** edit `recognize_enclose.hpp` guards.
- Deploy with `EPAPER_DEBUG_LOG=1` (no synth). Leave epaper running.

## After code

Set each story `in-review`. Do not flip EP-016 `done` or EP-017 `ready`.
