# Components — viewport-follow-infini

Closed inventory for [SRS-IN-27](../../../../.docs/modules/infini/features/tablet-sync/srs-ui.md#srs-in-27-follow-toggle). Canvas chrome is **context** (UI-IN-01), not this REQ.

| Component | Kind | File | Variants / states |
|---|---|---|---|
| ViewportFollowToggle | system | `../system/components/viewport-follow-toggle-infini.html` | default, hover, focus-visible, active, `aria-pressed`, peer (off), disabled |
| FollowChrome | screen | `./components/follow-chrome-infini.html` | cluster: toggle + caption + StatusZoom |
| CanvasStage | screen | `./components/canvas-stage.html` | hover grab, focus-visible, active grabbing (reuse UI-IN-01) |
| ZoomReadout | screen | `./components/zoom-readout.html` | display only |
| WorldLayer | screen | `./components/world-layer.html` | local crop / applied tablet crop |

## Pattern ids

| Component | CSS class | Notes |
|---|---|---|
| ViewportFollowToggle | `.c-follow-toggle` | `btn.viewport_follow`. Not `.c-follow-btn` (Epaper 1-bit sibling). |
| FollowChrome | `.c-follow-cluster` + `.c-chrome-trailing` | WindowFrame child; leading StatusZoom |
| Caption | `.c-follow-caption` | Status not by color alone |
| CanvasStage | `.c-canvas-stage` | Full-bleed; hop to local-nav while following |
| ZoomReadout | `.c-zoom-readout` | Unchanged |
| WorldLayer | `.c-world-layer` | Transform host |

## Unique system files this story (do not collide with EP-053)

| File | Role |
|---|---|
| `../system/assets/icon-viewport-follow-infini.svg` | Off / default glyph |
| `../system/assets/icon-viewport-follow-infini-on.svg` | Following Epaper |
| `../system/assets/icon-viewport-follow-infini-peer.svg` | Peer following you (this toggle off) |
| `../system/assets/icon-viewport-follow-infini-offline.svg` | No session |
| `../system/components/viewport-follow-toggle-infini.html` | Desktop hover toggle |

Do **not** reuse `icon-epaper-viewport-follow.svg` or `viewport-follow-epaper/components/follow-toggle.html`.
