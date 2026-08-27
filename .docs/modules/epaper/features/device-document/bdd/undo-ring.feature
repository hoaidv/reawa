@SRS-EP-07 @SRS-EP-09
Feature: Inverse-op undo ring on the device document
  As an RM2 creator
  I need structural ops to be undoable as counterpart inverses when lastOpId matches
  So that a wrong box or a slipped drag costs one undo, without a whole-tree snapshot

  # Re-homed 2026-08-13 from infini SRS-IN-12 (deprecated) — CHL-0008 / ADR-0014 §5.
  # Retagged 2026-08-27 for STORY-EP-059 / ADR-0032 (accepted): exactness is lastOpId
  # match → stored pre-op fields (geometry ±1 world unit), not a whole-tree snapshot.
  # 0 restore_snapshot on undo. Depth 20. One entry per completed gesture.
  # F20/F21 skip-whole / absence-partial catalogue: undo-fail-safe.feature (STORY-EP-060).
  # Device undo/redo queue shape: undo-queue.feature (STORY-EP-061). Do not regress these 12.

  @SRS-EP-09
  Scenario: Structural gesture commit pushes one inverse entry
    Given an empty undo ring on the device document
    When a create_frame gesture commits as opId "op-frame-1" at seq 1 with world origin (0, 0) and size 100 by 80
    Then the undo ring depth is 1
    And that entry is UndoEntry { forwardOpId: "op-frame-1", seq: 1, inverses, targets }
    And that entry's inverses contain exactly 1 op: remove_node of the created frame
    And that entry's targets contain exactly 1 row { nodeId of the created frame, prevLastOpId empty }
    And that entry contains 0 whole-tree snapshots
    And the created frame's lastOpId equals "op-frame-1"

  @SRS-EP-07
  Scenario: One completed gesture is one undo entry
    Given SmartGroup B at world origin (40, 60)
    And a move gesture that rendered 12 intermediate frames of B
    When the gesture is released and committed as opId "op-move-g1" with B at world origin (88, 91)
    Then the undo ring gained exactly 1 entry
    And that entry's inverses store B's absolute pre-op world origin (40, 60)
    And one undo reverts exactly that gesture

  @SRS-EP-07
  Scenario: Undo with matching lastOpId restores stored pre-op fields
    Given SmartGroup B at world origin (100, 200) with size 80 by 60
    And a set_smart_transform commit with opId "op-move-1" moved B to world origin (140, 228)
    And B's lastOpId equals "op-move-1"
    When undo runs
    Then B's world origin is (100, 200) within ±1 world unit
    And 0 nodes diverge from the stored pre-op fields on that entry
    And 0 restore_snapshot ops are queued for publication
    And the queued change op type is set_smart_transform

  @SRS-EP-07
  Scenario: Viewport tool selection clipboard-slot and copy do not push undo
    Given an undo ring with depth 0
    And Ink node I1 is selected
    When the viewport pans 40 world units
    And the tool switches from pen to selection
    And the selection changes from I1 to empty
    And copy places I1 into the clipboard slot
    Then the undo ring depth remains 0
    And the clipboard slot holds 1 node

  @SRS-EP-07
  Scenario: Ring overflow drops the oldest entry
    Given 20 committed create_frame ops on the undo ring with forwardOpIds "op-001" through "op-020"
    When a 21st create_frame commits with forwardOpId "op-021"
    Then the undo ring depth is 20
    And the entry with forwardOpId "op-001" is gone
    And the newest entry's forwardOpId is "op-021"

  @SRS-EP-07
  Scenario: Undo requested mid-gesture is deferred
    Given a set_smart_transform gesture in progress on SmartGroup B that has rendered 4 intermediate frames and has not committed
    And the undo ring depth is 0
    When undo is requested before release
    Then the in-flight gesture is not corrupted: 0 nodes are removed and 0 transforms are committed before release
    And after release the gesture commits as 1 undo entry
    And that undo then applies and restores B's stored pre-op fields within ±1 world unit

  @SRS-EP-07
  Scenario: An accepted document load empties undo and redo
    Given an undo ring with 3 inverse entries
    And a redo stack with 2 entries
    And an empty publish queue
    When a doc_load is accepted
    Then the undo ring depth is 0
    And the redo stack depth is 0
    And undo cannot reach the pre-load tree

  @SRS-EP-09
  Scenario: Bound-node drag does not bump connector lastOpId
    Given SmartGroup G at world origin (10, 20) with lastOpId "op-g-create"
    And connector C bound to G with rest shape S and lastOpId "op-c-create"
    When a drag of G commits set_smart_transform as opId "op-g-move" to world origin (50, 60)
    Then G's lastOpId equals "op-g-move"
    And C's lastOpId remains "op-c-create"
    And C's derived warp polyline V is recomputed from G's new pose
    And the undo ring gained exactly 1 entry whose targets include G

  @SRS-EP-09
  Scenario: Last-live-pose cache does not bump connector lastOpId
    Given connector C with lastOpId "op-c-create" whose bound endpoint is absent
    When the last-live-pose cache on C is written at world (10, 20)
    Then C's lastOpId remains "op-c-create"
    And the undo ring depth is unchanged

  @SRS-EP-07
  Scenario: Ungroup inverse does not delete children
    Given Ink I1 at root sibling index 0 and Ink I2 at root sibling index 1
    And a create_smart_group commit with opId "op-group-1" enclosed I1 and I2 as children of group G
    And G's lastOpId equals "op-group-1"
    When undo runs
    Then I1 is reparented to root at stored index 0
    And I2 is reparented to root at stored index 1
    And G is removed
    And I1 and I2 remain present
    And 0 child nodes were deleted

  @SRS-EP-07
  Scenario: Redo restores the undone forward fields
    Given a set_smart_transform that moved SmartGroup B from world origin (100, 200) to (140, 228) and was then undone
    And B's lastOpId equals "op-b"
    When redo runs
    Then B's world origin is (140, 228) within ±1 world unit
    And 0 restore_snapshot ops are queued for publication
    And a later create_frame commit clears the redo stack to depth 0

  @SRS-EP-07
  Scenario: Empty redo is a no-op
    Given an empty redo stack
    And SmartGroup B at world origin (100, 200)
    When redo runs
    Then B remains at world origin (100, 200)
    And 0 restore_snapshot ops are queued for publication
