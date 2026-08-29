---
persona: sm
captured: 2026-08-29
trigger: surprise
iter: iter-005
related:
  - STORY-EP-063
  - STORY-EP-064
  - STORY-EP-067
  - SRS-EP-55
  - SRS-EP-56
promoted_to: .docs/memory/erase-brush-commit.md
---

# Brush erase: commit failed on duplicate remnant ids, not a clip miss

## What happened

Human reported brush erase sometimes no-op on long lines (ghost correct, commit fails; handwriting 100%; easier from ends; works after erasing other drawings). First agent guess was uniform 0.25 mm clip scan missing 2-point strokes. Device log showed `reason "duplicate_id:s-3_r1"` with a 57-point eraser path — clip had already hit. Second nick of the same stroke reused `{id}_r1`. Skip-taken remnant ids fixed it. Stadium-interval clip was rolled back as an incorrect guess. Human confirmed brush fully works 2026-08-29.

## What I learned

- **Worked:** Trust commit `reason` over a geometric story. Ghost is ToolCanvas paint; commit is `planEraseEdits` + `requireUnique`.
- **Didn't work:** Treating “ghost overlaps, tree unchanged” as a clip miss. That pattern is also a rolled-back compound (`applied: false`).
- **Do differently:** Device-authored ids must come from one allocator ([STORY-EP-067](../stories/STORY-EP-067.md)). Do not private-concat `{id}_rN` per caller. Do not add clip complexity without a `noop` log.

## Promotion note (iter close only)

Promoted immediately — human asked for session memory; the id-collision fact is needed for sync.
