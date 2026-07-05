---
feature: connection-management
parent_req: [REQ-02, REQ-06]
version: 1.0.0
lifecycle: active
---

# SRS — Connection Management (UI)

## [SRS-RW-15] Settings connection editor

Two-pane layout in settings window (`SettingsUI.swift`):

- **Left:** Discovered devices list + saved connections
- **Right:** Active connection form

### New connection flow

- **New** clears form; header shows "New connection"; **Add connection** button visible.
- Fields: Name, IP, Password (required on add only).
- **Scan devices** refreshes Discovered list (already-saved IPs hidden).
- Select discovered device to pre-fill form.

### Edit connection flow

- Select saved connection; header shows "Editing \<name\>".
- Changes apply **immediately** — no Save button.
- Password field not required when editing (key already installed).

## [SRS-RW-16] Device configuration controls

| Control | Default | Behavior |
|---|---|---|
| Output mode | Relative | Segmented: Relative / Absolute / Native Stylus. Absolute on active connection triggers picker |
| Scale | Auto | Screen points per digitizer unit |
| Tablet orientation | Gut on top | Four options with icons: Gut on top, left, bottom, right — maps to axis transforms |
| Border color | `#3B82F6` | Absolute mode region outline |
| Snapped window | — | Read-only context when Absolute + bound |
| Auto-connect | per connection | Toggle in connection profile |

## [SRS-RW-17] Menu bar connection status indicators

| Status | Indicator |
|---|---|
| offline | ○ |
| online | ◎ |
| connected | ● |
| error | ✗ |

---

## Superseded

_None yet._
