@SRS-EP-21
Feature: One-finger hand-touch pick move palm and empty pan
  As an RM2 creator with Pen armed
  I need a finger on a Smart Group to select and move it, a short empty touch to rest, and travel past 20 mm to pan locally
  So that the hand shoves objects and the local camera without stealing ToolChip taps

  # STORY-EP-038 — SRS-EP-21 / SRS-EP-23 / SRS-EP-25. UI-EP-06 states
  # hand.finger_hit_box, hand.finger_moving, hand.finger_resizing,
  # hand.one_finger_empty_palm, hand.one_finger_empty_pan.
  # Independent cameras default (ADR-0029). Publish only if Infini follow is on.

  @SRS-EP-21 @SRS-EP-23 @SRS-EP-25
  Scenario: Finger-down inside a box arms sel_freeform and selects
    Given exclusive tool pen is armed
    And SmartGroup "sg_1" is in the device document at or above the device LOD cutoff
    When the creator finger-downs inside the SmartGroup world bounds
    Then exclusive tool is sel_freeform
    And "sg_1" is selected
    And region ToolChip shows tool.sel_freeform armed
    And both recognizer toggles are dimmed and remain armed
    And ovl.selection_bounds is visible
    And the chip update completes within p95 300 ms
    And region ToolChip remains finger-hittable

  @SRS-EP-21 @SRS-EP-25
  Scenario: Finger drag inside the selected box moves live-direct with 0 pan
    Given exclusive tool sel_freeform is armed
    And SmartGroup "sg_1" is selected
    And the creator finger is down inside the box bounds and not on a resize knob
    When the finger moves
    Then the real ink of "sg_1" follows the finger
    And committed geometry on lift equals the last previewed geometry with 0 px jump
    And partial refresh runs at 5 Hz or better
    And 0 viewport pan starts
    And follow.direction is unchanged

  @SRS-EP-21 @SRS-EP-25
  Scenario: Finger on a resize knob resizes live-direct with 0 pan
    Given exclusive tool sel_freeform is armed
    And SmartGroup "sg_1" is selected
    When the creator finger-downs on a resize knob and the finger moves
    Then the box resizes with inkScaleMode still applied
    And committed geometry on lift equals the last previewed geometry with 0 px jump
    And partial refresh runs at 5 Hz or better
    And 0 viewport pan starts
    And exclusive tool remains sel_freeform

  @SRS-EP-21 @SRS-EP-25
  Scenario: One-finger empty travel at or below 20 mm is palm-rest
    Given exclusive tool pen is armed
    And 0 nodes are selected
    And the creator finger-downs on empty canvas with hit.kind empty
    When Euclidean panel travel stays at 8 mm (at or below 20 mm / 178 du at 226 dpi) and the touch ends
    Then exclusive tool remains pen
    And 0 nodes are selected
    And 0 lasso starts
    And 0 pan occurs
    And the world layer is unshifted

  @SRS-EP-21 @SRS-EP-25
  Scenario: One-finger empty tap deselects the selected box
    Given exclusive tool sel_freeform is armed
    And SmartGroup "sg_1" is selected
    And the creator finger-downs on empty canvas with hit.kind empty
    When Euclidean panel travel stays at or below 20 mm / 178 du and the touch ends
    Then "sg_1" is not selected
    And 0 nodes are selected
    And exclusive tool remains sel_freeform
    And 0 pan occurs

  @SRS-EP-21 @SRS-EP-25
  Scenario: Pen proximity or contact disables canvas hand-touch
    Given exclusive tool pen is armed
    And the pen is in digitizer proximity or in contact
    When the creator finger-downs on empty canvas or on a SmartGroup
    Then 0 pan occurs
    And 0 pick or move starts
    And 0 pinch starts
    And ToolChip taps still arm tools

  @SRS-EP-21 @SRS-EP-25
  Scenario: Three or more contacts are palm
    Given exclusive tool pen is armed
    And the creator has three capacitive contacts on empty canvas
    When any contact moves
    Then 0 pan occurs
    And 0 pinch starts

  @SRS-EP-21 @SRS-EP-25
  Scenario: Hand-touch toggle off disables canvas hand-touch
    Given exclusive tool pen is armed
    And the hand-touch toggle is off
    When the creator finger-downs on empty canvas or on a SmartGroup
    Then 0 pan occurs
    And 0 pick or move starts
    And ToolChip taps still arm tools

  @SRS-EP-21 @SRS-EP-25
  Scenario: One-finger empty travel past 20 mm pans locally with Infini unchanged
    Given exclusive tool pen is armed
    And follow.direction is none
    And the creator finger-downs on empty canvas with hit.kind empty
    When Euclidean panel travel goes to 36 mm (past 20 mm / 178 du at 226 dpi)
    Then the local Epaper drawingRegion translates
    And exclusive tool remains pen
    And 0 nodes are selected
    And 0 lasso starts
    And Infini's camera is unchanged
    And Epaper emits 0 viewport up messages

  @SRS-EP-23 @SRS-EP-25
  Scenario: ToolChip 64 du tap still holds REQ-03
    Given exclusive tool pen is armed
    And a 64 du ToolChip primary tile tool.sel_rect is visible
    When the creator taps that tile
    Then exclusive tool becomes sel_rect within p95 300 ms
    And only the chip region needs refresh
    And 0 empty-canvas pan starts
    And 0 SmartGroup is selected by that tap

  @SRS-EP-21 @SRS-EP-25
  Scenario: One-finger empty pan while following Infini turns Epaper follow off
    Given follow.direction is infini_to_epaper
    And exclusive tool pen is armed
    And the creator finger-downs on empty canvas with hit.kind empty
    When Euclidean panel travel goes to 36 mm (past 20 mm / 178 du at 226 dpi)
    Then follow.direction becomes none before the local pan applies
    And the gesture drives the local camera
    And 0 continued Infini viewport apply occurs after that gesture starts
