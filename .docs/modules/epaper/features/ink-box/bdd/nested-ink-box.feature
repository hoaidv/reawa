@SRS-EP-76
@SRS-EP-77
@SRS-EP-75
Feature: Nested ink-boxes
  As a creator who pastes or encloses a box inside a box
  I need the inner box to stay selectable and the empty letter-box to join the paragraph
  So that nested clusters move as I intend

  @SRS-EP-76
  @SRS-EP-77
  Scenario: Nested child is tap-selectable after paste
    Given a non-empty ink-box pasted into another ink-box
    When the creator taps the child
    Then the child is selected
    And move or resize mutates only the child’s own-transform

  @SRS-EP-77
  Scenario: Marquee does not pick nested children
    Given a nested ink-box inside a top-level ink-box
    When sel_rect contains 80 percent of both boxes’ AABBs
    Then only the top-level ink-box is selected

  @SRS-EP-75
  Scenario: Enclose captures a non-empty inner box
    Given a non-empty Smart Group and surrounding free ink with a surround stroke
    When the creator taps Enclose
    Then the Smart Group is a nested child of the new box
    And creation is not refused for nesting

  @SRS-EP-75
  Scenario: Empty inner box flattens
    Given an empty Smart Group (boundary ink only) inside a qualifying enclose
    When enclose commits
    Then the empty wrapper is gone
    And its boundary ink is role content of the parent

  @SRS-EP-76
  @SRS-EP-77
  Scenario: Overflow past parent AABB is neither painted nor hittable
    Given a nested ink-box whose natural AABB lies fully outside its parent’s natural AABB
    When the document paints
    Then the child’s content is not emitted
    And a tap on the child’s AABB does not select the child

  @SRS-EP-77
  Scenario: Move reparents at 80 percent natural area
    Given a nested child whose natural area is 80 percent inside another box after a move
    When the move commits
    Then that box is the new parent
    And a move with less than 80 percent inside every container parents at the document root
