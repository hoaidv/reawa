---
id: STORY-IN-038
title: Infini applies compound and set ink samples
kind: implement
parent_srs: [SRS-IN-09, SRS-IN-07, SRS-IN-06]
parent_req: [REQ-02, REQ-03]
status: cancelled
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-061]
acceptance_criteria:
  - "Given a tablet undo published as compound of counterpart ops, When Infini applies them in order, Then the mirror equals the device tree (0 divergent nodes) and replay is idempotent by opId."
  - "Given set_ink_samples on an existing ink, When Infini applies it, Then samples match the payload (±1 px @ 100% zoom) and unknown extra fields do not crash."
  - "Given an unknown inverse op, When Infini receives it, Then the mirror is suspect — do not save silently; do not invent restore_snapshot."
  - "Given undo/redo traffic, When applied, Then 0 restore_snapshot is required for the undo path."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-038 — Infini applies compound and set ink samples

**Cancelled 2026-08-27 — do not implement this iter.** Human: skip the whole tablet→desktop sync path now. Infini apply of `compound` / `set_ink_samples` waits for a later phase with an independent sync algorithm. Keep this id.

Was: Infini **applier** (not a desktop undo stack). [STORY-EP-061](./STORY-EP-061.md) no longer depends on this story.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-061 (historical; this story is cancelled) |
