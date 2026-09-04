@SRS-EP-10
Feature: Draw-into membership for existing Smart Groups on the device
  As an RM2 creator writing inside a box
  I need ordinary ink drawn into a Smart Group to join as content
  So that free layout membership works without expanding bounds

  # Re-homed 2026-08-13 from infini SRS-IN-15 (deprecated) — CHL-0008 / ADR-0014.
  # Rules verbatim; evaluation runs on the device at pen-up.

  @SRS-EP-10
  Scenario: Pen stroke joins Smart Group with own UV
    Given a Smart Group with existing content that has layoutOffset UV
    And a new Pen stroke with at least 80 percent of its polyline length inside that group's boundary ink
    When pen-up runs membership on the device
    Then the new ink is reparented as role content under that group
    And the new ink has its own layoutOffset UV seeded
    And existing content layoutOffset values are unchanged
    And Smart Group bounds are not expanded
    And the reparent completes within p95 300 ms after pen-up

  @SRS-EP-10
  Scenario: Later sibling wins among overlapping groups
    Given several Smart Groups whose boundary ink qualifies for the same stroke
    When membership runs
    Then the ink joins the later sibling only
    And the ink has exactly one parent

  @SRS-EP-10
  Scenario: No qualifying group leaves ordinary parent
    Given a Pen stroke outside every Smart Group boundary ink
    When pen-up runs
    Then the ink stays under its ordinary parent
    And no Smart Group children change

  @SRS-EP-10
  Scenario: Membership never reflows existing content
    Given a Smart Group with three content inks
    When a new qualifying stroke joins the group
    Then zero existing content inks moved
    And no content ink was rescaled

  @SRS-EP-10
  Scenario: AABB inside without boundary-ink length does not join
    Given a Smart Group whose world AABB contains a new Pen stroke
    And less than 80 percent of that stroke's polyline length lies inside the group's boundary ink
    When membership runs
    Then the ink stays under its ordinary parent
    And no Smart Group children change

  @SRS-EP-10
  Scenario: Membership undo restores the prior tree
    Given a successful draw-into membership
    When undo runs
    Then the tree matches the pre-membership snapshot
