---
from: pm
to: architect
iter: iter-002
date: 2026-08-11
subject: smart-group-pilot-req04
verdict: READY
---

# Handoff: PM → Architect — Smart Group pilot adopted

## Verdict: **READY** (pilot Could)

Human asked to pilot **Smart Group / ink-box**. Adopted as **[REQ-04](../../../.docs/modules/infini/prd.md#smart-group)**
(Priority **Could**) under Infini, not a Must blocker for F2 open/save chrome.

## Product intent (locked)

- Enclose handwritten ink with a rectangle → Smart Group (or explicit promote any ink set)
- **Ink preserved** (no OCR)
- Explicit boundary; move / scale (incl. non-uniform + aspect) / rotate
- `inkScaleMode`: `withBounds` | `fixedInk` (text-box feel)
- Connector target on boundary
- Best-effort recognition + undo + explicit fallback

## Next

Architect ADR + SRS (done in co-session as ADR-0011 / SRS-IN-10) → `/sm` to decide whether
pilot stories follow STORY-IN-006 or a later wave (wip 1: do not parallel another feature).
