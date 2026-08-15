@SRS-EP-10
Feature: Tool-armed enclose recognition on the device
  As an RM2 creator with Ink-box armed
  I need enclose strokes to create Smart Groups on the panel immediately when guards pass
  So that arming the tool is the confirmation and no peer is involved

  # Re-homed 2026-08-13 from infini SRS-IN-10 (deprecated) — CHL-0008 / ADR-0014.
  # Rules unchanged; the trigger is local tool state, not wire intent.

  @SRS-EP-10
  Scenario: Successful enclose commits Smart Group immediately and locally
    Given ink exists in the device document
    And exclusive tool pen and tgl.recog.ink_box were armed at pen-down
    And the stroke AABB shorter side is at least 48 world units
    And at least one ink has at least 80 percent of samples inside that AABB
    When pen-up runs enclose recognition on the device
    Then create_smart_group is committed immediately with no propose or accept step
    And zero messages from the desktop were required
    And the enclose stroke is role boundary
    And captured ink is role content under the Smart Group
    And bounds equal the fitted AABB
    And each content ink has layoutOffset UV seeded
    And the Smart Group is visible on the panel within p95 500 ms after pen-up

  @SRS-EP-10
  Scenario: Stroke drawn with ink-box recognizer disarmed skips enclose
    Given exclusive tool pen was armed at pen-down
    And tgl.recog.ink_box was disarmed at pen-down
    When pen-up runs on the device
    Then enclose recognition does not run
    And dispatch continues at ADR-0022 step 2

  @SRS-EP-10
  Scenario: Tool switched mid-stroke does not change what the stroke means
    Given exclusive tool pen and tgl.recog.ink_box were armed at pen-down
    When tgl.recog.ink_box is disarmed before pen-up
    Then the stroke is still evaluated as an enclose candidate

  @SRS-EP-10
  Scenario: Failed guards leave ordinary ink
    Given an enclose stroke that is too small or has no capturable content
    When enclose recognition runs
    Then no Smart Group is created
    And the stroke stays ordinary ink
    And no error banner is shown

  @SRS-EP-10
  Scenario: Ink already inside a Smart Group is skipped
    Given an enclose over ink whose parent is already a Smart Group and over ungrouped ink
    When enclose recognition runs
    Then the already-grouped ink is skipped
    And the remaining ink is captured

  @SRS-EP-10
  Scenario: Enclose works with the session down
    Given the session to the desktop is down
    When a qualifying enclose is drawn
    Then the Smart Group is created and visible on the panel
    And the change is queued for publication

  @SRS-EP-10
  Scenario: Consecutive encloses stay correct
    Given ten qualifying enclose gestures drawn in sequence
    When each pen-up runs recognition
    Then exactly ten Smart Groups exist
    And no group is lost or desynced

  @SRS-EP-10
  Scenario: Undo restores the pre-create tree
    Given a successful enclose create_smart_group
    When undo runs
    Then the tree matches the pre-op snapshot exactly
