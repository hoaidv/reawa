---
from: sm
to: designer
date: 2026-08-13
iter: iter-003
cc: [architect, pm, qa]
---

# Hand-off: SM → Designer — EP-022 selection-enclose chrome

## Pipeline (driven)

1. **PM** adopted [CHL-0013](../challenges/CHL-0013-selection-create-feedback-enclose-cta.md)
2. **Architect** thickened SRS-EP-10/11/12 + [ADR-0016](../../../.docs/adr/ADR-0016-selection-create-enclose-cta.md)
3. **SM** sliced [STORY-EP-022](../stories/STORY-EP-022.md) **ready**

## Pickup

| Story | Status | Package |
|---|---|---|
| [STORY-EP-022](../stories/STORY-EP-022.md) | **ready** | `.plan/iter-003/design/selection-enclose-chrome/` |
| Compose | done | [UI-EP-02](../design/device-selection-chrome/) / [UI-EP-01](../design/epaper-tool-strip/) |

## Must design

1. `sel.marquee` — thin dotted rubber-band follows pen tip
2. `sel.nodes_selected` — dotted union rect + **6** square anchors + **`cta.enclose`**
3. `sel.create_refused` — refuse visible; selection kept
4. ToolChip stays **three** tools (ADR-0016)

## After design `done`

SM copies `ui_spec` / `scenes` / `hifi` onto [STORY-EP-018](../stories/STORY-EP-018.md) and flips
EP-018 → `ready` → `/qa` then `/dev`.

## Hold

- Undo chrome (CHL-0010)
- Nested enclose (CHL-0011)
- Anchor drag events (later)
