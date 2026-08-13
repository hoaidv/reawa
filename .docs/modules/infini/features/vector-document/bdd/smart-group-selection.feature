@SRS-IN-11 @deprecated
Feature: Smart Group selection hit-test move resize and fixedInk UV
  As Infini Selection tool
  I need to pick Smart Groups, move/resize them, and honour inkScaleMode
  So that ink-box manipulation matches SRS-IN-11

  # DEPRECATED 2026-08-13 — CHL-0008 / ADR-0014. Manipulation re-homed to the device; the
  # live spec is epaper/features/ink-box/bdd/smart-group-selection.feature (@SRS-EP-11),
  # which drops the ghost and the pan-below-LOD fallback. Kept as the acceptance evidence
  # for STORY-IN-015 — and as the record of a model that failed human verify four times.

  # STORY-IN-015 — SRS-IN-11

  @SRS-IN-11
  Scenario: Hit selects topmost SmartGroup above LOD
    Given scale is at least TILE_LOD_SCALE
    And the tree has SmartGroup siblings "sg_a" then "sg_b" with overlapping world bounds
    When the pointer presses inside the overlap in Selection tool
    Then "sg_b" is selected (topmost / later sibling)
    And SelectionOverlay shows bounds for the selected node

  @SRS-IN-11
  Scenario: Drag move emits one transform op and does not pan
    Given scale is at least TILE_LOD_SCALE
    And SmartGroup "sg_1" is selected
    When the user presses inside its bounds and drags then releases
    Then exactly one set_smart_transform was applied on release
    And the canvas viewport translate is unchanged by that drag

  @SRS-IN-11
  Scenario: fixedInk resize preserves per-ink UV and sample size
    Given a SmartGroup with inkScaleMode fixedInk
    And content inks with distinct layoutOffset UV values
    When bounds resize
    Then each content ink keeps its layoutOffset UV
    And each content ink sample AABB size is unchanged (±1 px at 100% scale)
    And smartLocalToWorld places content using UV (not translate-only)

  @SRS-IN-11
  Scenario: withBounds resize scales content; boundary always transforms
    Given a SmartGroup with inkScaleMode withBounds
    And content ink plus boundary ink children
    When the group scale or bounds grow
    Then content samples scale with the group
    And boundary ink samples also transform with the group

  @SRS-IN-11
  Scenario: Below LOD picking is disabled and pan wins
    Given scale is below TILE_LOD_SCALE
    And a pickable SmartGroup exists under the pointer
    When the pointer presses and drags
    Then no SmartGroup is selected
    And the viewport pans
