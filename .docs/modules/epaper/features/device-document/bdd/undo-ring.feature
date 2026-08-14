@SRS-EP-07
Feature: Snapshot undo ring on the device document
  As an RM2 creator
  I need structural ops to be undoable via snapshot restore
  So that a wrong box or a slipped drag costs one undo, not a stuck document

  # Re-homed 2026-08-13 from infini SRS-IN-12 (deprecated) — CHL-0008 / ADR-0014 §5.
  # Mechanism and depth verbatim. The shared-timeline scenario is dropped: one writer.

  @SRS-EP-07
  Scenario: Structural op pushes a snapshot before apply
    Given an empty undo ring on the device document
    When a structural op is applied through the undo-aware path
    Then the undo ring depth is 1
    And the ring entry equals the pre-op snapshot

  @SRS-EP-07
  Scenario: Undo restores the prior tree exactly
    Given a device document with at least one structural op on the undo ring
    When undo runs
    Then the tree equals the popped pre-op snapshot exactly
    And a second undo with an empty ring is a no-op

  @SRS-EP-07
  Scenario: One completed gesture is one undo entry
    Given a move gesture that rendered many intermediate frames
    When the gesture is released and committed
    Then the undo ring gained exactly one entry
    And one undo reverts exactly that gesture

  @SRS-EP-07
  Scenario: Viewport tool and selection do not push undo
    Given an undo ring with depth 0
    When only viewport pan or tool or selection state changes
    Then the undo ring depth remains 0

  @SRS-EP-07
  Scenario: Ring overflow drops the oldest entry
    Given an undo ring depth of 20
    When a 21st structural snapshot would push
    Then the ring depth remains 20
    And the oldest entry is gone

  @SRS-EP-07
  Scenario: Undo requested mid-gesture is deferred
    Given a manipulation gesture in progress
    When undo is requested before release
    Then the gesture is not corrupted
    And the undo applies after the gesture commits

  @SRS-EP-07
  Scenario: Undo publishes as a change
    Given a live session and a committed structural op
    When undo runs
    Then a restore_snapshot change is queued for publication
    And the desktop mirror converges to the restored tree

  @SRS-EP-07
  Scenario: An accepted document load clears the ring
    Given an undo ring with entries and an empty publish queue
    When a doc_load is accepted
    Then the undo ring is empty
    And undo cannot reach the pre-load tree

  @SRS-EP-07
  Scenario: Redo restores the undone tree
    Given a structural op that was undone
    When redo runs
    Then the tree equals the pre-undo tree
    And a later structural commit clears the redo stack

  @SRS-EP-07
  Scenario: Empty redo is a no-op
    Given an empty redo stack
    When redo runs
    Then the tree is unchanged
