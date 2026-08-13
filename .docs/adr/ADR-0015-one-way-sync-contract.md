---
id: ADR-0015
title: One-way sync contract v1 — message set, reconnect, and change ordering
status: accepted
date: 2026-08-13
deciders: [architect, pm]
supersedes: null
amends: [ADR-0009]
source: CHL-0008
---

# ADR-0015 — One-way sync contract v1

## Context

[ADR-0014](./ADR-0014-document-ownership-inversion.md) settles *who writes*. This ADR settles *what
crosses the wire*, because the pilot's instability came as much from the message set as from the
authority model: `doc_snapshot` was sent after desktop edits, on reconnect, and on orientation
change, which meant an inbound message could overwrite the device's picture at almost any moment.

Product contract to satisfy ([epaper REQ-07](../modules/epaper/prd.md#one-way-sync),
[infini REQ-03](../modules/infini/prd.md#tablet-sync)):

- Desktop → Tablet: **initial full-document load** + **pan/zoom viewport**. Nothing else.
- Tablet → Desktop: **document changes**.
- Editing works with the link down; changes queue in order and publish on reconnect.
- Mirror converges ≤300 ms p95 after an op; **0** inbound document messages after the initial load.

Existing shipped transport ([SRS-IN-07]): JSON-lines over TCP `:9877`, `viewport` /
`doc_snapshot` / `stroke_*`.

## Decision

### 1. Two directions, four message families

| Direction | Family | Messages | Cardinality |
|---|---|---|---|
| Desktop → Tablet | **Load** | `doc_load` | Once per session start / reconnect / explicit resync |
| Desktop → Tablet | **Viewport** | `viewport` | Continuous, coalesced ≤30 Hz |
| Tablet → Desktop | **Change** | `doc_change` | One per committed op |
| Tablet → Desktop | **Preview** | `stroke_begin` / `stroke_point` / `stroke_end` | Continuous during a stroke |

Anything else arriving is a **protocol defect**: logged, surfaced in the session status, and
dropped. Not tolerated silently, because silent tolerance is how the pilot's extra snapshots went
unnoticed.

`doc_load` replaces the old `doc_snapshot` name deliberately — a different name for a different
guarantee, so no code path can keep the old "snapshot is truth, at any time" behaviour by accident.

### 2. Changes are an ordered, idempotent op stream — not coalesced state

Answering the PM open question on granularity: **op stream**, one `doc_change` per committed
gesture.

```text
{ "type": "doc_change", "seq": n, "opId": "<uuid>", "op": { … }, "baseSeq": n-1 }
```

| Field | Meaning |
|---|---|
| `seq` | Monotonic per session, assigned by the device |
| `opId` | Stable, unique — the idempotency key |
| `op` | An ADR-0010 tree op (`create_smart_group`, `set_smart_transform`, `append_ink`, `reparent`, `remove`, `restore_snapshot`) |
| `baseSeq` | The `seq` the device believed current when committing — lets the mirror detect a gap |

Rationale: the ≤300 ms mirror target is comfortable for one message per gesture (gestures are
human-paced), and an op stream keeps the desktop's existing idempotent-apply-by-`opId` machinery
([SRS-IN-04]) rather than inventing a diff format. Coalesced state would have to be recomputed on
every op *and* would lose the undo unit, which is per-gesture by product rule.

`restore_snapshot` is how undo publishes: the device already restores wholesale (ADR-0014 §5), so it
emits the restore as an op rather than trying to synthesize inverse ops it does not compute.

### 3. Ordering, gaps, and idempotency

| Rule | Value |
|---|---|
| Order | Strict `seq` order; TCP gives this in-session, `baseSeq` detects a violation across a reconnect |
| Duplicate `opId` | Applied once; second apply is a no-op ([SRS-IN-04] already guarantees this) |
| Gap detected (`baseSeq` ≠ last applied `seq`) | The mirror is unsafe → desktop requests an explicit resync rather than guessing |
| Unknown `op` type | Log, do not crash, mark the mirror **suspect** and surface it — a suspect mirror must not be saved silently |

### 4. Queue, reconnect, and the load handshake

The device keeps an ordered, unbounded-in-practice publish queue. The handshake exists to make
"replace the local document" provably safe:

```text
Tablet                                  Desktop
  |-- hello { lastSeq, queued: k } ------>|
  |                                       |  k > 0 ?
  |<-- drain_ack (accepting changes) -----|
  |-- doc_change × k -------------------->|   (in order)
  |-- queue_empty ----------------------->|
  |<-- doc_load { document, seq: 0 } -----|   (only now)
  |-- load_ack --------------------------->|
```

| Rule | Value |
|---|---|
| `doc_load` legality | Only after `queue_empty`, or when the device reported `queued: 0` |
| Unsolicited `doc_load` mid-session | Rejected by the device; logged as a protocol defect |
| Load applies | Wholesale replace; `seq` resets to 0 for the new session epoch |
| Queue while disconnected | Retained in memory; publishes on the next `drain_ack` |
| Queue lost (app restart) | Unpublished work is gone — accepted (ADR-0014 Consequences); the next load restores what *was* published |

Why the queue drains *before* the load rather than the desktop merging: merging is
multi-directional sync, which is explicitly deferred. Draining first gives the same outcome for the
only case that exists in v1 — a single writer whose changes the mirror has simply not seen yet.

### 5. Viewport is unchanged and stays privileged

`viewport` keeps its shipped shape and coalescing ([SRS-IN-07]): ≤30 Hz, latest wins, `settle: true`
on flush, `drawingRegion` + `orientation` included. It is applied to the input→world map
**immediately** and never blocks on a repaint. Viewport carries **no** document content — this is
what makes "0 inbound document messages" checkable by message type alone.

### 6. Preview strokes are advisory in the one direction that is safe

The device keeps streaming `stroke_*` upward so the desktop feels live. The desktop renders them as
**transient preview paths** keyed by stroke id, and replaces the preview with the real node when the
`doc_change` carrying it arrives.

This is the one place an advisory picture survives the rework, and it is safe for the exact reason
the ghost was not: the previewer (desktop) is not the authority, so a preview being wrong costs a
repaint, not a corrected gesture.

| Rule | Value |
|---|---|
| Preview lifetime | From `stroke_begin` until the matching `doc_change` lands, or session drop |
| Orphaned preview (no change arrives) | Dropped on the next load or after a bounded timeout — never persisted |
| Preview → mirror | Preview paths are **never** written to the mirror or saved |

### 7. Message ids — closed set for v1

| id | Direction | Channel | Notes |
|---|---|---|---|
| `viewport` | D→T | viewport | Unchanged from [SRS-IN-07] |
| `doc_load` | D→T | load | Replaces `doc_snapshot`; handshake-gated |
| `hello` | T→D | session | Carries `lastSeq`, `queued` |
| `drain_ack` | D→T | session | Desktop ready to ingest the queue |
| `queue_empty` | T→D | session | Queue drained; load is now legal |
| `load_ack` | T→D | session | Load applied; publishing resumes |
| `doc_change` | T→D | change | One per committed op |
| `stroke_begin` / `stroke_point` / `stroke_end` | T→D | preview | Existing shape retained |

**Retired:** `doc_snapshot`, `doc_snapshot.pickables`, `tool_intent`, `stroke_begin.intent`
(see [SRS-IN-13]). **Still rejected:** `region_refresh` (PNG).

## Consequences

- The desktop's inbound path grows a real applier with gap detection; today it appends WorldLayer
  primitives from strokes and nothing else. This is the largest desktop change.
- "0 inbound document messages after load" is a **testable invariant by message type**, not a
  judgement call — it belongs in `srs-quality` as a trace assertion.
- Reconnect becomes a handshake rather than a reflex snapshot push. More states, but each one is
  legible; the pilot's "resend snapshot on reconnect" ([SRS-IN-07] Errors) is exactly what clobbered
  device work.
- A suspect mirror is a visible state. The desktop must not save a mirror it knows is gapped.
- Migration to multi-directional sync later replaces §2–§4 and keeps §1, §5, §6.

## Risks

| Risk | Severity | Mitigation |
|---|---|---|
| Queue grows unbounded during a long disconnect | Low | Ops are per-gesture and small; bound it and surface pressure before it matters |
| `restore_snapshot` ops are large (whole-document) | Medium | Only emitted on undo; measure against the ≤300 ms target and split if needed |
| Gap detection fires spuriously and triggers resyncs | Medium | Resync is visible and safe (queue drains first); tune `baseSeq` handling with trace evidence |
| Preview and real node briefly both visible | Low | Keyed by stroke id; replace, do not append |

## Alternatives Considered

| Approach | Convergence | Wire cost | Complexity | Notes |
|---|---|---|---|---|
| **Ordered op stream + handshake-gated load** | + | small, per gesture | moderate | **Chosen** |
| Coalesced change sets (periodic diff) | + | smaller under bursts | − recompute + loses the undo unit | Rejected — undo is per-gesture by product rule |
| Keep `doc_snapshot`, just stop sending it after edits | + | small | + trivial | Rejected — same name, same code path, same accident waiting to recur |
| Full document push on every change | + | − large | + trivial | Rejected — ≤300 ms target on a USB link with growing documents |
| Bidirectional `doc_op` now (ADR-0009 target) | + | small | − needs conflict rules | Deferred — that is multi-directional sync |
| No preview stream (desktop updates at pen-up only) | + | smallest | + simplest | Rejected — regresses the shipped ≤50 ms desktop liveness |
| Merge queued changes into the load instead of draining first | + | small | − that is a merge algorithm | Rejected — deferred scope, and unnecessary with one writer |
