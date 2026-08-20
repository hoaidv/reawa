---
from: designer
to: sm
date: 2026-08-20
iter: iter-005
cc: [pm, qa, dev]
---

# Hand-off: Designer → SM — STORY-EP-053 done `[UI-EP-07]`

## Verdict

**PASS** (ui-spec-gate checklist). Package `.plan/iter-005/design/viewport-follow-epaper/` — [UI-EP-07](../design/viewport-follow-epaper/ui-spec.md).

Lane A [STORY-EP-053](../stories/STORY-EP-053.md) is **done**. Copied `ui_spec` / `scenes` / `hifi` onto [STORY-EP-055](../stories/STORY-EP-055.md) only.

## Package

- Spec: [ui-spec.md](../design/viewport-follow-epaper/ui-spec.md)
- Navigator (iframe, 80%): [index.html](../design/viewport-follow-epaper/index.html)
- Primary / hifi: [viewport-follow-epaper-off.html](../design/viewport-follow-epaper/viewport-follow-epaper-off.html)
- States: [viewport-follow-epaper-states.html](../design/viewport-follow-epaper/viewport-follow-epaper-states.html)
- Unique system icon: `design/system/assets/icon-epaper-viewport-follow.svg`

Follow is a **10 mm icon toggle** at orientation-top trailing. **Not** a ToolChip exclusive, recognizer, or hand-tool tile. ToolChip remains UI-EP-04 (3 exclusive). Peer-following-you is visually **off** (exactly one direction). Reconnect does not restore. No last-writer token chrome.

## Scene list

| State | File |
|---|---|
| `follow.off` | `viewport-follow-epaper-off.html` |
| `follow.following_infini` | `viewport-follow-epaper-following-infini.html` |
| `follow.peer_following_you` | `viewport-follow-epaper-peer-following-you.html` |
| `follow.connection_lost` | `viewport-follow-epaper-connection-lost.html` |
| `follow.reconnect_stays_off` | `viewport-follow-epaper-reconnect-stays-off.html` |
| (showcase) | `viewport-follow-epaper-states.html` |

## Gate

- ui-spec-gate: **PASS** for this package (SRS-EP-50 closed list; 1-bit epaper; press invert; tokens.css + common.css; iframe index 80%; icon on disk).
- `data-platform="epaper"` vs engine allowlist: known [CHL-0002](../../iter-003/challenges/CHL-0002-epaper-platform-gate.md) residual — Spec/SRS authoritative.
- Design index / DESIGN.md: **not** updated (lock: SM stitches).

## Surprises

- `region-sync` has no `srs-experience.md`. Painted SRS-EP-50 states only (campaign override, same as UI-EP-06). Did not invent popups or extra states.
- Peer-following-you **looks like off** on purpose (anti dual-on). Navigator + `aria-description` distinguish the journey. AC “off/disabled” mapped as cannot-stay-on, still tappable to take over (SRS tap row).
- Placement: trailing sibling of ToolChip, not a fourth cluster in the exclusive row.

## Next

`/sm` stitch design index when joining with IN-036. `/dev` EP-055 against this package. Do not open EP-054 until WIP allows.
