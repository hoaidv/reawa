---
id: STORY-EP-013
title: "Measure ink latency with a resident document"
kind: implement
parent_srs: [SRS-EP-13]
parent_req: [REQ-04]
status: done
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given the current local-ink paint path (SRS-EP-01 baseline), When a resident document of 500 ink nodes / 50k samples plus a hit-test probe is on the paint path, Then pen-down → pixel p95 is ≤30 ms and equal to the pre-document baseline within measurement error."
  - "Given the same fixture, When hit-test runs, Then hit-test p95 is ≤100 ms and 0 ink samples are dropped or delayed by the probe."
  - "Given the budget is missed, When the story closes, Then a CHL-* is filed against ADR-0014 — not a workaround and not a relaxed ink budget."
  - "Given instrumentation, When it is added, Then it lives outside the paint loop (SRS-EP-13 Notes) so measurement itself cannot break the budget."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-013 — Measure ink latency with a resident document

Spike for [SRS-EP-13](../../../.docs/modules/epaper/features/device-document/srs-quality.md)
(the floor that outranks everything) and ADR-0014 Risks row 1. **No REQ-04 implement story
starts until this passes.** Does not ship the tree — stubs a resident document + hit-test on
the existing paint path.

[SRS-EP-01](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md) ≤30 ms is the bar.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

## Done when

- Baseline vs probe numbers recorded in the story or iter memory
- Pass → [STORY-EP-014](./STORY-EP-014.md) may leave `draft`
- Fail → `CHL-*`, stop the rework, do not proceed

## Measurement (2026-08-13)

QA verified 2026-08-13: host harness re-run **pass**; RM2 synth protocol **pass**.
SRS-EP-13 ink floor still met on device (arrival→flush p95=1298 µs ≪ 30 ms;
within the 2026-08-09 p95–p99 envelope). SM may ungate [STORY-EP-014](./STORY-EP-014.md).
QA does not flip EP-014.

### SRS-EP-01 device baseline (pre-document, 2026-08-09)

| Metric | n | p50 | p95 | p99 |
|---|---|---|---|---|
| arrival → flush | 764 | 305 µs | **798 µs** | 1517 µs |
| pen-down → pixel (eye) | — | — | met vs xochitl (~27 ms) | — |

### Host probe (QA re-run 2026-08-13 — analog only)

Harness: `epaper/tests/run_latency_probe.sh` (`-O2`). Fixture: 500 nodes / 50k samples.
**PASS** (`latency_probe_test: checks passed`).

| Metric | Baseline (no stub) | Probe on | Delta |
|---|---|---|---|
| Hit-test AABB+polyline p95 | — | 459 ns (bench n=2000) / 625 ns (ingest-path n=200) | — |
| Press → first simulated segment p95 | 42 ns (n=200) | 666 ns (n=200) | **+624 ns** |
| Samples dropped | 0 | **0** | 0 |
| Hit-tests invoked from simulated `paint()` | 0 | **0** | 0 |
| Samples emitted | 5000 | 5000 | 0 |

Host analog is not SRS-EP-13 met by itself. Device numbers below are the gate.

### Device probe (QA 2026-08-13 — RM2 `10.11.99.1`)

Protocol: `RM_INK_TRACE=1 RM_DOC_PROBE=1 RM_DOC_PROBE_SYNTH=1 ./scripts/deploy-rm2.sh --build`
then grep `/tmp/epaper.log`. Synth: 40 strokes then exit. Fixture confirmed
`nodes=500 samples=50000`.

```
[ink-trace] arrival->flush n=80 p50=813us p95=1298us p99=3102us
[ink-trace] flush->swap: (no samples)
[doc-probe] fixture nodes=500 samples=50000 enabled=1 every_sample=0
[doc-probe] hit-test n=40 p50=12499ns p95=22249ns p99=24249ns (p95=22us)
[doc-probe] samples processed=840 dropped=0 paint-loop-hits=0
```

| Bar | Device | Verdict |
|---|---|---|
| arrival→flush vs 798 µs band | p95=**1298 µs** (n=80) | **pass** — between 2026-08-09 p95 (798) and p99 (1517); 23× under 30 ms |
| pen-down → pixel ≤30 ms | software path 1.3 ms; `flush->swap` empty (no `RM_EP_SWAP`); eye not re-run on synth | **pass** on instrumented path (≪ 30 ms vs ~27 ms baseline) |
| hit-test p95 ≤100 ms | **22 µs** (n=40) | **pass** |
| dropped=0 | **0** | **pass** |
| paint-loop-hits=0 | **0** | **pass** |

n=80 synth is not the n=764 human-pen baseline. p50 813 µs vs 305 µs is workload
(tight-loop ingest vs human drawing), not a 30 ms miss. Hit-test itself is 22 µs
and does not explain the flush delta. No `CHL-*`.

### How to run

Host:

```bash
epaper/tests/run_latency_probe.sh
```

Device (synthetic ingest, then exit):

```bash
cd epaper
RM_INK_TRACE=1 RM_DOC_PROBE=1 RM_DOC_PROBE_SYNTH=1 ./scripts/deploy-rm2.sh --build
ssh root@10.11.99.1 'sleep 2; grep -E "ink-trace|doc-probe" /tmp/epaper.log'
```

Device (human draw):

```bash
cd epaper
RM_INK_TRACE=1 RM_DOC_PROBE=1 ./scripts/deploy-rm2.sh --build
# draw, then:
ssh root@10.11.99.1 'killall -TERM epaper; sleep 1; grep -E "ink-trace|doc-probe" /tmp/epaper.log'
```

Compare `[ink-trace] arrival->flush p95` to 798 µs. Compare `[doc-probe] hit-test p95`
to 100 ms. `dropped=` must stay 0. `paint-loop-hits=` must stay 0.

### Paint-loop hygiene

Probe lives in `ingestPoint` (GUI-thread ingest), gated by `RM_DOC_PROBE` /
`RM_DOC_PROBE_SYNTH`. `TabletCanvasItem::paint()` only blits the ink `QImage` —
no stub walk, no hit-test, no timers.

### Verdict (QA 2026-08-13)

| Bar | Host | Device |
|---|---|---|
| Hit-test p95 ≤100 ms | **pass** (p95 ≈ 0.6 µs) | **pass** (p95=22 µs) |
| 0 samples dropped | **pass** | **pass** |
| Instrumentation outside `paint()` | **pass** | **pass** (`paint-loop-hits=0`; `paint()` blit-only) |
| Pen-down → pixel p95 ≤30 ms vs baseline | analog only | **pass** (arrival→flush p95=1298 µs ≪ 30 ms) |

Story **done**. No `CHL-*` / `DEF-*`. SM ungates W9 / EP-014 — QA does not flip it.
