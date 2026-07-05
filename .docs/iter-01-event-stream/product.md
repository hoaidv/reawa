# reMarkable → macOS Pen Driver

## Overview

A macOS menu bar application that turns a reMarkable 2 into a pen-controlled input device for your Mac. The tablet streams pen events over USB (via SSH) and the app translates them into native mouse movement and clicks.

The app is designed for creative and productivity workflows where the reMarkable's paper-like surface is preferable to a trackpad or mouse — for example, sketching in design tools, annotating documents, or navigating applications with pen hover and touch.

## Platform

- **macOS** (menu bar application)
- **reMarkable 2** connected via USB (network over USB Ethernet)

## Core Capabilities

| Capability | Description |
|------------|-------------|
| Pen as mouse | Hover moves the cursor; touch performs left-click and drag |
| Multiple devices | Save and manage several reMarkable connections |
| Two input modes | **Relative** (trackpad-like) or **Absolute** (screen-mapped to a window) |
| USB auto-detect | Discovers the tablet when plugged in without hardcoding its IP |
| Auto-connect | Optionally connect automatically when the device appears |
| Planned native pen device mode (Swift) | Future output backend that exposes the reMarkable as a macOS-recognized tablet/stylus device instead of only mouse events |

## Menu Bar Experience

The app lives in the menu bar as a pen-on-tablet icon. When no settings window is open, it runs as a menu bar–only application with no Dock icon. Opening **Open** shows the settings window and a Dock icon; closing that window hides the Dock icon again.

### Menu Structure

| Item | Behavior |
|------|----------|
| **Connections** | One entry per saved device, with a status indicator (see [Connection Status](#connection-status)) |
| Click a connection | Connect or disconnect |
| **Relative** / **Absolute** | Active mode shows a green dot and is greyed out; inactive mode is clickable to switch (only while connected). Switching to **Absolute** starts the window-snapping flow |
| **Sending to …** | Shows the snapped window name (e.g. *Sending to Figma*); greyed out; visible only in Absolute mode after a window is chosen |
| **Choose window** | Re-run the window-snapping flow (Absolute mode only) |
| **Open** | Manage connections and device settings |
| **Quit** | Exit the application |

Mode switching and window re-selection are available from the menu bar only — there is no on-screen toolbar during normal use.

## Connections

A **connection** is a saved profile for reaching one reMarkable device.

| Field | Description |
|-------|-------------|
| **Name** | Display name (e.g. "My RM2") |
| **IP** | Device address (USB default: `10.11.99.1`) |
| **SSH key pair** | Per-connection key, generated and stored locally on first setup |
| **Password** | Stored in macOS Keychain; used only for the initial key installation |
| **Auto-connect** | Connect automatically when the device is detected on USB |
| **Device config** | Output mode and pen mapping settings |

### Adding and Editing Connections

The settings window (**Open**) provides a single form for creating and editing connections:

- **New** clears the form and shows "New connection". The form shows an **Add connection** button.
- Enter Name, IP, and Password, then click **Add connection**. Use **Scan devices** to refresh the **Discovered** list (IPs already saved are hidden). Select a discovered device to pre-fill the form.
- Select an existing connection to edit it. The header shows "Editing \<name\>". Changes apply immediately as you edit; there is no **Save changes** button. Password is required only when adding.

After the first successful setup, subsequent connections use the SSH key only — no password is needed.

Only **one connection** can be active at a time. Connecting to a second device disconnects the first.

## Device Configuration

Per-connection pen behavior:

| Setting | Default | Description |
|---------|---------|-------------|
| **Output mode** | Relative | Relative or Absolute (segmented toggle in settings) |
| **Scale** | Auto | Screen points per digitizer unit; auto uses display PPI |
| **Tablet orientation** | Gut on top | Four options: **Gut on top**, **Gut to the left**, **Gut at bottom**, **Gut to the right**. Each option is shown with an icon and maps to the underlying axis transform so pen motion feels natural for the tablet's physical rotation |
| **Border color** | `#3B82F6` | Color of the region outline in Absolute mode |

All settings in the edit form apply immediately for existing connections, including output mode and tablet orientation.

### Diagnostics

The settings window also includes two diagnostics tabs:

| Tab | Behavior |
|-----|----------|
| **App Behavior Log** | Always on. Searchable log of settings changes, mode changes, connection/session/SSH events, device detection, and Absolute-mode window-picking behavior |
| **Pen Event Log** | Off by default. A **Capture pen events** toggle enables a searchable stream of raw Linux pen events, accumulated pen semantics, and recognized gesture states |

In **Pen Event Log**, observed capability labels such as `BTN_STYLUS`, `ABS_TILT_X`, or `ABS_DISTANCE` can be clicked to fill the search box and immediately filter the log.

## Output Modes

### Relative Mode

Pen movement is **relative** to the current cursor position. Each event applies a delta, like a trackpad. No screen-to-tablet calibration is required; the mode works across multiple displays.

| Pen state | Effect |
|-----------|--------|
| Hovering | Move cursor |
| Touching | Left-click drag |

### Absolute Mode

Absolute mode maps the reMarkable screen directly onto a **region** on the Mac desktop. The region is always bound to a real application window and maintains the reMarkable's aspect ratio.

**Product principle:** There is no free-floating absolute region. Entering Absolute mode always requires choosing a target window. Cancelling the picker reverts to Relative mode.

#### Entering Absolute Mode

Choosing **Absolute** — in settings, via the menu bar, or on auto-connect when a saved connection's config is Absolute — starts the window picker when that connection is active:

1. Pen input to macOS is **paused** while picking.
2. A full-desktop overlay appears. **Hover** across any display and the window under the cursor is **highlighted**.
3. **Click** a window to select it — the region snaps to that window and pen input resumes in Absolute mode.
4. Press **Esc** to cancel — no window is chosen and the mode **reverts to Relative** (the settings window, if open, updates its toggle to match).

#### Once Snapped

| Behavior | Description |
|----------|-------------|
| Pen mapping | The reMarkable pen maps into the snapped window and is **clamped to it** — pen input cannot leave the window |
| Other input devices | Trackpad and mouse remain **free** — you can reach the menu bar, other windows, or the region's resize handles |
| Region outline | A colored border marks the region; the rest of the desktop stays fully visible and usable |
| Resize handles | Four corner handles; dragging resizes the region (aspect-locked) and resizes the snapped window to match |
| Window moved | The region **follows** the window |
| Window minimized | The region overlay hides; pen mapping stays in Absolute until you restore or close the window |
| Window maximized | The region expands to the largest reMarkable-aspect rectangle that fits the screen area, then the window is resized to match |
| Window closed | The app switches back to **Relative** mode |
| Window restored | The overlay reappears and stays aligned with the window |
| Re-snap | Use **Choose window** in the menu bar, or switch to Absolute again from the menu |
| Settings | When Absolute is selected, **Snapped window** shows which window is currently bound |

The overlay spans **all connected displays**, so the snapped window can live on any monitor.

## Planned Native Pen Device Mode

This is a **planned Swift feature**, not the current shipping behavior.

Repository note:

- The repo now includes an initial Native Stylus backend spike and local app-bundle/signing preparation.
- Even with that code in place, the feature cannot create a real macOS virtual HID pen device in the normal supported path until Apple approves the restricted entitlement `com.apple.developer.hid.virtual.device` for the signing team.
- Approving the later macOS Accessibility prompt is a separate local step and is **not** a substitute for Apple entitlement approval.

Today the app translates reMarkable input into Quartz mouse events. The next product direction is an additional output backend that publishes the reMarkable as a **macOS pen / tablet device**, so supported apps can recognize it as pen input instead of only as a mouse.

Target behavior:

| Goal | Description |
|------|-------------|
| Native pen recognition | Apps should receive tablet/stylus input from macOS rather than synthesized mouse-only input |
| Pressure-aware drawing | Pressure should reach drawing apps directly when the hardware reports it |
| Pen metadata | Proximity, tip contact, barrel button, and tilt should be forwarded when available |
| Optional backend | Mouse emulation remains available; pen-device mode is an additional output path, not a replacement |
| App-first validation | The first validation target is **Krita** (Tablet Tester), followed by end-to-end checks in apps such as **Photoshop** |

Product constraints for this feature:

1. The app should expose a **generic macOS digitizer / stylus device**, not pretend to be a specific Wacom-branded device.
2. The existing SSH pen stream and connection model should remain the source of truth for reMarkable input.
3. Mouse-emulation workflows must keep working even if pen-device mode is unavailable on a given machine.
4. Shipping this mode may require additional Apple-controlled entitlements and a separate install/approval flow from the current mouse-emulation build.

Local-development constraint:

- `swift run reawa` remains valid for mouse-emulation development, but it cannot test Native Stylus because it does not launch a signed app bundle with the restricted Virtual HID entitlement.

## USB Auto-Detect

When the reMarkable is plugged in via USB, the app scans local network interfaces (especially USB Ethernet) for subnets and probes hosts for SSH. This finds the device (typically at `10.11.99.1`) without hardcoding the IP.

| Situation | Behavior |
|-----------|----------|
| Saved connection, auto-connect **on** | Connect automatically and show a notification |
| Saved connection, auto-connect **off** | Notification prompting manual connection |
| No saved connection, device found | Notification with the discovered IP address(es) |
| Device disconnects while active | Session stops and a notification is shown |

## Connection Status

Each saved connection is shown in one of four states:

| Status | Indicator | Meaning |
|--------|-----------|---------|
| **offline** | ○ | No device reachable at the connection IP (e.g. tablet unplugged) |
| **online** | ◎ | Device IP is reachable, pen stream not active |
| **connected** | ● | SSH pen stream open; input is driving the Mac cursor |
| **error** | ✗ | A connect attempt failed (e.g. SSH authentication); device may still be online |

Status updates automatically (~every 3 seconds). Offline becomes online when the IP becomes reachable; online becomes connected when you connect (or auto-connect runs). Errors persist until you retry or the device goes offline.

## Permissions

| Permission | Required | Purpose |
|------------|----------|---------|
| **Accessibility** | Yes | Mouse control and window snapping (System Settings → Privacy & Security → Accessibility) |
| **Notifications** | No | Auto-connect and device-detect alerts |

## Product Decisions

These choices define what the product does and how it behaves. Implementation details are documented in [technical.md](technical.md).

1. **Absolute mode is always window-bound.** Pen input in Absolute mode is clamped to a real application window. There is no standalone absolute region on the desktop.

2. **Window picking is mandatory for Absolute mode.** Entering Absolute always runs the picker. Cancelling (Esc or switching to Relative) reverts to Relative mode.

3. **Pen input pauses during picking; other devices do not.** While choosing a window, the reMarkable pen does not affect the Mac. Trackpad and mouse remain usable throughout.

4. **Single active connection.** Only one device can drive the cursor at a time.

5. **Menu bar–first UI.** The app is a background utility. Mode switching, window re-selection, and connection toggling are available from the menu bar. The settings window is for configuration only.

6. **Dock icon only while settings are open.** The app stays out of the Dock during normal pen use.

7. **Per-connection SSH keys and settings.** Each device has its own key pair, password (for initial setup), and pen configuration.

8. **USB discovery without hardcoded IPs.** The app discovers the tablet on USB-tethered subnets rather than assuming a fixed address.

9. **Closing the snapped window exits Absolute mode.** If the target application window is closed, the app returns to Relative mode rather than leaving pen input in an invalid state.

10. **Minimize hides the overlay; close exits Absolute.** A minimized window hides the region outline but keeps Absolute mode active. Closing the window reverts to Relative.

11. **Future pen-device mode is additive, not disruptive.** The Swift app should support both mouse emulation and native pen-device output; users should not lose the existing Quartz-based workflow when the tablet-device path is unavailable.

## Related Documentation

- [technical.md](technical.md) — architecture, data model, implementation, and bug-fix history
