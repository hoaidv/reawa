---
from: sm
to: dev
iter: iter-006
date: 2026-10-04
subject: document-forest — STORY-EP-082 ready
cc: [architect, pm, qa]
---

# Document forest — start with STORY-EP-082

The developer is the human. Agents guide, review and check evidence; they do not edit `epaper/src/`.

## Ready

[STORY-EP-082](../stories/STORY-EP-082.md) (Forest foundations: host tests and geometry), plan
[step 1](../../document-forest-implementation.md#step-1--foundations).

## Things to know before the first line

- **Qt must be optional for the core.** `epaper/CMakeLists.txt` calls
  `find_package(Qt6 REQUIRED …)` at the top. A host build of `epaper_doc` and its tests must not
  need Qt. One way: build `epaper_doc` and `tests/` unconditionally, and put the Qt executable
  behind an option or a `Qt6_FOUND` check. The reMarkable 2 build must still link afterwards.
- **`epaper/src/doc/` exists and is empty.** The layout proposal is in the
  [plan](../../document-forest-implementation.md#code-layout); adjust it freely in this story.
- **Sanitizer presets.** Thread and address sanitizers run on the host build only, not the ARM
  cross build. They cannot share one build directory; use one preset each.
- **Panel map.** Port the cases from `epaper_old/tests/canvas_frame_test.cpp`; the code is in
  `epaper_old/drawing/canvas_frame.hpp`.
- **Tests first.** Each acceptance criterion becomes a named `ctest` case
  ([host gate](../../../.docs/memory/epaper-host-gate-before-device-verify.md)). Trace tags need a
  short label (`@implements [SRS-EP-07] document core library`).

## How to use the agents

- `/dev`: review a diff against the story and the spec, or explain a part of the design.
- `/architect-unified`: a design question, or code that must differ from the spec (docs change first).
- `/qa`: check that the evidence meets the story's acceptance criteria before you mark it `done`.

## Out of scope

Infini and the sync wire ([CHL-0034](../challenges/CHL-0034-forest-wire-sync-deferred.md)), tools
and recognizers, product chrome, using `transform`, deleting `epaper_old/`.
