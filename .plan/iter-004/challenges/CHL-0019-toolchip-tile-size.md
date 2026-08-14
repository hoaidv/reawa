---
id: CHL-0019
author: designer
target: [SRS-EP-05]
severity: medium
status: open
opened: 2026-08-14
iter: iter-004
expedite: false
interrupts_track: ""
---

# CHL-0019 — ToolChip tile size: SRS 32 px vs shipped 64 px

## Context

[SRS-EP-05](../../.docs/modules/epaper/features/tool-modes/srs-ui.md) still lists chip height
**32 px** / 32×32 tiles (CHL-0003 original). [UI-EP-01](../iter-003/design/epaper-tool-strip/ui-spec.md)
v0.4 and [UI-EP-03](../iter-003/design/selection-enclose-chrome/) compose **64×64** after human
verify that 32 px was too small on RM2. EP-023 / EP-025 passed at 64.

STORY-EP-026 must not regress that chrome. This package paints **64×64** (shipped geometry).
SRS-EP-05 text is stale.

## Proposal

Amend SRS-EP-05 physical constraints: height **64 px**, tiles **64×64**, still squared,
`border-radius: 0`, floating orientation-top. Keep CHL-0003's "not a full-band ≥120 px strip".

## Resolution
<!-- PM fills after triage: adopted | deferred | rejected -->

## Product doc updates
<!-- List docs updated if adopted -->
