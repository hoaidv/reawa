---
title: Business Requirements Document — Reawa
version: 1.0.0
status: approved
owner: analyst
last_review: 2026-08-20
---

# BRD — Reawa

## [BRD-01] Vision

Reawa is a macOS menu bar application that turns a reMarkable 2 tablet into a pen-controlled input device for the Mac. The tablet streams pen events over USB (via SSH) and the app translates them into native mouse movement and clicks — or, in a planned future mode, into a macOS-recognized tablet/stylus device.

The product serves creative and productivity workflows where the reMarkable's paper-like surface is preferable to a trackpad or mouse: sketching in design tools, annotating documents, or navigating applications with pen hover and touch.

## [BRD-02] Strategic Goals

| Goal | Measurable Outcome | Sponsor |
|---|---|---|
| Make reMarkable useful as a Mac input surface | Users can move the cursor and click/drag from the tablet in Relative mode without calibration | Product |
| Enable precision window-targeted pen work | Users can map the tablet screen onto a real application window in Absolute mode | Product |
| Zero-friction device setup | USB-plugged tablets are discovered automatically without hardcoded IPs | Product |
| Lightweight background utility | App runs menu bar–only with no Dock icon during normal pen use | Product |
| Future: native pen recognition in drawing apps | Krita Tablet Tester sees stylus/tablet-class input (not mouse-only) when Native Stylus ships | Product |

## [BRD-03] In-Scope Domains

1. **Pen input translation** — Relative (trackpad-like) and Absolute (screen-mapped to a window) mouse emulation; planned Native Stylus backend.
2. **Device connectivity** — SSH-based pen streaming, per-connection keys, saved profiles, auto-connect.
3. **USB discovery** — Automatic detection of reMarkable on USB-tethered subnets.
4. **Window snapping** — Absolute mode bound to real application windows with lifecycle handling (move, resize, minimize, maximize, close).
5. **Configuration & diagnostics** — Settings window, behavior log, optional pen event log.

## [BRD-04] Out of Scope

- **macOS Markup tool** — Standalone markup for pictures, screenshots, PDFs (future idea from README).
- **Windows / Linux / iOS** — Reawa pen-driver remains macOS-only. **Infini** (infinity canvas) may target desktop via Electron ([BRD-07](#brd-07-infinity-canvas--tablet-sync-infini--epaper)); mobile is still out.
- **reMarkable Paper Pro / reMarkable Connect cloud** — reMarkable 2 over USB Ethernet only.
- **Kernel extensions** — Classic kext-based tablet drivers are explicitly avoided.
- **Wacom driver identity spoofing** — Generic macOS digitizer/stylus device only.
- **Polished signed distributable `.app`** — Blocked until Apple Developer account and Virtual HID entitlement approval (packaging prep exists; shipping is not in scope until approved).

## [BRD-05] Constraints

- **Platform:** macOS 13+ menu bar application; Native Stylus requires macOS 15+.
- **Hardware:** reMarkable 2 connected via USB (network over USB Ethernet; typical IP `10.11.99.1`).
- **Permissions:** Accessibility required for mouse control and window snapping; Notifications optional for auto-connect alerts.
- **Apple entitlements:** Native Stylus requires Apple-approved `com.apple.developer.hid.virtual.device`; `swift run reawa` cannot exercise this path.
- **Single active connection:** Only one device drives the cursor at a time.
- **Independent project:** Not affiliated with reMarkable AS states, Wacom, or Apple.

## [BRD-06] On-device e-paper drawing (Epaper)

Related product surface (sibling module, not the macOS pen driver): a Qt app that
runs **on the reMarkable 2** and draws local pen ink on the e-paper panel with
pen-matched coordinates. Code lives in repo-root `epaper/` (sibling of `macOS/`); product docs in
`.docs/modules/epaper/`.

## [BRD-07] Infinity canvas + tablet sync (Infini ↔ Epaper)

Product line that turns the reMarkable into a **drawing tablet for an infinite desktop
canvas**. Epaper owns local ink. Infini remains navigator of **its own** infinity canvas
and the **persistence home** (desktop is the file). Both share a vector document.
The **document channel stays one-way** this campaign (tablet authors in-session; desktop
is the file)
([EXP-0001](../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md) →
modules [infini](modules/infini/prd.md) + [epaper](modules/epaper/prd.md)).

**Cameras are independent by default.** Always-on viewport sync (Infini drives the tablet
drawing region; last-writer shared camera) is **obsolete**. Infini pans/zooms its own
canvas; it does not exclusively own the tablet’s camera. On-device pan/zoom on Epaper is
**in-scope**.

| Goal | Measurable Outcome | Sponsor |
|---|---|---|
| Independent cameras by default | With a session connected and follow off, a pan/zoom on Epaper leaves Infini’s camera unchanged, and a pan/zoom on Infini leaves Epaper’s camera unchanged | Product |
| Infini own-canvas navigation | Infini pans/zooms its own infinity canvas (transform = translate + uniform scale). It does not exclusively own the tablet camera | Product |
| On-device viewport (Epaper) | Two-finger pan/zoom on Epaper changes the **tablet’s** camera. Local one-finger pan on empty canvas is allowed (independent camera). Neither implies Infini’s camera moves unless Infini is following Epaper | Product |
| Follow on → peer camera matches | When follow is on, the follower’s viewport matches the leader’s (Epaper → Infini, or Infini → Epaper) | Product |
| Exactly one follow direction | While Epaper and Infini are connected, turning one follow toggle on turns the other off. Both off is valid and is the default | Product |
| Follow off on disconnect | When the connection is lost, follow is automatically off; neither camera continues to track the other | Product |
| Interchangeable vectors | One persistence format (SVG profile), one in-memory model, one transmit encoding | Product |

### Business rule (human 2026-08-20)

1. **Tablet may change its own viewport.** Epaper and Infini viewports are **independent by default**.
2. **Optional follow** (exactly one direction at a time when connected):
   - Infini may optionally **follow** the connected Epaper (viewport sync Epaper → Infini).
   - Epaper may optionally **follow** the connected Infini (viewport sync Infini → Epaper).
   - **Exactly one** follow direction is enabled at a time while Epaper and Infini are connected. Turning one on turns the other off.
   - Follow is **automatically off** when the connection is lost.
3. **Chrome (business, not layout):** a **viewport-follow icon toggle** on **both** products. Epaper toggle: follow connected Infini. Infini toggle: follow connected Epaper. This is **not** a ToolChip exclusive “hand tool” tile.
4. Infini remains navigator + persistence home. Document channel stays one-way this campaign.

### Assumptions

- Palm-vs-pan threshold for one-finger empty-canvas pan is Product Manager detail — not a BRD field list.

### Decision (2026-08-20)

**Decided.** Supersedes the prior BRD-07 rule “desktop Infini owns pan/zoom viewing; Epaper owns local ink,” the session rule “Infini viewport drives RM drawing region,” and the deferral “on-device pan/zoom on Epaper.” Last-writer shared camera is **not** the business model.

**Forward (not a Must this campaign):** Infini may follow other Infini instances.

**Out of this BRD (defer):** reversing document direction / CRDT / multi-writer document this campaign. Human: “we change document direction later.”
