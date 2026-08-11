@SRS-EP-04
Feature: Epaper device tool modes
  As an RM2 creator
  I need Selection Pen and Ink-box on a floating chip
  So that intent and picking work without leaving the tablet

  # STORY-EP-005 — SRS-EP-04 / SRS-EP-06

  @SRS-EP-04
  Scenario: Ink-box arms enclose intent on stroke_begin
    Given the Ink-box tool is armed
    When a stroke begins on the canvas
    Then stroke_begin includes intent enclose
    And local ink still paints immediately

  @SRS-EP-04
  Scenario: Pen emits ink intent by default
    Given the Pen tool is armed
    When a stroke begins
    Then stroke_begin includes intent ink or omits intent

  @SRS-EP-04
  Scenario: Selection emits tool_intent on gesture complete
    Given pickables from the latest doc_snapshot
    When Selection drag on a pickable completes
    Then one tool_intent message is sent
    And the local ghost is discarded on the next doc_snapshot

  @SRS-EP-06
  Scenario: Tool switch does not invalidate full ink panel
    Given the ToolChip is tapped to change tools
    When the mode changes
    Then only the chip region needs refresh
    And ink surface content is preserved
