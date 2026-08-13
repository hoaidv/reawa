---
id: CHL-0009
title: device-document srs-logic.md is missing (SRS-EP-07 / SRS-EP-08)
status: open
severity: high
raised_by: sm
iter: iter-003
date: 2026-08-13
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

<!-- PM fills after triage: adopted | deferred | rejected -->

## Product doc updates

<!-- Architect: add `.docs/modules/epaper/features/device-document/srs-logic.md` -->

## Interrupt / expedite (when applicable)

Not an interrupt. TRACK-003 continues on the design + latency wave.
