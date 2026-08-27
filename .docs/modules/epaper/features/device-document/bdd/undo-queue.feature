@SRS-EP-08 @SRS-EP-07
Feature: Device undo queue records counterpart compound not restore snapshot
  As the device that owns the working document
  I need undo and redo to enqueue the counterpart op or a compound of counterparts
  So that history never publishes a whole-tree restore_snapshot

  # STORY-EP-061 / ADR-0032 §4. Device outbound queue only.
  # Matching lastOpId apply already asserts 0 restore_snapshot in undo-ring.feature;
  # this file owns the queue shape: counterpart, compound, Path A set_ink_samples.
  # Infini is not required to apply these ops this iter (STORY-IN-038 cancelled).
  # 0 Infini apply / unknown-desktop-op scenarios.

  @SRS-EP-08
  Scenario: Matching undo records the counterpart op
    Given SmartGroup B at world origin (140, 228) with lastOpId "op-move-1"
    And a committed set_smart_transform with opId "op-move-1" that moved B from world origin (100, 200)
    And the undo ring's newest entry has forwardOpId "op-move-1"
    When undo runs
    Then B's world origin is (100, 200) within ±1 world unit
    And the device outbound queue gained exactly 1 doc_change for this undo
    And that change's op type is set_smart_transform
    And 0 restore_snapshot ops are queued for publication

  @SRS-EP-08
  Scenario: Matching redo records the counterpart op
    Given a set_smart_transform that moved SmartGroup B from world origin (100, 200) to (140, 228) and was then undone
    And B's lastOpId equals "op-b"
    And the redo stack depth is 1
    When redo runs
    Then B's world origin is (140, 228) within ±1 world unit
    And the device outbound queue gained exactly 1 doc_change for this redo
    And that change's op type is set_smart_transform
    And 0 restore_snapshot ops are queued for publication

  @SRS-EP-08
  Scenario: Multi-inverse undo records one compound not N changes
    Given SmartGroup A at world origin (50, 60) with lastOpId "op-move-ab"
    And SmartGroup B at world origin (120, 140) with lastOpId "op-move-ab"
    And a committed multi-node set_smart_transform with opId "op-move-ab" that moved A from (10, 20) and B from (80, 90)
    And the undo ring's newest entry has forwardOpId "op-move-ab" and 2 inverses
    When undo runs
    Then A's world origin is (10, 20) within ±1 world unit
    And B's world origin is (80, 90) within ±1 world unit
    And the device outbound queue gained exactly 1 doc_change for this undo
    And that change's op type is compound
    And that compound contains 2 counterpart ops
    And 0 restore_snapshot ops are queued for publication

  @SRS-EP-08
  Scenario: Path A stroke-erase commit and undo use set_ink_samples
    Given Ink I1 with 4 samples at world (0, 0), (10, 0), (20, 0), (30, 0) and lastOpId "op-ink-1"
    When a Path A eraser-nib gesture intersects samples at (20, 0) and (30, 0) and commits as opId "op-erase-1"
    Then I1 remains present with 2 samples at world (0, 0) and (10, 0) within ±1 world unit
    And I1's lastOpId equals "op-erase-1"
    And the queued change op type is set_ink_samples
    And 0 remove_node ops were queued for this gesture
    And 0 restore_snapshot ops are queued for publication
    When undo runs
    Then I1 has 4 samples matching the stored pre-erase samples within ±1 world unit
    And the device outbound queue gained exactly 1 doc_change for this undo
    And that undo change's op type is set_ink_samples
    And 0 restore_snapshot ops are queued for publication

  @SRS-EP-08
  Scenario: Path A emptying erase uses set_ink_samples and remove_node
    Given Ink I1 with 2 samples at world (0, 0) and (10, 0) and lastOpId "op-ink-1"
    And both samples lie inside the eraser nib footprint
    When a Path A eraser-nib gesture commits as opId "op-erase-empty-1"
    Then I1 is absent
    And the queued change op type is compound
    And that compound contains set_ink_samples
    And that compound contains remove_node
    And 0 restore_snapshot ops are queued for publication
    When undo runs
    Then I1 is present with 2 samples at world (0, 0) and (10, 0) within ±1 world unit
    And the device outbound queue gained exactly 1 doc_change for this undo
    And that undo change is a counterpart or a compound of counterparts
    And that undo change's op type is not restore_snapshot
    And 0 restore_snapshot ops are queued for publication
