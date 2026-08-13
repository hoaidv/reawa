@SRS-EP-10
Feature: Selection create requires a surround stroke on the device
  As an RM2 creator promoting a selection
  I need Smart Group create from selection only when a surround stroke qualifies
  So that AABB-only groups are refused and a box always reads as a box

  # Re-homed 2026-08-13 from infini SRS-IN-16 (deprecated) — CHL-0008 / ADR-0014.
  # Rules verbatim, including the even-odd artificial-closed-path test.

  @SRS-EP-10
  Scenario: Surround winner becomes boundary
    Given at least two selected inks where one stroke surrounds at least 80 percent of every other
    When create Smart Group runs on the device
    Then the winner is role boundary
    And others are role content with layoutOffset UV
    And bounds equal the winner AABB

  @SRS-EP-10
  Scenario: Open surround stroke qualifies via an artificial closed path
    Given a selection whose candidate surround stroke is open
    When containment is tested with the even-odd rule on the artificial closed path
    Then the candidate may qualify
    And the stored samples of that stroke are unchanged

  @SRS-EP-10
  Scenario: No qualifying surround refuses create
    Given a selection with no surround at the 80 percent bar
    When create Smart Group runs
    Then no Smart Group is created
    And the selection is unchanged
    And the refuse reason is visible

  @SRS-EP-10
  Scenario: Later sibling wins among qualifying surrounds
    Given several selected strokes that each qualify as surrounds
    When create Smart Group runs
    Then the later sibling wins as boundary

  @SRS-EP-10
  Scenario: Undo restores the prior tree after success
    Given a successful selection create
    When undo runs
    Then the prior snapshot restores
