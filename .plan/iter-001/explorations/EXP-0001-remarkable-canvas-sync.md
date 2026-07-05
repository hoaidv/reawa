---
id: EXP-0001
goal: reMarkable as drawing tablet: draw on RM, view infinity canvas on macOS with synced drawing region
goal_source: chat
date: 2026-07-05
driven_by: pm
goal_from: human
initiative:
  - I1-rm-native-draw
  - I2-macos-infinity-viewer
  - I3-region-sync-protocol
status: paused-for-feedback
mode: checkpoint
timebox: 2h per round
token_budget: 80k per round
sandbox: .sandbox/EXP-0001-remarkable-canvas-sync (branch exp/EXP-0001-remarkable-canvas-sync)
brainstorm: none
participants: [pm, architect, dev]
iter: iter-001
relates-to: []
---

# EXP-0001 — reMarkable as drawing tablet: draw on RM, view infinity canvas on macOS with synced drawing region

## Goal (from human)

> **macOS:** Display infinity canvas  
> **reMarkable:** Synchronized drawing region ↔ macOS  
> Drawing happens on reMarkable (best writing feel); viewing on macOS (best viewing feel). Makes reMarkable a true drawing tablet.

**Success criteria (measurable):**

| # | Criterion | Target |
|---|---|---|
| S1 | Pen ink appears on RM screen locally (not relayed via macOS) | p95 ≤ 30 ms pen-down → pixel |
| S2 | Completed stroke visible on macOS canvas | p95 ≤ 50 ms after RM sample |
| S3 | macOS pan/zoom updates RM drawing region | p95 ≤ 100 ms viewport → RM full refresh |
| S4 | Brush width visually consistent across zoom levels | No >10% perceived width drift at 0.5×–2× zoom |
| S5 | SDK build + deploy loop works on this dev machine (ARM64 Mac) | Cross-compile + scp + run on connected RM2 |

Loop runs until human stops or S1–S5 met on real hardware.

## Initiatives & approaches (PM)

### I1 — RM native drawing app (`I1-rm-native-draw`)

Qt Quick + C++ `tabletEvent` app; local ink; e-paper tile grid; stops xochitl during session.

### I2 — macOS infinity canvas viewer (`I2-macos-infinity-viewer`)

Swift/AppKit (or SwiftUI+Metal) app: visible frame = window; drawing frame fixed center; pan/zoom changes world sampling only.

### I3 — Region sync protocol (`I3-region-sync-protocol`)

TCP JSON-lines (spike) → protobuf later. Sync **stroke data** RM→macOS; sync **viewport + stroke batch** macOS→RM on pan/zoom. Not pen-event relay (existing Reawa module).

**Candidate approaches** (riskiest / highest-value first):

| ID | Approach | Tests |
|---|---|---|
| **A** | x86_64 Docker SDK toolchain on ARM64 Mac → cross-compile RM Qt app → deploy via scp | S5 |
| **B** | RM `TabletCanvasItem` + 8×6 e-paper tile grid (dragly pattern) + local draw | S1 |
| **C** | macOS viewport model + mock sync client (no RM) | S2/S3 logic |
| **D** | End-to-end: RM draw → macOS view + macOS pan → RM re-render with brush_scale | S1–S4 |
| **E** | Alt: relay pen events to macOS, draw on Mac, push bitmap back to RM | Higher latency; fallback only |

## Priority check — architect + dev push back

- **Vital to the goal?** Yes — without **A** (SDK on this machine) nothing ships to RM; without **B** writing feel fails; **C** can parallelize.
- **Riskiest assumption?** SDK x86_64-only on ARM64 Mac host — requires Docker `--platform linux/amd64`; also pen input not documented in official Qt Quick tutorial (“marker is more involved”).
- **Timeboxable?** Round 1 (2h): Docker amd64 smoke test + protocol sketch + spike scaffold. Evidence: `uname -m` = x86_64 in container; hello_remarkable or calculator builds.
- **Verdict**: **proceed** — start with **A**, parallel **C** while SDK blocked.

## Loop log

### Round 1 — approach A (SDK toolchain) + protocol spike

- **Experiment** (architect + dev, sandbox):
  - Created worktree `.sandbox/EXP-0001-remarkable-canvas-sync` on branch `exp/EXP-0001-remarkable-canvas-sync`.
  - Researched reMarkable SDK docs: x86_64 Linux only; Qt Quick epaper backend; RM2 requires `systemctl stop xochitl`; pen via C++ `tabletEvent` + `evdevtablet` (not QML MouseArea).
  - Scaffolded `spike/docker/` (Dockerfile, compose, install-sdk.sh), `spike/protocol/viewport-sync.md`, RM + macOS spike READMEs.
  - Validated x86_64 emulation: `docker run --platform linux/amd64 ubuntu:22.04 uname -m` → `x86_64`.
  - Built `spike/docker` compose image (Ubuntu 22.04 + cmake/ninja); container reports `x86_64` + cmake 3.22.
  - SDK installer not present yet — cross-compile blocked until user drops `meta-toolchain-remarkable-*-x86_64-toolchain.sh` into `spike/docker/sdk-installer/`.
- **Assessment** (PM, vs goal): **partial** — host toolchain path proven (S5 prerequisite); protocol + module split defined; RM deploy and pen spike still open.
- **Adversarial check** (architect lens): Even with SDK working, official docs omit pen handling — **B** must be C++-first, not pure QML calculator clone. E-paper full refresh on every pan/zoom may miss S3 (100 ms) without tile grid. Confirmed.
- **Feedback gate**: **paused** — need RM device model + OS version (for SDK download), SDK installer in `spike/docker/sdk-installer/`, and checkpoint vs try-hard mode confirmation.
- **Adjust** (PM): Round 2 = (1) amd64 container smoke + SDK install if user provides installer, (2) start **C** macOS viewport mock on host (no Docker), (3) RM CMake skeleton with TabletCanvas stub.

## Outcome

- **Result**: _in progress_
- **Evidence**: _pending_

## Recommendation & routing

- **Decision** → ADR TBD (module split vs extend Reawa; stroke-sync vs event-stream)
- **Spec impact** → new module `.docs/modules/<name>/` via PM challenge after EXP achieves S1–S5 or downscope
- **Build path** → stories in iter-001 after greenlight

## Code disposition

- [x] Sandbox worktree created — `.sandbox/EXP-0001-remarkable-canvas-sync`
- [ ] Discard sandbox worktree (default) — after routing
- [ ] Promote to production via story(ies) — docs-first re-implementation
