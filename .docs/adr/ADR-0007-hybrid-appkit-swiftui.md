---
id: ADR-0007
title: Hybrid AppKit and SwiftUI UI split
status: accepted
date: 2026-07-05
deciders: [architect]
supersedes: null
---

# ADR-0007 — Hybrid AppKit and SwiftUI UI split

## Context

Reawa needs low-latency overlay windows, global cursor polling, AX integration, and per-display event routing — APIs that sit closest to AppKit. Settings and logs are form-heavy and benefit from SwiftUI declarative UI.

## Decision

- **AppKit:** Menu bar shell (`NSStatusItem`), picker overlay, region overlay + resize handles, `NSWindowController` for settings host, Accessibility/window lifecycle.
- **SwiftUI:** Settings tabs (Connections, App Behavior Log, Pen Event Log), connection editor, immediate-apply forms.

Settings window: `window.isReleasedWhenClosed = false` so reopening reuses controller safely (bug fix #7).

## Consequences

- `@MainActor` for AppKit/SwiftUI boundary; pen logs debounced to main-actor snapshots.
- Standard Edit menu / responder chain required for Cmd+V in text fields (bug fix #8).
- Two UI stacks to maintain but each used where it fits best.

## Alternatives Considered

| Alternative | Why rejected |
|---|---|
| Pure AppKit | Higher maintenance for settings/forms |
| Pure SwiftUI app lifecycle | Overlay/window level control harder |
| PyObjC (legacy) | Replaced by native Swift port |
