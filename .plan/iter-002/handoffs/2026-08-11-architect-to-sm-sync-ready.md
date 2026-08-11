---
from: architect
to: sm
iter: iter-002
date: 2026-08-11
subject: sync-readiness-vector-doc
verdict: READY-WITH-CONCERNS
---

# Handoff: Architect → SM — Sync readiness (W3-arch)

## Verdict: **READY-WITH-CONCERNS**

Document model is bound to tablet/region sync channels. **Do not open `/dev` stories yet**
(human: implement with epaper↔desktop sync wave / W4).

## What changed

| Doc | Change |
|---|---|
| [tablet-sync SRS-IN-07](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md) | Channels, viewport msg, **emit matrix**, op envelope |
| [tablet-sync SRS-IN-08](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md) | Latency + idempotency + hash |
| [region-sync SRS-EP-02](../../../.docs/modules/epaper/features/region-sync/srs-logic.md) | World `append_ink`, no on-device Smart Group v0 |
| [region-sync SRS-EP-03](../../../.docs/modules/epaper/features/region-sync/srs-quality.md) | Channel fidelity + hot-path rule |
| [SRS-IN-09](../../../.docs/modules/infini/features/vector-document/srs-data.md) | Cross-link to emit matrices |
| Infini architecture | ADR list + sync bind + reconnect risk |

## Emit matrix (v0) — summary

| Peer | Document channel |
|---|---|
| **Epaper** | `append_ink` only (full tablet samples, world space) |
| **Infini** | Applies ops; emits structure / Smart Group / transforms when desktop edits; owns viewport channel |

Enclose recognition = **Infini-first**. Transport baseline = **JSON-lines**.

## Review findings

| | Finding |
|---|---|
| Strength | ADR-0009/10/11 + op catalog now have a clear peer split |
| Concern | Reconnect `snapshot`/`hello` still TBD before W4 ship |
| Concern | Dual TS+Qt fixtures not yet authored (expected pre-dev) |
| Risk | None blocking arch gate for “sync-ready to plan W4” |

## Ask of SM

1. Keep implement stories **uncreated / draft** until W4 (vector-doc + tablet-sync + region-sync together).
2. When opening W4, slice stories from SRS-IN-04/07/09/10 + SRS-EP-02 (and Smart Group Could).
3. Do not revive STORY-IN-006 design.

## Next

**`/sm`** — park W3-arch complete; schedule W4 when ready. No `/dev` now.
