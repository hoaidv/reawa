@SRS-IN-16
Feature: Selection create requires surround stroke
  As Infini Selection tool
  I need Smart Group create from selection only when a surround stroke qualifies
  So that AABB-only groups are refused

  # STORY-IN-017 — SRS-IN-16

  @SRS-IN-16
  Scenario: Surround winner becomes boundary
    Given at least two selected inks where one stroke surrounds at least 80 percent of every other
    When create Smart Group runs
    Then the winner is role boundary
    And others are role content with layoutOffset UV
    And bounds equal the winner AABB

  @SRS-IN-16
  Scenario: No qualifying surround refuses create
    Given a selection with no surround at the 80 percent bar
    When create Smart Group runs
    Then no Smart Group is created
    And the selection is unchanged
    And refuse UI is indicated per Spec

  @SRS-IN-16
  Scenario: Later sibling wins among qualifying surrounds
    Given several selected strokes that each qualify as surrounds
    When create Smart Group runs
    Then the later sibling wins as boundary

  @SRS-IN-16
  Scenario: Undo restores prior snapshot after success
    Given a successful selection create
    When undo runs
    Then the prior snapshot restores
