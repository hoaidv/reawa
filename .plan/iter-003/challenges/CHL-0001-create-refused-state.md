---
id: CHL-0001
title: Add create_refused to SRS-IN-14 states matrix
status: open
iter: iter-003
raised_by: designer
date: 2026-08-11
---

# CHL-0001 — create_refused state missing from SRS-IN-14 matrix

## Conflict

[SRS-IN-14](../../.docs/modules/infini/features/vector-document/srs-ui.md#srs-in-14-ink-box-ui)
interaction map + [srs-experience](../../.docs/modules/infini/features/vector-document/srs-experience.md)
journey `select_create_refuse` require a refuse-create UI, but the **states matrix** does not list
`tool.selection.create_refused` (or equivalent).

Designer shipped scene `ink-box-ui-create-refused.html` per SM AC / journey (not invented inventory).

## Ask PM

Adopt a states-matrix row, e.g. `tool.selection.create_refused` | Selection armed | multi-ink selected | CTA disabled + refuse hint.

## Product doc updates (on adopt)

- `srs-ui.md` SRS-IN-14 states matrix + control-states for `cta.create_smart_group`
