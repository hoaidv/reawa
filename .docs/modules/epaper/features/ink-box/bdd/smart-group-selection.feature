@SRS-EP-11
Feature: Smart Group selection hit-test move resize and fixedInk UV on the device
  As an RM2 creator using Selection
  I need to pick Smart Groups, move and resize them, and honour inkScaleMode
  So that manipulation is direct and the released geometry is the committed geometry

  # Re-homed 2026-08-13 from infini SRS-IN-11 (deprecated) — CHL-0008 / ADR-0014.
  # Changed: the ghost is gone, hit-testing reads the local document, and below the
  # LOD cutoff the press falls through to nothing (there is no on-device pan).

  @SRS-EP-11
  Scenario: Hit selects topmost SmartGroup above the LOD cutoff
    Given scale is at or above the device LOD cutoff
    And the device document has SmartGroup siblings "sg_a" then "sg_b" with overlapping world bounds
    When the pen presses inside the overlap with Selection armed
    Then "sg_b" is selected (topmost / later sibling)
    And the selection overlay shows bounds for the selected node
    And the selection affordance appears within 100 ms

  @SRS-EP-11
  Scenario: Drag moves the real ink and emits one op on release
    Given scale is at or above the device LOD cutoff
    And SmartGroup "sg_1" is in the device document
    When the pen presses inside its bounds and drags then releases
    Then the ink itself moved during the drag
    And no advisory ghost was drawn
    And exactly one set_smart_transform was committed on release
    And the committed geometry equals the last previewed geometry

  @SRS-EP-11
  Scenario: Drag feedback stays inside the partial-refresh budget
    Given a move gesture in progress
    When feedback renders
    Then updates occur at 5 Hz or better
    And no stall exceeds 200 ms
    And zero full-panel invalidations occur

  @SRS-EP-11
  Scenario: fixedInk resize preserves per-ink UV and sample size
    Given a SmartGroup with inkScaleMode fixedInk
    And content inks with distinct layoutOffset UV values
    When bounds resize
    Then each content ink keeps its layoutOffset UV
    And each content ink sample AABB size is unchanged (±1 px at 100% zoom)
    And unrelated content inks did not move

  @SRS-EP-11
  Scenario: withBounds resize scales content; boundary always transforms
    Given a SmartGroup with inkScaleMode withBounds
    And content ink plus boundary ink children
    When the group scale or bounds grow
    Then content samples scale with the group
    And boundary ink samples also transform with the group

  @SRS-EP-11
  Scenario: Inverted resize normalizes before commit
    Given a selected SmartGroup
    When a handle is dragged past the opposite edge and released
    Then bounds width and height are non-negative
    And content and boundary follow the normalized rect
    And no negative-size state is committed or published

  @SRS-EP-11
  Scenario: Below the LOD cutoff manipulation is unavailable and inert
    Given scale is below the device LOD cutoff
    And a SmartGroup exists under the pen
    When the pen presses and drags
    Then no SmartGroup is selected
    And no transform is applied
    And the UI states that manipulation is unavailable

  @SRS-EP-11
  Scenario: Deselect leaves no residue
    Given a selected SmartGroup
    When the pen presses empty canvas
    Then the selection clears
    And the next settled frame shows zero residual selection pixels

  @SRS-EP-11
  Scenario: Gesture survives a link drop
    Given a move gesture in progress
    When the session drops before release
    Then the gesture completes and commits locally
    And the change is queued for publication

  @SRS-EP-11
  Scenario: Manipulation dispatches through the capability descriptor
    Given SmartGroup is registered with the shared capability descriptor
    When the gesture router resolves a press on a SmartGroup
    Then the declared verbs are select, move, resize, and set-ink-scale-mode
    And no node-kind branch is used to route the gesture
    And the emitted transform op carries an unset reserved rotation field
