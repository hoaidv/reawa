---
id: STORY-EP-081
title: Empty reMarkable 2 shell
kind: implement
parent_srs: [SRS-EP-01]
parent_req: [REQ-01]
status: done
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given the previous repo-root epaper tree, When this story lands, Then that tree is at repo-root epaper_old, including uncommitted work and the local SDK installer, and nothing in it was deleted."
  - "Given the new repo-root epaper tree, When its sources are listed, Then the application entry is an empty Qt main and the tree contains no drawing, document, sync, tool, or test code from the archived tree."
  - "Given Docker and the reMarkable SDK installer, When epaper/scripts/build.sh runs, Then it produces an ARM executable at epaper/build/bin/epaper."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-081 — Empty reMarkable 2 shell

Human 2026-10-03: the current Epaper tree is too tangled to extend. Archive it and start again from the build ceremony plus an empty `main`.

This story does **not** implement pen ink. [SRS-EP-01](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md#srs-ep-01) (Pen event path, coordinate map, and Pen-mode refresh) and [REQ-01](../../../.docs/modules/epaper/prd.md#local-pen-ink) (Local pen-matched ink) stay the parent records so the shell is traceable to the module it will eventually serve. Behavior from those records remains in `epaper_old/` until a later story brings it back.

## Architectural change trace

- Reason: human ordered a from-scratch Epaper tree on 2026-10-03.
- Affected design: repo-root `epaper/` becomes a cross-compile shell. The previous tree moves to `epaper_old/`.
- Consequences: follow-through implementation on the old tree stops ([TRACK-007](../../tracks/TRACK-007-follow-through.md) paused). Product documents are unchanged.
- Implementation: done — `epaper_old/` holds the previous tree; new `epaper/main.cpp` is an empty `QGuiApplication`.
- Verification: done on the host, 2026-10-03 — `file epaper/build/bin/epaper` reports ELF 32-bit ARM EABI5. Not launched on a tablet.

## Kind

| Field | Value |
|---|---|
| Kind | implement |
| Owner | Developer |
| Depends on | none (no user interface) |

## Out of scope

- Porting drawing, tools, sync, or tests
- Editing product documents
- Deleting `epaper_old/`
- A commit, unless the human asks

## Done when

- The three acceptance criteria hold.
- Host verification 2026-10-03: `epaper/build/bin/epaper` is ELF 32-bit ARM EABI5. Tablet launch was not run.
