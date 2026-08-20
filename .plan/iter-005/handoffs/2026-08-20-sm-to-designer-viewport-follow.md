---
from: sm
to: designer
date: 2026-08-20
iter: iter-005
---

# Hand-off: SM → Designer — viewport-follow toggles EP-053 ∥ IN-036

Human 2026-08-20: cameras **independent by default**; optional **one-way** follow; exactly one direction; off on disconnect. [ADR-0029](../../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md). Last-writer [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) is **superseded** — do not paint token chrome.

Work-in-progress 2: **only these two packages**.

| Lane | Story | Package | Platform |
|---|---|---|---|
| A | [STORY-EP-053](../stories/STORY-EP-053.md) | `design/viewport-follow-epaper/` | epaper-device |
| B | [STORY-IN-036](../stories/STORY-IN-036.md) | `design/viewport-follow-infini/` | desktop |

Queued (do not open): [STORY-EP-054](../stories/STORY-EP-054.md) hand-touch empty-pan (`hand-touch/` — different write set, still wait for WIP).

Do **not** edit `.docs/design/index.md` (Scrum Master stitches). Unique names under `design/system/` only.
