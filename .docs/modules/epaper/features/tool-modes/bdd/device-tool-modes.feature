@SRS-EP-04
Feature: Epaper device tool modes
  As an RM2 creator
  I need Selection Pen and Ink-box on a floating chip
  So that every tool acts on the device's own document without leaving the tablet

  # Revised 2026-08-13 — CHL-0008 / ADR-0014. The intent-emission scenarios are replaced
  # by local-operation scenarios: arming a tool changes what happens at pen-up on this
  # device, not what a peer is told. Creation and manipulation detail lives in
  # epaper/features/ink-box/bdd/.

  # STORY-EP-005 — SRS-EP-04 / SRS-EP-06

  @SRS-EP-04
  Scenario: Ink-box evaluates the stroke locally at pen-up
    Given the Ink-box tool is armed
    When a stroke begins on the canvas
    Then local ink paints immediately
    And no intent field is placed on the wire
    When the stroke ends
    Then enclose evaluation runs on the device

  @SRS-EP-04
  Scenario: Pen ingests the stroke and evaluates membership
    Given the Pen tool is armed
    When a stroke ends
    Then the stroke becomes an Ink node in the device document
    And draw-into membership is evaluated locally

  @SRS-EP-04
  Scenario: Pen and Ink-box are identical while the pen is down
    Given the same stroke drawn once with Pen armed and once with Ink-box armed
    When the samples and the local raster are compared
    Then they are identical
    And the tools differ only in what happens at pen-up

  @SRS-EP-04
  Scenario: Selection acts on the local document
    Given the Selection tool is armed
    When a drag on a Smart Group completes
    Then the transform was applied to the local document during the drag
    And exactly one change publishes on release
    And no tool_intent message is sent

  @SRS-EP-04
  Scenario: Tools stay available with the session down
    Given the session to the desktop is down
    When the creator arms Selection or Ink-box
    Then both tools are available
    And the chip shows that changes are queued

  @SRS-EP-04
  Scenario: Tool mode is never synced
    Given tool mode changes on the device
    When observing outbound session messages
    Then no message type carries a device tool mode field

  @SRS-EP-06
  Scenario: Tool switch does not invalidate full ink panel
    Given the ToolChip is tapped to change tools
    When the mode changes
    Then only the chip region needs refresh
    And ink surface content is preserved
