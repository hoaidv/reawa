---
from: qa
to: dev
date: 2026-08-13
iter: iter-003
cc: [sm]
---

# Hand-off: QA → Dev — W10b Device Log BDD ready

## Pickup

| Lane | Story | Feature | Tags |
|---|---|---|---|
| **E** | [STORY-IN-029](../stories/STORY-IN-029.md) | [device-log.feature](../../../.docs/modules/infini/features/tablet-sync/bdd/device-log.feature) | `@SRS-IN-17` `@SRS-IN-18` `@SRS-IN-19` |
| **F** | [STORY-EP-021](../stories/STORY-EP-021.md) | [debug-log-ship.feature](../../../.docs/modules/epaper/features/device-document/bdd/debug-log-ship.feature) | `@SRS-EP-15` `@SRS-EP-16` |

Stories stay **`ready`** until Dev flips `in-progress`. UI regions/copy from
[SRS-IN-18](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md) — no design package.

## Asks

1. Implement both lanes in parallel (no file conflict).
2. Cover each scenario with a unit or host test where Electron/Qt I/O cannot run here.
3. Set each story `in-review` when tests pass. Do not flip `done`. Do not flip EP-016 / EP-017.
