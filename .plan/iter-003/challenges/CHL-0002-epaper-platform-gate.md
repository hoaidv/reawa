---
id: CHL-0002
title: Allow data-platform epaper in adlc design gate
status: resolved
resolution: adopted
iter: iter-003
raised_by: designer
date: 2026-08-11
resolved_by: pm
resolved: 2026-08-11
engine_patch: pending-human-adlc-approval
---

# CHL-0002 — `data-platform="epaper"` rejected by design gate

## Conflict

[SRS-EP-05](../../.docs/modules/epaper/features/tool-modes/srs-ui.md) declares
`data-platform: epaper`. The mechanical gate only accepts
`ios|android|web|desktop` (`design_gate.py` `_PLATFORM_HTML`).

Designer shipped scenes with `data-platform="epaper"` per SRS (not inventing android/web).

## Ask

- **Engine / ADLC:** extend allowlist with `epaper` (touch kit: no hover, ≥120 px historically —
  pilot chip is 32 px per CHL-0003; platform tag is still `epaper`).
- Until then: gate *Design platform* may FAIL for STORY-EP-003 — Spec remains authoritative.

## Resolution

**Adopted** — 2026-08-11 (PM).

Product SoT wins: `data-platform="epaper"` is correct and stays on SRS-EP-05 / `[UI-EP-01]`.
Do **not** retarget scenes to `android`/`web` to please the gate.

**Engine residual (ADLC — needs human approval):** one-line allowlist in
`.agent/tools/adlc/design_gate.py` `_PLATFORM_HTML` → add `epaper`. Until patched, treat
*Design platform* FAIL as known noise; Spec/SRS authoritative for EP-003.

No product-doc lifecycle change. No story replan.
