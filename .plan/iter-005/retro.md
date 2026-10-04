---
iter: iter-005
date: 2026-10-04
status: complete
participants: analyst, pm, architect, sm, designer, dev, qa
---

# Iter 005 Retrospective

> SM close-iter 2026-10-04. The human asked to close iter-005 and open iter-006 for the
> document-forest implementation ([ADR-0041](../../.docs/adr/ADR-0041-document-forest.md)).
> Persona reflections were collected from one read-only sub-agent per persona. PM retro gate
> ran in the same session.

## What went well

- Hand-touch, inverse-op undo, three erasers, clipboard, Path B endpoint ink and nested ink-boxes
  were built and **human-verified on the reMarkable 2** (2026-08-20 … 2026-09-05).
- Inverse-op undo (EP-059…061) wrote its scenarios before the code; all 21 mapped one-to-one to
  host tests.
- Challenges the human decided went from `adopted` to an architect handoff the same day
  (CHL-0026, CHL-0028, CHL-0031, CHL-0032).
- Track pivots (TRACK-005 → 007 → 008) left nothing in flight: each had freeze notes and an
  explicit WAIT cursor.
- When the tree proved too tangled, the human archived it as `epaper_old/` and the new shell
  (EP-081) produced an ARM binary. ADR-0041 was then designed, reviewed and accepted in docs
  before any forest code.

## What to improve

- **Capacity.** 136 points committed against 72, stories EP-059…081 added mid-iter without a
  re-baseline, and no `end` date. A rescope should close the iter.
- **Challenge triage.** CHL-0022, CHL-0023 and CHL-0027 sat open six to seven weeks while their
  SRS text contradicted shipped behavior. Deferred at this close.
- **Substrate.** ADR-0039 and ADR-0040 were layered on a tree whose by-value children and
  transform-based resize ADR-0041 later named as the root problem.
- **QA cadence.** After 2026-08-27 there were no QA verdict handoffs; several stories were verified
  only by hand on the device, and a red host check (`kMinEncloseWorld == 28`) was never filed.
- **Code without a story.** `epaper/` grew past the empty shell of EP-081 (native window, stylus
  handler, pen and finger ink) with no story behind it. iter-006 stories start from that code.
- **BRD drift.** BRD-07 was last reviewed 2026-08-20 and does not yet show Infini sync deferred.
  Analyst follow-up in iter-006.
- **Design system.** 41 iter-005 system files (3 components, 38 icons) were never promoted. They
  describe the archived application; see *Memory captured*.

## Iter memory reviewed

- [design-index-iframe-preview.md](./memory/design-index-iframe-preview.md) — promoted (project)
- [erase-brush-duplicate-remnant-ids.md](./memory/erase-brush-duplicate-remnant-ids.md) — already
  promoted 2026-08-29 to [erase-brush-commit](../../.docs/memory/erase-brush-commit.md)

## Memory captured

- **Project**
  - [design-index-iframe-preview](../../.docs/memory/design-index-iframe-preview.md) (designer)
  - [epaper-rebuild-carryover](../../.docs/memory/epaper-rebuild-carryover.md) (dev)
  - [epaper-host-gate-before-device-verify](../../.docs/memory/epaper-host-gate-before-device-verify.md) (qa)
- **ADLC** (`status: proposed`, waiting for human approval)
  - [scope-pivot-triggers-brd-check](../../.agent/memory/scope-pivot-triggers-brd-check.md) (analyst)
  - [challenge-triage-before-track-close](../../.agent/memory/challenge-triage-before-track-close.md) (pm)
  - [challenge-the-substrate-before-layering](../../.agent/memory/challenge-the-substrate-before-layering.md) (architect)
  - [rescope-closes-iter](../../.agent/memory/rescope-closes-iter.md) (sm)
- **Design system** — not promoted. `.plan/iter-005/design/system/` holds 3 components
  (`pen-map-button`, `pen-map-select`, `viewport-follow-toggle-infini`) and 38 icons that are
  missing from or differ from `.docs/design/system/`. They belong to the archived application,
  and iter-006 has no UI design, so they stay archived in place. Whether the
  `.docs/design/index.md` rows for those screens move from `current` to archived is a human or PM
  call.

## Upstream signals

- The iframe-preview defect needed a template and gate hotfix (landed 2026-08-20). Already fixed
  in the distribution copy; export with `adlc feedback export`.
- `adlc audit` does not scan `.cpp`, so C++ implementations of SRS records show as orphans.
- `adlc validate` reads a line beginning `SRS-<MOD>-NN:` as a second declaration, which is how
  SRS-EP-13, -16 and -33 come to be "defined twice" in `srs-quality.md`'s *Superseded* list.

## Persona reflections

- **analyst**: The BRD-07 change of 2026-08-20 went to the PM as an explicit old-versus-new list, and the PRDs and ADR-0023 followed the same day | The BRD was not revisited after the rebuild (10-03) or the sync deferral (10-04); record that deferral before iter-006 work leans on BRD-07 | ADLC: scope-pivot-triggers-brd-check
- **pm**: Each human decision went from challenge adopted to architect handoff with a verdict on the same day, so undo, erase, clipboard and nested ink-box were human-verified within days | CHL-0022/0023/0027 sat open 6–7 weeks while SRS text contradicted shipped behavior; 136 points against 72 capacity; ADR-0040 stayed proposed until superseded | ADLC: challenge-triage-before-track-close
- **architect**: Binds for ADR-0032…0039 were quick, each with a concerns review and a call-site inventory; ADR-0041 split into one domain file per algorithm, and reading both peers found drift from ADR-0020 on centre ends | ADR-0039/0040 built on by-value children and transform-based resize without questioning them; restate the requirement before drafting (the first forest draft solved editing during a walk, not concurrency) | ADLC: challenge-the-substrate-before-layering
- **sm**: Freeze notes and explicit WAIT cursors made the TRACK-005 → 007 → 008 pivots leave nothing in flight; every done story was human-verified | 136 against 72 points, EP-059…081 added without re-baseline, no `end` date, TRACK-006 ran without stories, ink-path code went past EP-081 without a story | ADLC: rescope-closes-iter
- **designer**: Six design packages shipped (hand-touch, pen-button map, both viewport-follow packages, pen-map as Epaper, empty-pan delta); the eraser reduced to three icons without a package | The blank-iframe preview bug shipped twice before the template, skills and gate were fixed; promote system work when done, not at iter close | Project: design-index-iframe-preview
- **dev**: Inverse undo EP-059…061 shipped with every scenario mapped to a host test; brush erase was diagnosed from the commit `reason`, not geometry guesses; the EP-081 shell built an ARM ELF | Dev handoffs skipped the device and Qt build; a stale test assert was carried forward; node ids were minted in four places until the tree was too tangled to extend | Project: epaper-rebuild-carryover
- **qa**: EP-059…061 had scenarios before code, all 21 mapped one-to-one to host tests, each asserting 0 `restore_snapshot` | No QA verdicts after 08-27; erase, clipboard, Path B and nested verified only by hand on the device; the red `dispatch_test` check was never filed as a defect | Project: epaper-host-gate-before-device-verify

## Carry-over to iter-006

| Item | Disposition |
|---|---|
| [STORY-EP-078](./stories/STORY-EP-078.md)…[STORY-EP-080](./stories/STORY-EP-080.md) | **cancelled** — implemented superseded ADR-0040; forest R-tree is [STORY-EP-086](../iter-006/stories/STORY-EP-086.md) |
| [TRACK-007](../tracks/TRACK-007-follow-through.md) stories: EP-048…052, EP-057, EP-058, EP-070…073 (`draft` / `ready`) | **carry, paused** — stay with TRACK-007; not committed to iter-006. Most target the archived application and need re-slicing when their feature is ported |
| [STORY-EP-045](./stories/STORY-EP-045.md), [STORY-EP-046](./stories/STORY-EP-046.md) | **blocked** (frozen 2026-09-04, Path A replaced by the erasers, [CHL-0028](./challenges/CHL-0028-eraser-three-tools.md)) — carry as frozen |
| [STORY-EP-035](../iter-004/stories/STORY-EP-035.md) | **carry** — parking lot |
| CHL-0022, CHL-0023, CHL-0027 | **deferred** 2026-10-04 (PM) |
| REQ-15, REQ-08, CHL-0012, EP-032 | **parked** |
