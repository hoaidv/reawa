---
from: architect
to: sm
date: 2026-08-13
iter: iter-003
---

# Hand-off: Architect → SM — Device Log (TCP :9878)

## Context

Human locked **option 2**: Infini **Device Log** button → full-size **in-app** overlay
(in-memory buffer, filter/search); Epaper ships console logs **when requested** on
**TCP 9878**, off the ink/GUI thread. Thin SRS only. **No ADR** (human: unless we must —
we must not: mixing into `:9877` is lock-forbidden, so the port split is already decided).
[ADR-0015](../../../.docs/adr/ADR-0015-one-way-sync-contract.md) is **unamended**.

This is so a human can inspect EP-012…016 (enclose / ingest / sync) without a document
channel, `doc_change`, or Smart Group change.

### New SRS ids

| Id | File | Covers | Parent REQ |
|---|---|---|---|
| `SRS-IN-17` | [tablet-sync/srs-logic.md](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md#srs-in-17-debug-log-channel) | `:9878` JSON-lines family, request/start/stop, never applied to VectorDocument | infini `[REQ-03]` |
| `SRS-IN-18` | [tablet-sync/srs-ui.md](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md#srs-in-18-device-log-panel) | Device Log button + overlay states: closed, open/empty, streaming, disconnected, filtered | infini `[REQ-03]` |
| `SRS-IN-19` | [tablet-sync/srs-quality.md](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md#srs-in-19-debug-log-isolation) | 0 log I/O on paint/apply; drop under backpressure; 0 tree mutations | infini `[REQ-03]` |
| `SRS-EP-15` | [device-document/srs-logic.md](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-15-debug-log-ship) | Env-gated ship of `qInfo`/`qWarning`/`qCritical` + stdout/stderr; worker/queued; `[enclose]` log *source* | epaper `[REQ-07]` |
| `SRS-EP-16` | [device-document/srs-quality.md](../../../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-16-debug-log-ship-quality) | 0 log I/O on paint/`ingestPoint`; drop oldest; recognizer fixtures unchanged | epaper `[REQ-07]` |

Indexes updated. Architecture views note the sidecar (not a new contract). `lifecycle: active`.

### Closed message types — TCP **:9878** only

| type | Direction |
|---|---|
| `debug_request` | Infini → Epaper |
| `debug_start` | Infini → Epaper |
| `debug_stop` | Infini → Epaper |
| `debug_log` | Epaper → Infini |

Envelope for `debug_log`: `{ type, ts, level, logger, msg, dropped }`. Canonical in SRS-IN-17.

**Not on :9877.** SRS-IN-07 retired/rejected and SRS-EP-08 classification reject `debug_*` on
the document socket. Do **not** add them to ADR-0015.

### CHL

**None.** SRS-IN-02 already lists tablet-sync chrome as out of canvas inventory.

## Review (readiness)

**Verdict: READY-WITH-CONCERNS**

| Class | Finding |
|---|---|
| Strength | Two sockets, two decoders — ADR-0015 message set stays closed; debug cannot become a load/change |
| Strength | Ink path: 0 log I/O on paint/`ingestPoint`; worker + bounded drop; env default off |
| Strength | `[enclose]` is a `qInfo` source after ingest dispatch — 0 recognizer change |
| Concern | No ADR for `:9878` — **accepted**. Human option 2 + lock `forbidden: mixing debug into :9877` |
| Concern | `adlc audit` will show these five SRS as orphan (no stories/BDD/code) — **expected**; you slice next |
| Concern | stdout/stderr is required-if-capture-succeeds, not unconditional — Qt handler is the hard bar |

`needs_design: false` on SRS-IN-18. Do **not** open a Designer story unless the human
rejects the overlay.

## Asks

1. **Slice Infini as protocol then UI.** IN-17 (listen `:9878`, decoder, 10k ring, IPC) can
   land without chrome; IN-18 is the button + overlay. Put IN-19 bars in those stories.
2. **Slice Epaper as shipper then log source.** EP-15 worker + Qt handler + env is the
   feature. The `[enclose]` line is a **logging hook at the ingest call site**, not an
   ink-box / `recognize_enclose` / `create_smart_group` story. EP-16 bars ride along.
3. **Do not schedule Designer.** Overlay states are specified. Canvas SRS already deferred
   tablet-sync chrome.
4. Keep EP-012…016 inspectable: shipping must be requestable while those stories are in
   verify — env `EPAPER_DEBUG_LOG` + Device Log button.

## Constraints

- **Do not amend ADR-0015.** Do not add `debug_log` to the `:9877` table.
- **Do not edit** Smart Group / enclose / `create_smart_group` logic (SRS-EP-10 or code).
- Residue EP-007…011, EP-017 membership, on-panel undo chrome, mixing debug into `:9877`
  stay forbidden.
- Ink latency floor (SRS-EP-01 ≤30 ms) outranks the shipper.

## Out of scope

- Document channel, `doc_change`, preview `stroke_*` on `:9878`
- Persist-to-disk / export / second window
- On-device Device Log UI
- New REQ-09
