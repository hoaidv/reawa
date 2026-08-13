@SRS-IN-10 @deprecated
Feature: Tool-armed enclose recognition
  As Infini with Ink-box armed
  I need enclose strokes to create Smart Groups immediately when guards pass
  So that arming the tool is the confirmation (no propose/accept)

  # DEPRECATED 2026-08-13 — CHL-0008 / ADR-0014. Recognition re-homed to the device;
  # the live spec is epaper/features/ink-box/bdd/enclose-recognition.feature (@SRS-EP-10).
  # Kept as the acceptance evidence for STORY-IN-010 and as the return path if desktop
  # authoring comes back with multi-directional sync. Do not run in the device suite.

  # STORY-IN-010 — SRS-IN-10 (rewritten)

  @SRS-IN-10
  Scenario: Successful enclose commits Smart Group immediately
    Given tree-backed ink exists inside a region
    And a stroke with intent enclose whose AABB shorter side is at least 48 world units
    And at least one ink has at least 80 percent of samples inside that AABB
    When stroke_end runs enclose recognition on Infini
    Then create_smart_group is committed immediately with no propose or accept step
    And the enclose stroke is role boundary
    And captured ink is role content under the Smart Group
    And bounds equal the fitted AABB
    And each content ink has layoutOffset UV seeded

  @SRS-IN-10
  Scenario: Non-enclose stroke skips recognition
    Given a stroke with intent ink or absent intent
    When stroke_end runs on Infini
    Then enclose recognition does not run
    And the stroke is committed as ordinary ink only

  @SRS-IN-10
  Scenario: Failed guards leave ordinary ink
    Given a stroke with intent enclose that is too small or has no capturable content
    When enclose recognition runs
    Then no Smart Group is created
    And the stroke stays ordinary ink
    And no error banner is required

  @SRS-IN-10
  Scenario: Undo restores pre-create snapshot
    Given a successful enclose create_smart_group
    When undo runs
    Then the tree matches the pre-op snapshot
