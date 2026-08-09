---
id: CHG-0001
title: Promote EXP-0001 local ink to Epaper source root
date: 2026-08-09
iter: iter-001
source: EXP-0001
---

# CHG-0001 — Promote local pen ink to `Epaper/`

## Decision

Lift the verified RM2 local-ink spike into repo-root **`Epaper/`** (sibling of
`Sources/`), named Epaper. Not a SwiftPM target.

## Product docs

- New module `.docs/modules/epaper/` with [REQ-01] / [SRS-EP-01]
- BRD gains [BRD-06] for on-device e-paper drawing

## Code

- `Epaper/` — Qt Quick + C++ sources + `scripts/deploy-rm2.sh`
- Sandbox spike under `.sandbox/` remains ignored; canonical tree is `Epaper/`
