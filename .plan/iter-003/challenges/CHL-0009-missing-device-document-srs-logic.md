---
id: CHL-0009
title: device-document srs-logic.md is missing (SRS-EP-07 / SRS-EP-08)
status: resolved
resolution: adopted
severity: high
raised_by: sm
resolved_by: pm
iter: iter-003
date: 2026-08-13
resolved: 2026-08-13
related: [REQ-04, REQ-07, SRS-EP-07, SRS-EP-08, ADR-0014, ADR-0015]
expedite: false
interrupts_track: ""
---

# CHL-0009 — Missing device-document logic SRS

## Context

SM re-slice of TRACK-003 against the [architect handoff](../handoffs/2026-08-13-architect-to-sm-device-document.md). That handoff assigns:

| Id | Claimed file |
|---|---|
| `SRS-EP-07` | `.docs/modules/epaper/features/device-document/srs-logic.md` — tree, ingestion, op set, undo ring |
| `SRS-EP-08` | same file — inbound classification, load handshake, publish queue, preview |

The feature folder has `index.md`, `srs-product.md`, `srs-data.md`, `srs-quality.md`, and BDD (`undo-ring.feature` `@SRS-EP-07`, `one-way-sync.feature` `@SRS-EP-08`). **`srs-logic.md` is not on disk.** Index, data, product, quality, and infini tablet-sync all link to it. `adlc` will not resolve `[SRS-EP-07]` / `[SRS-EP-08]` headings.

SM does not edit SRS. Document/sync implement stories are sliced against product AC + BDD + ADR-0014/0015 and stay `draft` until this file exists.

This does **not** block the NOW wave: design `[SRS-EP-12]` and the ink-latency measurement `[SRS-EP-13]` have complete specs.

## Proposal

Architect authors `device-document/srs-logic.md` with `[SRS-EP-07]` and `[SRS-EP-08]` matching the handoff table, the BDD scenarios already tagged, and the product/quality/data files that already point at those anchors. No id change.

## Resolution

**Adopted** 2026-08-13 by PM.

Completeness gap, not a product-direction change. IDs `[SRS-EP-07]` / `[SRS-EP-08]` were already assigned in the 2026-08-13 architect handoff; the file was never written. Product, data, quality, BDD, and infini tablet-sync already bind to those headings.

- **No PRD rewrite.** REQ-04 / REQ-07 and BR-D01…D12 stand.
- **No new ids.** Architect authors the missing file with the already-assigned headings.
- **Not an interrupt** (`expedite: false`). TRACK-003 continues on W8 (EP-012 ∥ EP-013).
- SM may flip EP-014 / EP-015 / EP-020 from `blocked` → `draft` after the file lands; `ready` still waits on EP-013 pass. PM does not flip story statuses.

## Product doc updates

- Created [`.docs/modules/epaper/features/device-document/srs-logic.md`](../../../.docs/modules/epaper/features/device-document/srs-logic.md) with `[SRS-EP-07]` `{#srs-ep-07-device-document}` and `[SRS-EP-08]` `{#srs-ep-08-one-way-sync}`. No new ids.
- Feature [index.md](../../../.docs/modules/epaper/features/device-document/index.md) Logic links now point at those anchors.

## Interrupt / expedite (when applicable)

Not an interrupt. TRACK-003 continues on the design + latency wave.
