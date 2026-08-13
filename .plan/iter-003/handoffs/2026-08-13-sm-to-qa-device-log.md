---
from: sm
to: qa
date: 2026-08-13
iter: iter-003
cc: [dev]
---

# Hand-off: SM → QA — W10b Device Log BDD (author)

## Context

Human locked architect option 2 (`:9878`, in-app Device Log panel) so they can inspect
EP-012…016 before W10 membership. [STORY-EP-016](../stories/STORY-EP-016.md) stays
**`in-review`**. Do not flip EP-017.

SRS: [architect handoff](./2026-08-13-architect-to-sm-device-log.md).

## Pickup

| Lane | Story | Status | Feature file (create) | Tags |
|---|---|---|---|---|
| **E** | [STORY-IN-029](../stories/STORY-IN-029.md) | **ready** | `infini/features/tablet-sync/bdd/device-log.feature` | `@SRS-IN-17` `@SRS-IN-18` `@SRS-IN-19` |
| **F** | [STORY-EP-021](../stories/STORY-EP-021.md) | **ready** | `epaper/features/device-document/bdd/debug-log-ship.feature` | `@SRS-EP-15` `@SRS-EP-16` |

UI regions/copy are in [SRS-IN-18](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md) —
no design package. Cover overlay states present in AC: closed, open/empty, streaming,
disconnected, filtered.

## Asks

1. One Gherkin scenario per AC, tagged as above.
2. Hand `/dev` per lane. Do not flip `done`.
3. Do not put `debug_*` on `:9877` scenarios as legal traffic.

## Hold

- EP-017 / EP-018 / EP-020 / IN-028
- ADR-0015 message set
- `recognize_enclose.hpp` guards
