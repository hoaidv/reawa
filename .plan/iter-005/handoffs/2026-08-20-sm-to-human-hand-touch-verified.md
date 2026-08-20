---
from: sm
to: human
date: 2026-08-20
iter: iter-005
cc: [pm, architect, designer]
---

# Hand-off: Scrum Master → human — hand-touch field-test approved

You approved the **hand-touch** delivery. Product, architecture, and [UI-EP-06](../design/hand-touch/ui-spec.md) now match what shipped. [TRACK-005](../../tracks/TRACK-005-hand-on-paper.md) stays **paused** for the remaining Infini + Epaper follow score. Do **not** start W3 erase or Device Settings until you say go.

## What is now product truth ([REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch))

- Palm-rest / empty tap: **≤ 20 mm** (178 du @ 226 dpi); empty tap deselects.
- Local one-finger pan: travel **past 20 mm**.
- **≥3** capacitive contacts = palm (0 pan, 0 pinch).
- **HT** toggle default **on**; off kills canvas fingers; chrome still works.
- Pen near or in contact still disables canvas hand-touch.

Stories [STORY-EP-038](../stories/STORY-EP-038.md), [STORY-EP-039](../stories/STORY-EP-039.md), [STORY-EP-054](../stories/STORY-EP-054.md) stay **`done`**. Acceptance text restated to 20 mm / 178 du.

## Still open on hardware

The original pause asked you to score **viewport follow** on Infini + Epaper (follow on/off, exclusivity, drop/reconnect). That score is **not** in this approval.

## Resume

In `/sm` say: follow field-test pass/fail/notes, **and/or** pick W3 erase design vs Device Settings.
