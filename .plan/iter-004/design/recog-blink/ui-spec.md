---
id: UI-EP-06
title: Enclose blink + last-join membership highlight
parent_srs: [SRS-EP-12]
parent_req: [REQ-05]
stories: [STORY-EP-029]
status: draft
iter: iter-004
fidelity: hifi
---

# [UI-EP-06] — Enclose blink and membership highlight

Platform: **epaper-device**, 1-bit. Paint-time width only (do not persist `strokeWidth`).

Human 2026-08-15 (revised after device lag): **do not blink** on draw-into. A Mono pulse during inking starved Pen.

## Enclose create — still a one-shot blink

`ovl.enclose_blink`: whole box (boundary + content). Semantic: current width → **2×** → current. One Mono ~250 ms, then idle.

## Draw-into membership — no ToolCanvas highlight

Live Pen stamps **are** the join. Do **not** 2× the boundary on ToolCanvas: damaging the box AABB
on every pen-up stalls the next down (human 2026-09-05). Enclose still blinks once.

| Step | Feedback |
|---|---|
| Stroke joins a box | Content reparents; boundary stays 1×; no overlay pulse |
| Stroke does not join | Ordinary ink / other verdict; clear any leftover stamp |

Reset leftover stamp: exclusive **tool change**, **Undo**, **Redo**, non-join pen-up.

`ovl.membership_highlight` is **retired** (was 2× last-join boundary).

## Anti-patterns

- Timer / second rasterize mid-inking (causes Pen lag)
- Invert-fill (connector)
- Highlighting content ink on membership
- Full-panel flash
- ToolCanvas 2× of the box boundary on every draw-into (stalls pen-up)

Promote via [CHL-0020](../../challenges/CHL-0020-recog-width-blink.md). Device UI state machine is **[STORY-EP-032](../../stories/STORY-EP-032.md)** for `/architect` later.
