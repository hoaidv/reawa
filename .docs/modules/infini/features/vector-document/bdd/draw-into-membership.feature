@SRS-IN-15 @deprecated
Feature: Draw-into membership for existing Smart Groups
  As Infini session SoT
  I need ordinary ink drawn into a Smart Group to join as content
  So that free layout membership works without expanding bounds

  # DEPRECATED 2026-08-13 — CHL-0008 / ADR-0014. Membership re-homed to the device;
  # the live spec is epaper/features/ink-box/bdd/draw-into-membership.feature (@SRS-EP-10).
  # Kept as the acceptance evidence for STORY-IN-016.

  # STORY-IN-016 — SRS-IN-15

  @SRS-IN-15
  Scenario: Pen stroke joins Smart Group with own UV
    Given a Smart Group with existing content that has layoutOffset UV
    And a new Pen stroke with at least 80 percent of samples inside that group world bounds
    When stroke_end runs membership
    Then the new ink is reparented as role content under that group
    And the new ink has its own layoutOffset UV seeded
    And existing content layoutOffset values are unchanged
    And Smart Group bounds are not expanded

  @SRS-IN-15
  Scenario: Later sibling wins among overlapping groups
    Given several Smart Groups whose world bounds qualify for the same stroke
    When membership runs
    Then the ink joins the later sibling only
    And the ink has exactly one parent

  @SRS-IN-15
  Scenario: No qualifying group leaves ordinary parent
    Given a Pen stroke outside every Smart Group world bounds
    When stroke_end runs
    Then the ink stays under its ordinary parent
    And no Smart Group children change

  @SRS-IN-15
  Scenario: Membership undo restores prior snapshot
    Given a successful draw-into membership
    When undo runs
    Then the tree matches the pre-membership snapshot
