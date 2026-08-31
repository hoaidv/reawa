---
from: dev
to: human
date: 2026-08-31
story: interrupt TRACK-005 / SRS-EP-01
---

# Guided review — skip FullClear, InPlaceDirty, NodeEmphasis

Read this in order. Each **Dnn** is: why → what was coded → how to read it → misread trap.
Do not swim the whole diff first.

Challenges: [CHL-0029](../challenges/CHL-0029-settle-is-not-fullclear-on-ink.md),
[CHL-0030](../challenges/CHL-0030-node-emphasis.md).

---

## D01 — Ordinary `append_ink` does not rasterize the document

**Why:** Live stamps already match the new free-ink node. FullClear walked every on-camera ink and queued the next down.

**Coded:** [`ingestCurrentStroke`](../../../epaper/drawing/tabletcanvasitem.cpp) `else` branch (ordinary Ink / failed enclose) emits `recogChrome(0)` and does **not** set `m_needEncloseRasterize`. `endStroke` only rasterizes if enclose flag, camera-deferred sharp, or pending soft.

**Read:** `endStroke` then the Ink `else` in `ingestCurrentStroke`. Camera-during-stroke still uses `m_rasterizeDeferredSharp` / `m_rasterizePending`.

**Misread:** Skipping is not “forget the node”. The node is in `DeviceDocument`. Pixels are already on `m_image`.

---

## D02 — Delete the 180 ms settle follow-up

**Why:** Second FullClear ~180 ms after the first; handwriting pauses are longer than 180 ms, so `CancelSettle` lost.

**Coded:** `scheduleVectorRasterize` sharp path calls `rasterizeVectors(true)` once. No `QTimer` follow-up. `kSettleFollowUpMs` / `m_settleFollowUpToken` removed. Soft camera still 250 ms.

**Read:** `TabletCanvasItem::scheduleVectorRasterize`.

**Misread:** Soft 250 ms is still there for `cameraChanged`. That is SRS-EP-03 coalesce, not the hitch.

---

## D03 — InPlaceDirty tightens `worldClip` and clips the painter

**Why:** A small `clearRect` plus a full-tree paint is still a 600 ms hitch and overpaints the rest of `m_image`.

**Coded:**
- [`intersectWorldAabb`](../../../epaper/rendering/rendering.hpp)
- [`QImagePixelSink::clearRect`](../../../epaper/rendering/rendering_qt.hpp) sets `QPainter::setClipRect`
- `rasterizeVectors(bool, QRectF)`: if dirty valid and ≤50% of panel, `InPlaceDirty` + `worldClip = camera ∩ dirty world` + `update(dirty)`; else FullClear
- [`test_inplace_dirty_clearRect_and_tight_clip`](../../../epaper/tests/rendering_test.cpp)

**Read:** `rasterizeVectors` `useDirty` branch, then `QImagePixelSink::clearRect`.

**Misread:** `paint()` still only `drawImage`s `m_image`. The document walk is only in `rasterizeVectors`.

---

## D04 — No world-AABB cache

**Why:** Stale cache after enclose → white holes. Not in this change.

---

## D05 — `noteDocumentMutated` stays FullClear; `noteDocumentDirty` is explicit

**Why:** Silent reinterpretation of `documentMutated` would hole the panel when a caller forgets a rect.

**Coded:** `DocContext::noteDocumentDirty` → `scheduleDirtyRasterize` then `documentMutated`. Tablet `documentMutated` slot **returns** if `m_consumeMutatedRasterize` (set by dirty schedule). Same-turn FullClear still happens if someone calls `noteDocumentMutated` without dirty first.

**Read:** `SessionDocContext::noteDocumentDirty`, Tablet ctor `documentMutated` lambda, `scheduleDirtyRasterize`.

**Misread:** `documentMutated` still FullClears when consume is false (camera-unrelated unknown mutation).

---

## D06 — Enclose / connector document paint is one InPlaceDirty (no blink on Tablet)

**Why:** Group-local children are not in live panel stamps. One bake of the new group (or connector ∪ two boxes) is still required. Blink is not a second Tablet FullClear.

**Coded:** `m_encloseDirtyPanel` set in ingest; `endStroke` `rasterizeVectors(true, dirty)`. Tablet `m_blink*` / `beginRecogWidthBlink` / highlight styles **deleted**.

**Read:** ingest enclose/connector branches, then `endStroke` enclose arm.

**Misread:** Membership does **not** set `m_needEncloseRasterize` (live stamps + ToolCanvas bold).

---

## D07 — Erase commit dirties the clip / lasso AABB

**Why:** `noteDocumentMutated` after erase was a FullClear hitch on a filled page.

**Coded:** Brush: capsule path AABB + radius×scale. Area: panel poly AABB. Object: union of `m_hitPanelRects` **before** clear.

**Read:** `commit()` in brush / area / object erase — `noteDocumentDirty` vs empty fallback FullClear.

**Misread:** Object dirty must be snapshotted before `m_hitPanelRects.clear()`.

---

## D08 — Live manip: dirty origin ∪ live, never FullClear mid-gesture

**Why:** SRS-EP-11: 0 full-panel invalidations during manipulation. Begin only needs to punch `suppressIds` in the origin AABB. `apply` is overlay-only.

**Coded:** `TransformGesture` stores `m_originPanel`. begin/commit/abort call `noteDocumentDirty`. `apply` unchanged (no Tablet rasterize).

**Read:** `transform_gesture.hpp` begin punch, then commit `punch` lambda.

**Misread:** Mid-drag still does not call `noteDocument*`. Overlay is ToolCanvas.

---

## D09 — Undo/redo: target AABB union, else FullClear

**Why:** A hole after undo is worse than a hitch on a rare action. Erase/enclose undo often drops the original id.

**Coded:** `applyHistoryRestore` copies target ids, unions `panelBoundOfNodeId` before and after, then dirty or FullClear. Also `emitRecogChrome(0)` (clear membership stamp). `UndoStack::newestRedo` added.

**Read:** `applyHistoryRestore` then `newestRedoEntry` on `DeviceDocument`.

**Misread:** Empty dirty (typical erase undo) **intentionally** FullClears.

---

## D10 — Camera, `doc_load`, first show, resize stay FullClear

**Coded:** `cameraChanged` still `scheduleVectorRasterize(false)`. `doc_load` still sharp FullClear. No change to that policy.

---

## D11 — Spec: ink latency outranks “immediate full redraw on commit”

**Coded:** [CHL-0029](../challenges/CHL-0029-settle-is-not-fullclear-on-ink.md) adopted. No SRS table rewrite this slice.

---

## D12 — This file

You are reading it.

---

## D13 — `NodeEmphasis` is the one owner

**Why:** Blink / last-join bold / AABB highlight are not selection chrome and not an in-flight Operation. Tablet flags FullCleared the document.

**Coded:**
- [`node_emphasis.hpp/.cpp`](../../../epaper/drawing/tools/ui/node_emphasis.hpp)
- `ToolCanvasItem` owns `m_emphasis`; `HostCaps.emphasis`
- `InteractionMode::paintNodeEmphasis`; Ink / Selection / Eraser all call it (emphasis **first**, then Operation)
- InkMode `syncOverlay`: visible if Transforming **or** `emphasis->active()`; waveform Mono
- Tablet ingest `CanvasSession::recogChrome`; ToolCanvas connects and calls blink/stamp/clear
- Tool switch: `clearStrokeStamp` + `hideAllAabbs`

**Read:** `node_emphasis.hpp` API, then `ToolCanvasItem::setSession` `recogChrome` lambda, then Mode `paintOverlay` first line.

**Misread:** Do not look for blink in `tabletcanvasitem.cpp` paint/rasterize. `ToolCanvasItem::paint` still only forwards to Mode.

---

## D14 — Overlay 2× on document 1×; teardown dirties the halo

**Why:** Punching Tablet `suppressIds` for blink would InPlaceDirty twice. On 1-bit the extra width is a halo. ToolCanvas `fillColor` is transparent.

**Coded:** `blink` sets overlay visible + `setStrokeWaveform(false)`. 250 ms timer clears ids and `damage` + `syncOverlayPresence`. `QPainterPixelSink` clear is a no-op, so overlay paint does not white-fill.

**Read:** `NodeEmphasis::blink` timer lambda, then `paint`.

**Misread:** Overlay `DocumentRenderer::render` with tight `worldClip` still visits overlapping neighbors at 1× (no-op on 1-bit). Styled ids are 2×.

---

## D15 — Partial update: union of emphasized panel AABBs

**Coded:** `computeDirty` / `damageChrome` of that union. AABB show uses `damageChromeSegment` outline strips. Paint `worldClip` from `m_dirty`.

**Read:** `NodeEmphasis::computeDirty` and `paint` `req.worldClip`.

---

## D16 — Recog mapping

| Outcome | Tablet | ToolCanvas |
|---|---|---|
| Enclose Created | one InPlaceDirty of group AABB | `blink(group + inks)` |
| Connector | InPlaceDirty spine ∪ two boxes | `blink(body ∪ boxes)` |
| Membership | skip | `setStrokeStamp(boundary, Bold)` |
| Ordinary Ink / fail | skip | `clearStrokeStamp` |

**Read:** `ingestCurrentStroke` outcomes + ToolCanvas `recogChrome` kinds 0–3.

---

## D17 — Emphasis API (the one place)

```
blink(ids, ms=250)
setStrokeStamp(ids, Off|Bold|Dotted)
clearStrokeStamp()
showAabb / hideAabb / hideAllAabbs
paint()  // Mode only
```

`Stamp::Dotted` and `showAabb` are implemented. This slice’s callers are blink + Bold membership only. Object erase keeps `m_hitPanelRects`.

---

## Device verify (not run in this session)

`cd epaper && ./scripts/deploy-rm2.sh --build`

- Recog off, dense page, continuous writing: no `queued behind=rasterizeVectors` on down
- Recog on: enclose blinks on ToolCanvas; no 250 ms second Tablet FullClear
- Draw-into: boundary bold on ToolCanvas
- Connector: spine + two boxes, not the whole page
