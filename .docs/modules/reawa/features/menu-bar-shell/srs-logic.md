---
feature: menu-bar-shell
parent_req: [REQ-01, REQ-09]
version: 1.0.0
lifecycle: active
---

# SRS — Menu Bar Shell (Logic)

## [SRS-RW-01] Application activation policy

The app uses `NSApplicationActivationPolicy.accessory` by default (menu bar only, no Dock icon). When the settings window opens, policy switches to `NSApplicationActivationPolicy.regular` (Dock icon visible). Closing the settings window restores accessory policy.

Implementation: `AppController.setDockVisible(_:)` in `Sources/ReawaApp/AppController.swift`.

## [SRS-RW-02] Menu bar icon and menu rebuild

- Status item uses SF Symbol `pencil.tip.crop.circle` when available; fallback to bundled `menu_icon.png`.
- Menu is rebuilt on each connection status change via `refreshMenu()`.
- Menu actions route through `AppController` selectors: connect/disconnect, output mode, window picking, open settings, about, quit.

## [SRS-RW-03] Accessibility permission gate

Mouse control (`CGEventPost`) and window snapping (AX APIs) require Accessibility permission. The app does not bypass System Settings; user must grant in Privacy & Security → Accessibility.

## [SRS-RW-04] Notification service (bundled runs only)

`NotificationService` uses `UNUserNotificationCenter` only when `Bundle.main.bundleURL` path extension is `.app`. Non-bundled launches (`swift run`) suppress notifications and log suppression instead of crashing (regression: UNUserNotificationCenter requires bundle proxy).

---

## Superseded

_None yet._
