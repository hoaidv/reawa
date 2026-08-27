@SRS-EP-13 @SRS-EP-07
Feature: Undo fail-safe skip and no-op catalogue
  As an RM2 creator
  I need a mismatched or missing undo target to skip or no-op without error
  So that a later field is never clobbered and a dead top-of-stack never traps the next tap

  # STORY-EP-060 / ADR-0032 F20 F21 F1. Device-document only.
  # Matching lastOpId apply, depth 20, ungroup, redo-apply, empty redo stay in
  # undo-ring.feature (12 scenarios — do not regress). Empty redo is already there;
  # this file adds empty undo (F1) plus skip / absence-partial.
  # Infini apply / unknown-desktop-op: out (STORY-IN-038 cancelled).

  @SRS-EP-13
  Scenario: F21 absences no-op and live targets apply
    Given SmartGroup A at world origin (50, 60) with lastOpId "op-move-ab"
    And SmartGroup B is absent from the tree
    And the undo ring depth is 1
    And that newest entry is a multi-node set_smart_transform with forwardOpId "op-move-ab"
    And that entry's targets are A and B, each with prevLastOpId captured before "op-move-ab"
    And that entry's inverses restore A to world origin (10, 20) and B to world origin (80, 90)
    And the redo stack depth is 0
    When undo runs
    Then A's world origin is (10, 20) within ±1 world unit
    And B remains absent
    And 0 extra nodes were inserted
    And the undo ring no longer contains the entry with forwardOpId "op-move-ab"
    And the undo ring depth is 0
    And the redo stack depth is 1
    And exactly 1 doc_change is queued for this undo
    And 0 restore_snapshot ops are queued for publication
    And 0 error UI is shown

  @SRS-EP-13
  Scenario: F20 any sibling lastOpId mismatch skips the whole entry
    Given SmartGroup A at world origin (50, 60) with lastOpId "op-move-ab"
    And SmartGroup B at world origin (120, 140) with lastOpId "op-other-b"
    And the undo ring depth is 1
    And that newest entry is a multi-node set_smart_transform with forwardOpId "op-move-ab"
    And that entry's targets are A and B, each with prevLastOpId captured before "op-move-ab"
    And that entry's inverses restore A to world origin (10, 20) and B to world origin (80, 90)
    And the redo stack depth is 0
    When undo runs
    Then the whole entry is skipped
    And A's world origin remains (50, 60)
    And B's world origin remains (120, 140)
    And B's lastOpId remains "op-other-b"
    And A's lastOpId remains "op-move-ab"
    And 0 nodes were moved or removed by this undo
    And the undo ring no longer contains the entry with forwardOpId "op-move-ab"
    And the undo ring depth is 0
    And the redo stack depth remains 0
    And 0 doc_change ops are queued for this undo
    And 0 restore_snapshot ops are queued for publication
    And 0 error UI is shown

  @SRS-EP-13
  Scenario: Empty undo ring is a no-op
    Given an empty undo ring
    And an empty redo stack
    And SmartGroup B at world origin (100, 200)
    When undo runs
    Then B remains at world origin (100, 200)
    And the undo ring depth remains 0
    And the redo stack depth remains 0
    And 0 doc_change ops are queued for this undo
    And 0 restore_snapshot ops are queued for publication
    And 0 error UI is shown

  @SRS-EP-13
  Scenario: Pure no-op undo consumes the entry and does not push redo
    Given SmartGroup A is absent from the tree
    And SmartGroup B is absent from the tree
    And the undo ring depth is 1
    And that newest entry is a multi-node set_smart_transform with forwardOpId "op-move-ab"
    And that entry's targets are A and B, each with prevLastOpId captured before "op-move-ab"
    And that entry's inverses restore A to world origin (10, 20) and B to world origin (80, 90)
    And the redo stack depth is 0
    When undo runs
    Then A remains absent
    And B remains absent
    And 0 extra nodes were inserted
    And the undo ring no longer contains the entry with forwardOpId "op-move-ab"
    And the undo ring depth is 0
    And the redo stack depth remains 0
    And 0 doc_change ops are queued for this undo
    And 0 restore_snapshot ops are queued for publication
    And 0 error UI is shown
