---
from: architect
to: sm
date: 2026-08-13
iter: iter-003
cc: [pm, designer]
---

# Hand-off: Architect → SM — CHL-0013 / EP-018 chrome

## Verdict

**READY-WITH-CONCERNS** for design slice. Implement EP-018 waits on EP-022.

## Delivered

| Artifact | Change |
|---|---|
| [ADR-0016](../../../.docs/adr/ADR-0016-selection-create-enclose-cta.md) | Enclose CTA on SelectionOverlay; chip stays 3 |
| [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) | Marquee select + `cta.enclose` invoke; SG-in-selection refuse |
| [SRS-EP-11](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) | Marquee pickable set; gestures |
| [SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md) | Controls/states: marquee, nodes_bounds, 6 anchors, cta.enclose |
| [srs-experience](../../../.docs/modules/epaper/features/ink-box/srs-experience.md) | journey.device_select_create updated |
| PRD REQ-05 | Creation B AC for rubber-band + Enclose |

## Concerns

1. Marquee vs SmartGroup-move gesture disambiguation — press-on-box still moves; marquee starts off handles / empty canvas (SRS gestures table). Designer must make hit targets obvious.
2. 6-anchor layout (corners + which mids) is Designer choice; count is locked.
3. BDD `selection-create-surround.feature` needs QA extension for marquee/CTA before Dev.

## Asks

SM: design story ready → `/designer`. Do not mark EP-018 `ready` until EP-022 `done`.
