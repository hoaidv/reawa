---
from: qa
to: sm
date: 2026-08-13
iter: iter-003
cc: [dev]
---

# Hand-off: QA → SM — W10b Device Log host PASS, device WAIT USB

## Verdict

**READY-WITH-CONCERNS** — host/unit scenarios green; live RM2 ship not exercised this turn.

| Lane | Story | Host | Device |
|---|---|---|---|
| E | [STORY-IN-029](../stories/STORY-IN-029.md) | PASS (`tests/device-log.test.ts`, Infini 93/93) | Infini must be restarted for `:9878` + Device Log button |
| F | [STORY-EP-021](../stories/STORY-EP-021.md) | PASS (`debug_log_test OK`; ARM binary built) | WAIT USB — `en7` is `169.254.x`, not `10.11.99.12` |

Stories stay **`in-review`**. Do **not** flip `done` until the human opens Device Log against a live tablet (the reason this slice exists). EP-016 stays **`in-review`**. EP-017 stays **`draft`**.

## Isolation checks (code review)

- Infini `:9878` is a second `net.createServer` — `:9877` `broadcastStroke` path is unchanged.
- Overlay is a `WindowFrame` sibling of `CanvasStage`, not inside `WorldLayer`.
- Epaper `qInfo` `[enclose]` is after `ingestStrokeAtPenUp` in `tabletcanvasitem.cpp`. `recognize_enclose.hpp` not edited.

## Next

1. Plug USB so the Mac has `10.11.99.12` on `en7`.
2. `cd epaper && RM_SYNC_HOST=10.11.99.12 EPAPER_DEBUG_LOG=1 ./scripts/deploy-rm2.sh` (binary already built; do not `--restore`).
3. Restart Infini, click **Device Log**, draw Ink-box enclose, confirm `[enclose]` lines.
