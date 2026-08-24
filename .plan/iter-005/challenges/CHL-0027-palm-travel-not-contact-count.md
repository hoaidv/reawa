---
id: CHL-0027
author: engineer
target: [REQ-10, SRS-EP-21]
severity: medium
status: open
opened: 2026-08-24
iter: iter-005
expedite: false
interrupts_track: ""
---

# CHL-0027 — Palm rest by 20 mm travel, not 3-contact eat

## Context

[SRS-EP-21](../../../.docs/modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger) still says **≥3** simultaneous contacts = palm (0 pan, 0 pinch), and the app filter ate those events before Qt delivery.

Field: that 3-finger eat mostly does not work on RM2. Empty-canvas palm vs pan already uses **20 mm** / **178 du** travel. Human 2026-08-24: drop contact-count eat; rely on travel.

## Proposal

Stop eating ≥3-contact touch in the app filter. One-finger empty canvas keeps travelPastPalm / emptyTapClearsSelection (20 mm). Two-finger pan/pinch stays PinchHandler. Domain `palmByContactCount` remains for tests until PM revises SRS-EP-21.

## Resolution
<!-- PM: adopted | deferred | rejected -->

## Product doc updates
<!-- If adopted: strike ≥3-contact palm eat from SRS-EP-21; keep 20 mm travel as SoT -->
