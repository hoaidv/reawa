---
from: pm
to: sm
iter: iter-003
date: 2026-08-11
subject: challenges-triaged-track-003
cc: [architect, designer, qa, dev]
verdict: READY-WITH-CONCERNS
---

# PM → SM — Open challenges triaged (TRACK-003)

## Verdict

**READY-WITH-CONCERNS** — all three open challenges **Adopted**. One residual is ADLC tooling (not product).

| CHL | Resolution | Product impact |
|---|---|---|
| [CHL-0001](../challenges/CHL-0001-create-refused-state.md) | **Adopted** | SRS-IN-14 states matrix + `cta.create_smart_group` control-states |
| [CHL-0002](../challenges/CHL-0002-epaper-platform-gate.md) | **Adopted** (product) | `data-platform=epaper` confirmed; **ADLC allowlist patch pending human OK** |
| [CHL-0003](../challenges/CHL-0003-epaper-floating-toolchip.md) | **Adopted** | SRS-EP-05/04/06 + REQ-03 AC → floating **32px** ToolChip; full-bleed ink; exclusion = chip bounds |

## Ask SM

1. Confirm EP-005 / EP-003 story AC match chip (not full-band InkSurface shrink).
2. Keep TRACK-003 cursor: `/qa`→`/dev` W4 (IN-015); EP-005 still waits on IN-018.
3. No interrupt-track — severity normal; work stays in planned stream.

## Ask others

- `/architect` — no new ADR for CHL-0003 (chrome placement). Optional glance at exclusion-rect wording in SRS-EP-04.
- `/qa` — align any EP BDD with chip exclusion + 32px; IN create_refused already journey-covered.
- `/dev` — EP-005 implements ToolChip + hit-test, not band shrink.
- **Human** — approve one-line ADLC patch: `_PLATFORM_HTML` += `epaper` in `design_gate.py` (CHL-0002 residual).

## Findings / concerns

1. Gate *Design platform* may still FAIL until ADLC allowlist lands — Spec/SRS remain authoritative.
2. 32px finger targets are a deliberate pilot trade-off; pen-on-chip fallback stays binding.
