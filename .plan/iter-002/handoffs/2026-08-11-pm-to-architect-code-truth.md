---
from: pm
to: architect
iter: iter-002
date: 2026-08-11
subject: code-truth-prd-rewrite
cc: [sm, qa]
verdict: READY-WITH-CONCERNS
---

# PM → Architect — PRD rewritten to code SoT

## Verdict: **READY-WITH-CONCERNS**

Human directed: **code is source of truth** — rewrite PRD/SRS as needed. PM updated
Infini + Epaper PRDs; Architect owns SRS/ADR/architecture follow-through (done in same
session — see sibling handoff).

## What changed (product)

| Module | Change |
|---|---|
| [infini/prd.md](../../../.docs/modules/infini/prd.md) v0.2.0 | REQ-03 = shipped `viewport`/`doc_snapshot`/`stroke_*`, gut poses, world stroke width; REQ-02 = library + WorldLayer live; REQ-04 Smart Group = Could / library-only |
| [epaper/prd.md](../../../.docs/modules/epaper/prd.md) v0.3.0 | REQ-02 matches Qt path; `regionsync/` marked tests-only; no PNG refresh |

## Review (review-prd)

| Class | Finding |
|---|---|
| ✅ Strength | Must REQs measurable; Non-Goals explicit about interim wire |
| ⚠️ Concern | Dual SoT (WorldLayer vs tree) accepted until migration |
| ⚠️ Concern | Smart Group still Could with no UI — STORY-IN-010 parked |
| ⚠️ Concern | Human hardware confirm still open from W5 QA PASS-WITH-CONCERNS |

**prd-check:** Infini/Epaper 0 FAIL (Infini open-question WARN if any); Reawa unrelated WARNs.

## Ask

Architect: keep SRS/protocol/ADR aligned (completed). SM: do **not** invent new Must stories
from aspirational `@future` BDD until PM opens a migration wave.
