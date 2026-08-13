@SRS-IN-12 @deprecated
Feature: Snapshot undo ring for vector document
  As Infini creator
  I need structural ops to be undoable via snapshot restore
  So that false Smart Group creates and transforms are safe to try

  # DEPRECATED 2026-08-13 — CHL-0008 / ADR-0014 §5. Undo belongs where editing happens, so
  # the ring moved to the device: epaper/features/device-document/bdd/undo-ring.feature
  # (@SRS-EP-07). Mechanism and depth are inherited verbatim. Kept as the acceptance
  # evidence for STORY-IN-014.

  # STORY-IN-014 — SRS-IN-12

  @SRS-IN-12
  Scenario: Structural op pushes a snapshot before apply
    Given an empty undo ring on a VectorDocument
    When a structural op create_frame is applied through the undo-aware path
    Then the undo ring depth is 1
    And the ring entry equals the pre-op snapshotString

  @SRS-IN-12
  Scenario: Undo restores the prior tree exactly
    Given a VectorDocument with at least one structural op on the undo ring
    When undo runs
    Then the tree snapshotString equals the popped pre-op snapshot
    And a second undo with an empty ring is a no-op

  @SRS-IN-12
  Scenario: Viewport and selection do not push undo
    Given an undo ring with depth 0
    When only viewport pan or tool or selection state changes
    Then the undo ring depth remains 0

  @SRS-IN-12
  Scenario: Ring overflow drops the oldest entry
    Given an undo ring depth of 20
    When a 21st structural snapshot would push
    Then the ring depth remains 20
    And the oldest entry is gone
