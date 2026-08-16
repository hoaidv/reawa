---
from: pm
to: architect
date: 2026-08-16
iter: iter-004
---

# Hand-off: PM → Architect — iter-005 **draft** PRD (do not decompose for TRACK-004)

## Context

Human asked `/pm` to capture the parked wave as an **iter-005 draft PRD**. Product docs updated; **iter-005 is not open**; TRACK-004 lock unchanged.

Source: [BS-0002](../brainstorms/BS-0002-iter-005-feature-wave.md) D10.
PRD: [epaper 0.8.0-draft](../../../.docs/modules/epaper/prd.md) [REQ-11](../../../.docs/modules/epaper/prd.md#erase)–[REQ-18](../../../.docs/modules/epaper/prd.md#pen-buttons);
[infini 0.5.0-draft](../../../.docs/modules/infini/prd.md) [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map).
MoSCoW: [prioritization-iter-005-draft.md](../prioritization-iter-005-draft.md).

## Review-prd

**Verdict: READY-WITH-CONCERNS** (as *draft for the next campaign*, not as TRACK-004 scope)

| Class | Finding |
|---|---|
| Strength | Outcome-first REQs; Must cluster is paper-like edit (erase, clipboard, connector decorate, buttons). |
| Strength | D9 button model: one catalogue item per Click / Hold-move; no 3-in-1 hold. |
| Concern | [REQ-10] two-finger pan/zoom **conflicts with BRD-07** until analyst amends. Same REQ as one-finger pick/move; [REQ-16] retired. |
| Concern | [REQ-15] tables vs EXP-0002 ≤2% FP — Could until corpus exists. |
| Concern | [REQ-17] manual create vs “no general palette” — closed insert set only. |
| Gap (accepted) | **AI** unspecified — no REQ. |
| Gap (accepted) | No SRS yet — **do not decompose until retro-gate + human says start iter-005.** Orphans expected. |

## Asks

1. **Do not** write SRS / ADRs for REQ-11…18 as TRACK-004 work.
2. When iter-005 starts: decompose Must first (11, 12, 13, 14, 18 + infini 05). ADR candidates: viewport last-writer (16), clipboard ops, endpoint-ink membership, attachment `t`, button event channel, settings-vs-document publish.
3. Spike: eraser nib vs barrel bits.

## Constraints

- Vertical lock, verified stop, wip 2 — still IN-030 / remaining iter-004.
- SM: no stories for these REQs yet.
