---
feature: region-sync
parent_req: [REQ-19]
version: 0.2.0
lifecycle: active
needs_design: true
---

# SRS — Region sync Epaper (UI)

Pan/pinch itself has **no extra chrome** ([SRS-EP-22](../ink-box/srs-ui.md#srs-ep-22-hand-touch-ui)). This file is the Designer contract for the **viewport-follow icon toggle** only.

Historical stub (v0.1.0 “no on-device pan chrome”) is withdrawn for **follow**; two-finger still has no dedicated pan-mode tile.

---

## [SRS-EP-50] Viewport-follow Infini toggle {#srs-ep-50-follow-toggle}

<!-- lifecycle: active -->
<!-- needs_design: yes -->

**Parent:** [REQ-19](../../prd.md#viewport-follow). **Logic:** [SRS-EP-49](./srs-logic.md#srs-ep-49-viewport-follow). **Quality:** [SRS-EP-51](./srs-quality.md#srs-ep-51-follow-quality). **Platform:** **epaper-device** (`data-platform: epaper`). **Do not parent on [SRS-EP-05](../tool-modes/srs-ui.md)** (ToolChip). **Do not add this control to the hand-touch package.**

### Purpose

One job: let the creator **opt in** to matching Infini’s drawing region, or see that follow is off. Not a tool, not a recognizer, not a hand-tool tile.

### Composition / containment (contract, not craft)

| Region | Parent | Role |
|---|---|---|
| DeviceScreen | panel | Full panel |
| FollowToggle | DeviceScreen | Icon toggle — **not** inside ToolChip exclusive-tool cluster |
| ToolChip | DeviceScreen | Unchanged three exclusive tools ([SRS-EP-05](../tool-modes/srs-ui.md)) |

**Placement vs ToolChip is a design story.** Binding: finger-eligible hit ≥ primary ToolChip tile (**64×64 du**). Chip hits still win when the finger is on the chip ([REQ-10](../../prd.md#hand-touch)). Follow is **not** a fourth exclusive `toolMode`.

### Closed control inventory

| id | Kind | Notes |
|---|---|---|
| `btn.viewport_follow` | icon toggle | Off / following Infini / peer-following-you (pressing on turns Infini follow off) |

No extra follow-mode canvas overlay required. Designer may reuse existing region marker only if it does not fight the local camera.

### States matrix (journeys from PRD — do not add)

| State id | When |
|---|---|
| `follow.off` | Default; both off; or after explicit off |
| `follow.following_infini` | `direction = infini_to_epaper` |
| `follow.peer_following_you` | `direction = epaper_to_infini` — this toggle is off; enabling it turns Infini follow off |
| `follow.connection_lost` | Session dropped → forced off |
| `follow.reconnect_stays_off` | Link back; still off until opt-in |

### Interaction map

| Control | Action | Effect |
|---|---|---|
| `btn.viewport_follow` (off, session live) | tap | → `follow.following_infini` |
| `btn.viewport_follow` (following) | tap | → `follow.off` |
| `btn.viewport_follow` (peer following you) | tap | → `follow.following_infini` (peer off) |
| No session | tap or look | off or unavailable; 0 follow-on |

### UI-driving fields

`follow.direction`, `session.connected` — Designer must not invent a fourth exclusive tool or a “hand tool” chip tile.

### Anti-patterns

- Fourth exclusive ToolChip tool / recognizer / hand-tool tile
- Hover/focus/cursor
- Restoring follow on reconnect without a tap
- Painting last-writer / token chrome
- Dual-on presentation (both toggles “on”)

### Dual-ask

`/designer` Spec + one scene HTML per state id. `/qa` BDD from [REQ-19](../../prd.md#viewport-follow) AC + [SRS-EP-51](./srs-quality.md#srs-ep-51-follow-quality).
