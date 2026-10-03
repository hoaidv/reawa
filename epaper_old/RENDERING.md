# Epaper rendering & pen-to-ink latency

Deep dive into how local ink works on reMarkable 2, why `/dev/fb0` is a dead
end, and how this tree reaches toward xochitl-class latency (< 27 ms by eye).

Related:

- Spec: [SRS-EP-01](../.docs/modules/epaper/features/local-pen-ink/srs-logic.md)
- Probe history: [EXP-0001](../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md)
- Sources: [`epaperbridge.cpp`](epaperbridge.cpp), [`tabletcanvasitem.cpp`](drawing/tabletcanvasitem.cpp), [`Main.qml`](drawing/Main.qml)

---

## 1. Why `/dev/fb0` is not the panel on RM2

### reMarkable 1 vs 2

| | RM1 (i.MX6SL) | RM2 (i.MX7D) |
|---|---|---|
| Display HW | On-chip **EPDC** | No EPDC |
| Kernel path | `mxc_epdc_fb` → `/dev/fb0` | Stub / unrelated FB node |
| Refresh API | `MXCFB_SEND_UPDATE` ioctl | Userspace **SWTCON** |
| Owner | Kernel driver | Process that loads `libqsgepaper.so` |

On RM1 a classic Linux framebuffer works:

```c
fd = open("/dev/fb0", O_RDWR);
ioctl(fd, FBIOGET_VSCREENINFO, &var);   // 1404x1872 RGB565
mem = mmap(...);
ioctl(fd, MXCFB_SEND_UPDATE, &upd);      // rect + waveform
```

On RM2 the EPDC is gone. reMarkable runs a **software display controller
(SWTCON)** inside `libqsgepaper.so` (scenegraph plugin) / `libepaper.so` (QPA).
xochitl (and any app using `QT_QPA_PLATFORM=epaper`) links that stack and becomes
the sole panel owner.

### What our Round-9 probe actually saw

```
/dev/fb0: 260x1408, bpp=32, line_length=1040, smem_len=32MiB
MXCFB_SEND_UPDATE (0x4048462e) → EINVAL
```

`1040 = 260 × 4` is self-consistent, but it is **not** the physical
`1404×1872` panel. The ioctl is rejected because nothing behind that node
implements the old EPDC update path. Writing pixels there does nothing visible.

### Single ownership

`EPFramebuffer::checkLockFile` + `QLockFile::tryLock` enforce one writer.
That is why [`scripts/deploy-rm2.sh`](scripts/deploy-rm2.sh) runs
`systemctl stop xochitl` before launch — we are taking the panel, not just
hiding a UI.

`rm2fb` (Round 11) failed because it injects into SWTCON by **hardcoded
per-build addresses** keyed on `libqsgepaper.so` build ID. Our firmware
(`3.28.0.157` / Codex `5.8.197`) is unsupported → `Failed to get addresses`.

---

## 2. What we use instead: Qt epaper + Pen mode

### Boot env ([`main.cpp`](main.cpp))

```cpp
qputenv("QMLSCENE_DEVICE", "epaper");
qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");
qputenv("QT_QPA_GENERIC_PLUGINS", "evdevtablet");
qputenv("QT_QUICK_BACKEND", "epaper");
```

This loads:

- `/usr/lib/plugins/platforms/libepaper.so` — QPA (input, windowing)
- `/usr/lib/plugins/scenegraph/libqsgepaper.so` — SWTCON + waveforms

### Confirmed symbols (firmware 3.28.0.157)

Demangled from `libqsgepaper.so`:

```
EPFramebuffer::instance()
EPFramebuffer::swapBuffers(QRect, EPContentType, EPScreenMode, QFlags<UpdateFlag>)
EPScreenModeItem::EPScreenModeItem(QQuickItem*)
EPScreenModeItem::staticMetaObject   // Mode enum + mode property
EPRenderBlocker::{start,stop,setDeadline}
EPFramebufferSwtcon::update(QRect, int, PixelMode, int)
```

`EPScreenModeItem::Mode` (from moc string table):

| Value name | Role |
|---|---|
| **Pen** | Low-latency ink (what xochitl uses for pen strokes) |
| Mono | Fast 1-bit |
| Animation | DU-style animation |
| UI | UI chrome |
| Content | Document content |
| Sleep | Sleep screen |

**There is no exported pixel-buffer getter.** Only
`setBuffers(tuple<QImage,QImage>, QImage*)`. We cannot legally paint straight
into the panel buffer; Qt Quick remains the rasterizer. Latency work is about
**damage size** + **waveform mode**, not bypassing SWTCON.

---

## 3. Runtime bridge ([`epaperbridge.cpp`](epaperbridge.cpp))

Unlike `rm2fb`, we never hardcode addresses. Qt loads the scenegraph plugin with
`RTLD_LOCAL`, so `dlsym(RTLD_DEFAULT, …)` alone fails — we
`dlopen("/usr/lib/plugins/scenegraph/libqsgepaper.so", RTLD_NOW|RTLD_GLOBAL|RTLD_NOLOAD)`
then `dlsym` from that handle:

```cpp
m_lib = dlopen("/usr/lib/plugins/scenegraph/libqsgepaper.so",
               RTLD_NOW | RTLD_GLOBAL | RTLD_NOLOAD);
m_instance = dlsym(m_lib, "_ZN13EPFramebuffer8instanceEv");
// Try newer 4-arg (SDK), then older 3-arg (device 3.28):
m_swapBuffers4 = dlsym(m_lib,
    "_ZN13EPFramebuffer11swapBuffersE5QRect13EPContentType12EPScreenMode"
    "6QFlagsINS_10UpdateFlagEE");
m_swapBuffers3 = dlsym(m_lib,
    "_ZN13EPFramebuffer11swapBuffersE5QRect12EPScreenMode"
    "6QFlagsINS_10UpdateFlagEE");
m_screenModeCtor = dlsym(m_lib, "_ZN16EPScreenModeItemC1EP10QQuickItem");
m_screenModeMeta = dlsym(m_lib, "_ZN16EPScreenModeItem16staticMetaObjectE");
```

On firmware `3.28.0.157` the **3-arg** form resolves (`swap=3arg` in status).

`Pen` is resolved at runtime:

```cpp
const int idx = m_screenModeMeta->indexOfEnumerator("Mode");
m_penMode = m_screenModeMeta->enumerator(idx).keyToValue("Pen");
```

`EPScreenModeItem` size is unknown, so we **placement-construct** into a
4 KiB zeroed block and drive it only through `QObject` / `QQuickItem` APIs
(`setProperty("mode", penValue)`, `setParentItem`, geometry). Missing symbols
→ `available()==false` and the app falls back to stock Qt epaper refreshes.

QML sees the singleton as `EpaperBridge` (`available`, `status`,
`attachPenModeRegion`, `swapPen`).

---

## 4. Pen → ink pipeline (current)

```
Wacom /dev/input/event1
  → Qt evdevtablet plugin
  → TabletAppFilter (GUI thread)          [tabletappfilter.cpp]
  → TabletCanvasItem::ingestPoint         [tabletcanvasitem.cpp]
  → mapInputToCanvas (landscape transform)
  → paint segment into persistent QImage
  → time-based flush (~8 ms)
  → update(dirtyRect)  // SG partial damage
  → epaper render loop rasterizes the painter node
  → SWTCON (Pen waveform, because the region is tagged) → panel
```

`EPFramebuffer::swapBuffers` is **not** on the default path. Tagging the region
with `EPScreenModeItem(Mode::Pen)` is sufficient; set `RM_EP_SWAP=1` to add the
explicit call for experiments.

### Coordinate map (verified Round 19)

Panel is portrait; device is used landscape:

```cpp
const qreal rx = raw.y() * (w / h);
const qreal ry = h - raw.x() * (h / w);
```

([`tabletcanvasitem.cpp`](drawing/tabletcanvasitem.cpp) `mapInputToCanvas`)

### The invisible-ink bug: a zero-sized item, not a backend limit (2026-08-09)

The first `QQuickPaintedItem` ink layer showed **nothing**, while the QML
`Rectangle` pool worked. The tempting conclusion — "this backend can't render
painted items" — was wrong. `libqsgepaper.so` exports
`EPContext::createPainterNode(QQuickPaintedItem*)`, and disassembly shows it
returns a private class that embeds an `EPNode` at offset `0x50`, exactly like
the rectangle and image nodes. Painter nodes are first-class here.

The actual cause: **the item was `0x0` forever**, so `updatePaintNode()` returned
a null node and Qt never called `paint()`.

```
[ink] componentComplete size QSizeF(0, 0) hasContents true
      parentItem QQuickRootItem(geometry=0,0 0x0) parentSize QSizeF(0, 0)
[ink] updatePaintNode size QSizeF(0, 0) textureSize QSize(-1, -1)
      old Node(null) new Node(null)
```

`Main.qml` used `anchors.fill: parent`, but under this QPA the `QQuickRootItem`
of our `QQuickWindow` subclass is never resized — it stays `0x0` even though the
window is `1404x1872`. Everything that *did* render had its own size: `Text` sizes
itself implicitly, and the pool `Rectangle`s carried explicit geometry. Only the
anchored item collapsed.

Fix — bind to the window instead of anchoring to the content root:

```qml
TabletCanvas {
    x: 0; y: 0
    width: root.width
    height: root.height
}
```

The same bug silently broke the latency path: `attachPenModeRegion` was tagging a
`0x0` rectangle, so the **Pen waveform never covered the canvas**. Sizing the item
correctly fixed the ink and the latency in one change.

### Diagnosing it: beacons, not eyes

There was no framebuffer readback on RM2, so "is it on screen?" needed a human.
Two squares stamped into the ink `QImage` turned that into a truth table (static
square at image creation vs flush square toggled each flush). Plus `flush=` /
`paint=` counters in the status line. `flush=` climbing while `paint=` stayed at
`0` isolated the failure to Qt — above the epaper backend — in a single deploy.

Those probes (`RM_INK_BEACON`) are **gone**: live ink reaches the panel, so they
are no longer compiled in.

### Why the 2000-item `Rectangle` pool is only a fallback

Every segment was a live `QQuickItem`. The renderer walked 2000+ nodes per
frame, old ink wrapped away as the pool recycled (the "snake"), and debug
overlays forced a **second damage region** far from the stroke, ballooning the
union rect toward full-screen `GC16` refreshes.

Now: **one** `QQuickPaintedItem`, persistent `QImage`, `update(bbox)` only.
`RM_INK_MODE=pool` still selects the old path for comparison.

### Why the 3 px distance gate was removed

At a careful ~50 px/s, 3 px ≈ **60 ms** before first ink — already over the
27 ms budget before the panel is asked. Replaced with a **time-based flush**
(`kFlushIntervalMs = 8`).

### Stroke sync

[`strokesync.cpp`](regionsync/strokesync.cpp) is **inert unless `RM_SYNC_HOST` is set**.
When enabled, lines are queued and flushed on a deferred `QTimer::singleShot(0)`
so TCP never blocks the pen hot path.

### EPRenderBlocker note

`beginStrokeBlock` / `endStrokeBlock` exist on the bridge but are **not**
armed during ink. Blocking the render loop would suppress the SG paint that
must land in the FB *before* `swapBuffers`. Pen-mode tagging + tight damage
is the primary lever.

---

## 5. Before / after (measured 2026-08-09)

| Lever          | Before                                    | After                                         |
| -------------- | ----------------------------------------- | --------------------------------------------- |
| Ink nodes      | 2000 `Rectangle`s                         | One `QQuickPaintedItem` + persistent `QImage` |
| Canvas size    | `0x0` (`anchors.fill` on an unsized root) | `width/height` bound to the window            |
| Pen region     | Tagged over a `0x0` rect (no effect)      | Covers the full `1404x1872` canvas            |
| Flush gate     | ≥3 px travel                              | Time-based, `kFlushIntervalMs = 8`            |
| Waveform       | Backend default (often GC16)              | `EPScreenModeItem(Mode::Pen)`                 |
| Debug overlays | Text + blinker every sample               | Status text refreshed between strokes         |
| Stroke TCP     | Every sample on GUI thread                | Off unless `RM_SYNC_HOST`                     |

Result: **at parity with xochitl by eye**, meeting the < 27 ms goal.

`RM_INK_TRACE=1`, 764 samples over a drawing session:

```
[ink-trace] arrival->flush n=764 p50=305us p95=798us p99=1517us
[ink-trace] flush->swap: (no samples)     # RM_EP_SWAP unset
```

Our own code costs ~0.3 ms of the budget, so the remaining latency is input
plumbing, scene-graph render, and SWTCON waveform time. Further gains must come
from those, not from the ink layer.

### Resident-document probe (STORY-EP-013 / SRS-EP-13)

Does **not** ship a document tree. Behind `RM_DOC_PROBE=1` the ingest path holds a
500-node / 50k-sample stub and hit-tests on pen-down. `paint()` is unchanged.
Host numbers and the device protocol live in
[STORY-EP-013](../.plan/iter-003/stories/STORY-EP-013.md).

```bash
# Device-runnable (synthetic strokes, then exit):
RM_INK_TRACE=1 RM_DOC_PROBE=1 RM_DOC_PROBE_SYNTH=1 ./scripts/deploy-rm2.sh --build
ssh root@10.11.99.1 'sleep 2; grep -E "ink-trace|doc-probe" /tmp/epaper.log'

# Or draw by hand, then dump:
RM_INK_TRACE=1 RM_DOC_PROBE=1 ./scripts/deploy-rm2.sh --build
ssh root@10.11.99.1 'killall -TERM epaper; sleep 1; grep -E "ink-trace|doc-probe" /tmp/epaper.log'
```

---

## 6. How to run / measure

`deploy-rm2.sh` forwards any `RM_*` app flag that is set locally:

```bash
cd epaper
RM_INK_TRACE=1 ./scripts/deploy-rm2.sh --build
# draw for a while, then ask the app to exit so it can dump stats:
ssh root@10.11.99.1 'killall -TERM epaper; sleep 1; grep ink-trace /tmp/epaper.log'
```

`SIGTERM` is handled through the event loop ([`main.cpp`](main.cpp)), so
`killall` produces the dump instead of losing it. On device log
(`/tmp/epaper.log`) look for:

```
[epaperbridge] libqsgepaper Pen OK (mode=0 content=0 swap=3arg)
[ink] updatePaintNode size QSizeF(1404, 1872) ... new GeometryNode(…)
[ink-trace] arrival->flush n=… p50=…us p95=…us
```

A `new Node(null)` on that `updatePaintNode` line means the canvas is unsized
again — ink will be invisible.

Ink-path attribution is **on by default** (pen-down → pixel budget, 30 ms).
Each hitch writes one line to `/tmp/epaper-ink-path.log` (also stderr):

```
[ink-path] event=down stroke=12 i=0 total_ms=2 gap_ms=180 reason=queued
  slowest=- slowest_ms=0 behind=rasterizeVectors behind_ms=108
  recent=rasterize.warp:80,rasterize.render:22,rasterizeVectors:108
  ink=412 nodes=480 spans=-
```

`reason=queued` + `i=0 event=down` is the "first millimetre lags, then smooth"
signature: GUI thread was in `behind=` immediately before the sample.
`reason=slow_sample` means the InkStroke callback itself was over budget;
`slowest=` is the leaf stage. `EPAPER_INK_PATH=0` disables. Tail on device:

```bash
ssh root@10.11.99.1 'tail -f /tmp/epaper-ink-path.log'
```

Camera / document rasterize is **on by default** (every `rasterizeVectors`). One line to
`/tmp/epaper-raster.log` (stderr and `[raster]` in `/tmp/epaper.log` too):

```
[raster] reason=camera cam=pan blit=1 inplace=0 sharp=0 total_ms=40 warp_ms=0
  render_ms=12 update_ms=8 ink=812 nodes=940 samples=44102 visits=0 skip=0
  polylines=0 pts=0 pan_px=18.2 zoom=1.000
```

`blit=1` is pan shift / zoom scale of the previous panel (or a slightly stale
job warped toward the current camera). `blit=0 sharp=1` with large `render_ms`
is the LatestJob vector swapped in — that is when newly revealed strips and
zoom-in AA appear. `cam=none` on that line means the job matched the current
camera (success, not a skip). First paint / orientation may still be a GUI
vector. `reason=dirty inplace=1` is transform/erase. `EPAPER_RASTER=0` disables.

```bash
ssh root@10.11.99.1 'tail -f /tmp/epaper-raster.log'
```

If status says `libqsgepaper unavailable`, symbols changed on this firmware —
the app still draws via stock Qt epaper, just without Pen partials.

---

## 7. Failure modes to watch

1. **Unsized canvas** — never `anchors.fill: parent` inside `TabletWindow`; the
   content root stays `0x0` and the ink layer silently stops painting.
2. **`setProperty("mode", Pen)` fails** — moc layout differs; check
   `EpaperBridge.status` on screen.
3. **`swapBuffers` before SG paint** — only relevant under `RM_EP_SWAP`;
   mitigated by `afterRendering` + `QueuedConnection|SingleShotConnection`.
4. **Opaque object size too small** — `kOpaqueBytes = 4096`; bump if ctor
   crashes.
5. **Freeing the opaque items** — `EPScreenModeItem` / `EPRenderBlocker` are
   parented to the canvas, so Qt destroys them; the bridge must not `free()` its
   backing storage as well (that was a double free on exit).
6. **Firmware skew** — mangled names change; bridge falls back gracefully.
7. **Thread warning** — historical `Cannot create children for a parent that
   is in a different thread` in logs; keep all FB calls on the GUI thread.
