---
from: pm
to: sm
iter: iter-002
date: 2026-08-11
subject: w5-gate
verdict: READY-WITH-CONCERNS
cc: [qa, architect, dev]
---

# Gate review — W5 (live viewport + region picture)

## Verdict: **READY-WITH-CONCERNS**

W5 Must stories **IN-011** + **EP-002** are `done` (QA PASS-WITH-CONCERNS). Product docs
match shipped code (code-truth rewrite). Human exercised hardware through the W5 bug-fix
loop (pen axes, gut L/R, zoom stroke thickness, vector settle). Campaign Must exit criteria
(REQ-01…03 + Epaper REQ-02) are **met with owned gaps** — not a clean `adlc gate --check`.

## Scope checked (lock)

```
direction: vertical · modules: infini, epaper · stop: verified · wip: 1
features: infinity-canvas; vector-document; tablet-sync; region-sync
```

| REQ | Feature | Evidence | Status |
|---|---|---|---|
| Infini REQ-01 | infinity-canvas | IN-001…005 done | **met** |
| Infini REQ-02 | vector-document | IN-007/008 + library; live WorldLayer | **met** (chrome deferred) |
| Infini REQ-03 | tablet-sync | IN-009 + IN-011; viewport/`doc_snapshot`/`stroke_*` | **met** (interim wire) |
| Epaper REQ-02 | region-sync | EP-001 + EP-002; Qt gut UV + vector rasterize | **met** (`regionsync/` library follow-up) |
| Infini REQ-04 | Smart Group Could | IN-010 draft | **out of Must** |

`validated_by: human / 2026-08-11 — W4 draw + W5 hardware bug-fix loop (axes, gut L/R, stroke×zoom, settle); see pm-gate-review-w5`

## Story rollup (W5)

| Story | Status |
|---|---|
| STORY-IN-011 | done |
| STORY-EP-002 | done |
| STORY-IN-010 | draft Could — **parked** (do not unpark without PM wave) |
| STORY-IN-006 | blocked (cancelled) |

## Evidence chain

| Source | Result |
|---|---|
| QA | [qa-to-pm-w5](./2026-08-11-qa-to-pm-w5.md) PASS-WITH-CONCERNS |
| SM | [sm-to-pm-code-truth](./2026-08-11-sm-to-pm-code-truth.md) — no new Must slices |
| Architect | code-truth SRS READY-WITH-CONCERNS |
| Human loop | Digitizer restore; vector `doc_snapshot`; gut 4-pose + L/R flip; world×panel live ink |

## Engine (`adlc gate`) — disposition

**Overall:** FAIL (repo-wide). Lock checks: **PASS**. Campaign-relevant vs noise:

| FAIL | Disposition |
|---|---|
| Orphan SRS-IN-05 / IN-06 | **Owned** — DocChrome cancelled |
| Orphan SRS-IN-10 / Needs-design REQ-04 | **Owned** — Could; IN-010 parked |
| SRS-EP-01 / quality SRS without dedicated story | **Owned** — covered by EP-001/002 + sibling BDD |
| tablet-sync `srs-ui` without design story | **Owned** — REQ-03 Needs design: **no**; marker affordance only |
| `reawa/*` design/BDD gaps | **Out of lock** — do not block |

`prd-check` epaper: **ok**. Infini: open-question WARN only. Lock: **PASS**.

## Concerns (do not block W5 gate)

1. **Interim wire** — shipped `stroke_*` + `doc_snapshot`; target ADR-0009 `doc_op` deferred.
2. **`regionsync/` unwired** on device — Qt canvas is production path.
3. **Dual SoT** — WorldLayer live vs `VectorDocument` library.
4. **Reconnect hello** still TBD (resend snapshot on connect today).
5. **REQ-04 / IN-010** unpaid design until pilot opens.
6. Residual: optional explicit “all four guts green” ack after final L/R deploy (loop evidence accepted).

## Decisions (PM)

| Topic | Decision |
|---|---|
| Unpark IN-010 | **No** — until explicit PM wave |
| `doc_op` migration | **Defer** — backlog / next campaign slice |
| DocChrome | Stay cancelled |
| Campaign Must exit | **Met** (READY-WITH-CONCERNS) |

## Ask of SM

1. Mark W5 **gated** on [execution-board](../execution-board.md); TRACK-002 cursor → campaign close or next slice ask.
2. Update MASTER `validated_by` already set by PM — refresh board Verdict.
3. Ask human: **close iter-002** (retro) vs open next wave (migration / Smart Group / other).

## Next

**`/sm`** — board + track. Then human chooses: **`/pm` retro-gate / close iter** or `/campaign` for next direction.
