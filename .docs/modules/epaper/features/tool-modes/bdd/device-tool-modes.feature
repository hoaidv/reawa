@SRS-EP-04
Feature: Epaper device tool modes
  As an RM2 creator
  I need Selection, Pen, recognizer toggles, and Undo/Redo on a floating chip
  So that every tool acts on the device's own document without leaving the tablet

  # Revised 2026-08-13 — CHL-0008 / ADR-0014. The intent-emission scenarios are replaced
  # by local-operation scenarios: arming a tool changes what happens at pen-up on this
  # device, not what a peer is told. Creation and manipulation detail lives in
  # epaper/features/ink-box/bdd/.

  # STORY-EP-005 — SRS-EP-04 / SRS-EP-06

  @SRS-EP-04
  Scenario: Ink-box recognizer evaluates the stroke locally at pen-up
    Given exclusive tool pen is armed
    And tgl.recog.ink_box is armed
    When a stroke begins on the canvas
    Then local ink paints immediately
    And no intent field is placed on the wire
    When the stroke ends
    Then enclose evaluation runs on the device as ADR-0022 step 1

  @SRS-EP-04
  Scenario: Pen ingests the stroke and evaluates membership
    Given exclusive tool pen is armed
    When a stroke ends
    Then the stroke becomes an Ink node in the device document
    And draw-into membership is evaluated locally as ADR-0022 step 2 unless a prior step already committed

  @SRS-EP-04
  Scenario: Recognizer toggles do not change paint while the pen is down
    Given the same stroke drawn once with both recognizers armed and once with both disarmed
    When the samples and the local raster are compared
    Then they are identical
    And the toggles differ only in what happens at pen-up

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
    When the creator arms sel_rect or pen or flips either recognizer toggle
    Then all three exclusive tools and both toggles stay available
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

  # STORY-EP-024 inventory superseded 2026-08-15 — STORY-EP-028 / ADR-0021 / UI-EP-04

  @SRS-EP-05
  Scenario: Primary strip is three exclusive tools, two recognizer toggles, then Undo and Redo
    Given the floating ToolChip is visible on launch
    When region ToolChip is observed
    Then exclusive tools are exactly tool.sel_rect, tool.sel_freeform, tool.pen in that order
    And tool.ink_box is absent
    And tgl.recog.ink_box and tgl.recog.connector are independent toggles default armed
    And cta.undo and cta.redo remain actions after a 32 px paper gap
    And tapping Undo or Redo does not change the armed exclusive tool
    And each tool and toggle tile is 64 by 64 px per UI-EP-04 and CHL-0019

  @SRS-EP-05
  Scenario: Selection dims both recognizer toggles and keeps armed state
    Given tool.pen is armed and both recognizer toggles are armed
    When the creator taps tool.sel_rect
    Then both toggles are dimmed and not tappable
    And both toggles remain armed
    When the creator taps tool.pen
    Then both toggles are tappable and still armed

  @SRS-EP-04
  Scenario: Toggle flip mid-stroke uses the pen-down latch
    Given exclusive tool pen is armed and tgl.recog.connector is armed at pen-down
    When the creator disarms tgl.recog.connector before pen-up
    Then dispatch at pen-up uses the latched pen-down tuple with connector recognition still armed

  @SRS-EP-04
  Scenario: Pen inking latency and chip exclusion rect are unchanged
    Given exclusive tool pen is armed
    When the creator inks on InkSurface outside ToolChip bounds
    Then pen-down to pixel p95 stays at most 30 ms
    When pen-down starts inside ToolChip bounds
    Then that press is not ink

  @SRS-EP-05
  Scenario: Undo and Redo on the chip restore document history
    Given at least one structural commit on the device
    When the creator taps Undo
    Then the pre-op tree is restored
    And tapping Redo restores that op when no newer commit has landed
    And a new structural commit after undo makes Redo a no-op
    And tapping Undo or Redo on an empty stack leaves the tree unchanged
