---
captured: 2026-08-31
verified: 2026-08-31
related:
  - SRS-EP-11
  - SRS-EP-18
  - SRS-EP-20
  - BR-B19
  - BR-C06
  - CHL-0018
---

# Connector live preview vs settle — regression note

**Verified 2026-08-31 on RM2:** move/resize a bound SmartGroup — ToolCanvas live connector matches
the drag; origin connector is gone for the gesture; settle is one spine (no leftover origin, no
missing middle).

## What works

| Surface | During drag | After pen-up |
|---|---|---|
| ToolCanvas | Live re-warp + live node (`renderSubtree` + `boundConnectorsPanelUnion` dirty). Preview was already correct. | Live copy gone. |
| TabletCanvas | Origin **box and bound connector spines** suppressed (`collectManipSuppressIds`) and punched (`InPlaceDirty` of origin∪connectors). | `refreshAllConnectorWarps` then InPlaceDirty of **origin ∪ live box ∪ live connector spines**. |

Warp itself was not the bug. `set_smart_transform` still emits **no** connector op ([BR-C06](../modules/epaper/features/connector-ink/srs-product.md)).

## What was broken (and how it looked)

InPlaceDirty used only the **SmartGroup AABB**. Suppress already hid the connector in the *clip*, but
pixels **outside** that rect were never `clearRect`’d.

| Symptom | Why |
|---|---|
| Origin connector not fully hidden during drag | Spine outside the box AABB stayed on `m_image`. |
| Leftover origin connector after settle | Same pixels never cleared. |
| Missing middle of the new spine | New warp’s middle also sits outside both box AABBs; only the ends that overlap the boxes were painted. |
| Ends / partial spine correct | Those segments intersected the box dirty rect. |

## What does not (still)

| Path | Behaviour |
|---|---|
| Mid-gesture e-ink ghosting | Allowed (BR-B15). Fail only if the **settled** frame disagrees. |
| Very long spine | Union AABB can exceed 50% of the panel → FullClear fallback. Correctness over hitch. |
| Connector **create** (pen-up recog) | Separate path: `m_encloseDirtyPanel` = spine ∪ two boxes. Not this punch. |
| Delete bound box / orphan pose | SRS-EP-18; not covered by this punch. |

## Regression check

Move a connected box across a gap. Fail if the old spine remains, if the new spine has a hole in
the middle, or if ToolCanvas still shows a second copy after settle.

Code: `TransformGesture` begin/commit/abort unions `DocContext::boundConnectorsPanelUnion`.
Tablet: `warpedConnectorPanelRect`. Test: `test_connector_spine_outside_box_aabb` in
`epaper/tests/live_manip_overlay_test.cpp`.

Ink hitch that shares InPlaceDirty: [ink-path-density-hitch.md](./ink-path-density-hitch.md).
