---
feature: menu-bar-shell
parent_req: [REQ-01]
version: 1.0.0
lifecycle: active
---

# SRS — Menu Bar Shell (UI)

## [SRS-RW-05] Menu structure

| Item | Behavior |
|---|---|
| **Connections** | One entry per saved device with status indicator (see connection-management) |
| Click connection | Connect or disconnect |
| **Relative** / **Absolute** | Active mode: green dot, greyed out. Inactive: clickable. Only while connected. Absolute switch starts window picker |
| **Sending to …** | Snapped window name; greyed out; visible only in Absolute after window chosen |
| **Choose window** | Re-run window picker (Absolute only) |
| **Open** | Open settings window |
| **Quit** | Exit application |

Mode switching and window re-selection are menu bar only — no on-screen toolbar during normal use.

## [SRS-RW-06] Dock visibility states

| State | Dock icon | Trigger |
|---|---|---|
| Normal pen use | Hidden | Settings closed |
| Configuration | Visible | Settings window open |

---

## Superseded

_None yet._
