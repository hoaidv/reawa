---
id: TRACK-008
slug: rebuild-epaper
kind: expedite
status: active
iter: iter-005
goal: "Archive the current Epaper tree and replace it with a minimum reMarkable 2 build ceremony and an empty main."
scope:
  - epaper/local-pen-ink
stories:
  - STORY-EP-081
cursor: "WAIT human — shell is built. Name the next slice. Do not port behavior until then."
paused_reason: ""
interrupts:
  - TRACK-007
---

# TRACK-008 — Rebuild Epaper

## Goal

The human judged the current Epaper tree too tangled to extend. Keep it as `epaper_old/` and start a new `epaper/` that cross-compiles an empty Qt application for the reMarkable 2.

## Scope

- In: repo-root rename, Docker SDK ceremony, CMake, empty `main`, build script, ignore rule for the SDK installer.
- Out: product behavior, product documents, Infini, deleting the archive, follow-through stories.

Parent records for the shell story only: [REQ-01](../../.docs/modules/epaper/prd.md#local-pen-ink) (Local pen-matched ink) and [SRS-EP-01](../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md#srs-ep-01) (Pen event path, coordinate map, and Pen-mode refresh). The shell does not implement them.

## Stories

| ID | Status | Notes |
|---|---|---|
| [STORY-EP-081](../iter-005/stories/STORY-EP-081.md) | done | Empty reMarkable 2 shell · ARM binary on the host |

## Cursor

**WAIT.** [STORY-EP-081](../iter-005/stories/STORY-EP-081.md) (Empty reMarkable 2 shell) is **done**. Human names the next slice. Do not port drawing, sync, or tools until then.

## Execution board

[iter-005/execution-board-rebuild-epaper.md](../iter-005/execution-board-rebuild-epaper.md)

## Log

| Date | Event |
|---|---|
| 2026-10-03 | Opened as expedite. Interrupts [TRACK-007](./TRACK-007-follow-through.md). Human ordered archive + empty shell. |
| 2026-10-03 | [STORY-EP-081](../iter-005/stories/STORY-EP-081.md) done. Host ARM binary produced. Cursor waits for the human. |
