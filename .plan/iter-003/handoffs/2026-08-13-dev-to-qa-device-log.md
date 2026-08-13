---
from: dev
to: qa
date: 2026-08-13
iter: iter-003
cc: [sm]
---

# Hand-off: Dev → QA — W10b Device Log in-review

## Stories

| Lane | Story | Status | Tests |
|---|---|---|---|
| **E** | [STORY-IN-029](../stories/STORY-IN-029.md) | **in-review** | `infini` vitest 93/93 including `tests/device-log.test.ts` |
| **F** | [STORY-EP-021](../stories/STORY-EP-021.md) | **in-review** | `epaper/tests/run_device_document_test.sh` — `debug_log_test OK`; ARM binary linked |

EP-016 stays **`in-review`**. EP-017 stays **`draft`**. `:9877` decoder unchanged (no `debug_*` cases). `recognize_enclose.hpp` untouched.

## What landed

**Infini** — listen `INFINI_DEBUG_PORT` default **9878** in `electron/main.cjs` (separate server from 9877). Device Log button + full-size overlay on `WindowFrame` (`DeviceLogChrome`), not WorldLayer. Ring cap 10000 in main. Filter is view-only.

**Epaper** — `DebugLogShip` worker thread; env `EPAPER_DEBUG_LOG`. Qt handler try-lock enqueue; stdout/stderr pipes on the worker. After `ingestStrokeAtPenUp`: one `qInfo` `[enclose] armed=… outcome=…`.

## Human check

1. Restart Infini (`npm run electron:dev` in `infini/`) so `:9878` is listening.
2. Deploy (if not already): `RM_SYNC_HOST=10.11.99.12 EPAPER_DEBUG_LOG=1 ./scripts/deploy-rm2.sh` from `epaper/` (binary already built). Do **not** `--restore`.
3. Click **Device Log**. Draw Ink-box enclose on RM2. Look for `[enclose]` lines.

## Asks

Verify tagged BDD via the unit/host tests above. Do not flip `done` until the human has used the panel (this slice exists so they can inspect EP-012…016).
