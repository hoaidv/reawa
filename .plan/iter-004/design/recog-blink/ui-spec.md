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

## Draw-into membership — persistent highlight, no blink

| Step | Feedback |
|---|---|
| First stroke joins box A | **Highlight A's boundary-ink** (2× width). No pulse. |
| Further strokes also join A | A's boundary **stays** highlighted |
| Stroke does **not** join A | **Clear** A's highlight |
| Latest join is box B | Highlight **only B** (last draw-into event) |

Reset highlight to none: exclusive **tool change**, **Undo**, **Redo**.

`ovl.membership_highlight` = boundary inks of the SmartGroup of the **latest** `membership` verdict.

Rejected / ordinary ink / enclose: not a join → clear previous membership highlight.

## Anti-patterns

- Timer / second rasterize mid-inking (causes Pen lag)
- Invert-fill (connector)
- Highlighting content ink on membership
- Full-panel flash

Promote via [CHL-0020](../../challenges/CHL-0020-recog-width-blink.md). Device UI state machine is **[STORY-EP-032](../../stories/STORY-EP-032.md)** for `/architect` later.
