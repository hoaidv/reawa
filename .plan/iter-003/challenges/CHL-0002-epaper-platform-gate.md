---
id: CHL-0002
title: Allow data-platform epaper in adlc design gate
status: open
iter: iter-003
raised_by: designer
date: 2026-08-11
---

# CHL-0002 — `data-platform="epaper"` rejected by design gate

## Conflict

[SRS-EP-05](../../.docs/modules/epaper/features/tool-modes/srs-ui.md) declares
`data-platform: epaper`. The mechanical gate only accepts
`ios|android|web|desktop` (`design_gate.py` `_PLATFORM_HTML`).

Designer shipped scenes with `data-platform="epaper"` per SRS (not inventing android/web).

## Ask

- **Engine / ADLC:** extend allowlist with `epaper` (touch kit: no hover, ≥120 px, 1-bit).
- Until then: gate *Design platform* may FAIL for STORY-EP-003 — Spec remains authoritative.
