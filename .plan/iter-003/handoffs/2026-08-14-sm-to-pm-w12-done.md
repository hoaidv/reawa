---
from: sm
to: pm
date: 2026-08-14
iter: iter-003
cc: [qa]
verdict: W12 complete
---

# Hand-off: SM → PM — W12 done, campaign committed work complete

## Context

W12 is **done**:

- [STORY-EP-020](../stories/STORY-EP-020.md) `done` — QA PASS + human confirm (device publish/handshake).
- [STORY-IN-028](../stories/STORY-IN-028.md) `done` — QA PASS + human confirm (drain then `doc_load`).

Human 2026-08-14: tablet→desktop ink-box create, membership, and move/resize sync after `RM_SYNC_HOST` was set. Handshake retry / seq-adopt landed as follow-up to a missed-hello / remount gap.

All TRACK-003 re-slice implement+design stories in the lock are `done`. Residue EP-007…011 / IN-020…026 stays blocked. No further implement wave inside this lock.

## Asks

1. `/pm` gate-close for EP-020 and IN-028 (QA already handed off).
2. Human campaign verify against exit criteria: epaper REQ-04…07 (device document, ink-box create, manip, one-way sync). Stop line is `verified`.
3. Do **not** open iter-004 until retro gate. Do **not** slice REQ-08 here.

## Constraints

- Deploy must export `RM_SYNC_HOST=<Mac USB IP>` (usually `10.11.99.12`).
- ADR-0019 amend for CHL-0018 remains deferred; not a campaign gate.
- CHL-0011 / CHL-0012 stay future.

## Out of scope

- Nested enclose, FREE_FORM / align-content, any-node manip (REQ-08)
- iter close / retro (wait for PM after human verify)
