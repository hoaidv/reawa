---
source: .plan/iter-005/retro.md (qa reflection)
captured: 2026-10-04
related:
  - STORY-EP-059
  - STORY-EP-060
  - STORY-EP-061
  - TRACK-009
---

# Rebuilt Epaper: host tests before device verification

In `epaper/`, from iter-006 on:

- Each acceptance scenario maps to a named `ctest` case.
- A story goes to human verification on the reMarkable 2 only when `ctest` is green, and also
  the sanitizer presets once they exist (step 1 of the
  [forest plan](../../.plan/document-forest-implementation.md)).
- Acceptance criteria that only the device can check are marked **device-only** in the verify
  handoff, so "human-verified" never stands in for host coverage.
- A known-red host check is a defect, not a "pre-existing" note. In iter-005,
  `dispatch_test` asserted `kMinEncloseWorld == 28` against 36 / 42 in code and was never filed.

The inverse-undo stories EP-059…061 are the model: scenarios written before the code, each mapped
one-to-one to a host test.

`adlc audit` does not scan `.cpp` files, so implemented SRS records in C++ still show as orphans.
