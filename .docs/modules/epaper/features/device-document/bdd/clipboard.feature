@SRS-EP-31 @SRS-EP-32 @SRS-EP-33
Feature: In-document clipboard on the device
  As an RM2 creator
  I need copy cut and paste on the tablet without the OS pasteboard
  So that a cluster that is already right can be duplicated or moved as a copy

  @SRS-EP-31 @SRS-EP-07
  Scenario: Copy does not push undo
    Given SelectionMode and a non-empty selection of ink I1
    When copy runs
    Then the document still contains I1
    And the clipboard slot is non-empty
    And the undo ring depth is unchanged

  @SRS-EP-31
  Scenario: Copy of a tap-selected ink-box keeps children
    Given a SmartGroup G with child ink C
    And G is the only selected id
    When copy runs
    Then the clipboard slot holds G
    And G's clone still contains C

  @SRS-EP-31
  Scenario: Cut removes roots and leaves empty groups
    Given a SmartGroup G with one selected child C
    When cut runs
    Then C is gone
    And G remains in the tree
    And the clipboard slot holds C
    And undo depth increased by 1

  @SRS-EP-31 @SRS-EP-33
  Scenario: Paste lands union AABB top-left on the tap
    Given a non-empty clipboard slot whose union AABB is 40 by 30
    And a tap world point (100, 200)
    When paste commits
    Then new ids exist
    And the paste union AABB top-left is (100, 200) within ±1 world unit
    And the slot is unchanged
    And undo depth increased by 1
    And 0 doc_change entries were enqueued

  @SRS-EP-31
  Scenario: Paste free ink into an ink-box
    Given free ink in the clipboard slot
    And a SmartGroup G that is not a live source
    And a tap on G
    When paste commits
    Then the copy's parent is G

  @SRS-EP-31
  Scenario: Empty slot paste is a no-op
    Given an empty clipboard slot
    When paste is invoked
    Then 0 nodes change
    And undo depth is unchanged

  @SRS-EP-32
  Scenario: Empty tap while selected only deselects
    Given a non-empty selection and a non-empty clipboard slot
    When the creator taps empty canvas
    Then selection is empty
    And paste is not on the toolbar

  @SRS-EP-32
  Scenario: Empty clipboard tap shows no paste chrome
    Given an empty clipboard slot
    And SelectionMode
    When the creator taps empty canvas
    Then selection is empty
    And paste is not on the toolbar

  @SRS-EP-32
  Scenario: Freeform selection has no paste button
    Given a non-empty clipboard slot
    And a freeform selection with no tap location
    Then paste is not on the toolbar

  @SRS-EP-32
  Scenario: Paste chrome is hidden during a move from a selected node
    Given a non-empty clipboard slot
    And a selected node with a tap location
    When a move drag is in progress
    Then paste is not on the toolbar

  @SRS-EP-32
  Scenario: Paste chrome is hidden during a move from empty-tap origin
    Given a non-empty clipboard slot
    And an empty-canvas tap location
    And no selection
    When a move drag of a node is in progress
    Then paste is not on the toolbar

  @SRS-EP-31
  Scenario: Paste onto live originals is refused
    Given a copy of node S still in the tree
    When the creator taps S and taps paste
    Then 0 nodes change
    And a refuse reason is shown
    And the clipboard slot is unchanged

  @SRS-EP-31
  Scenario: Cut then paste undo is two counterpart entries
    Given a non-empty selection
    When the creator cuts then pastes
    Then one undo removes the copies and originals stay gone
    And a second undo restores the originals
    And 0 restore_snapshot ops run

  @SRS-EP-33 @SRS-EP-11
  Scenario: Tap does not nudge
    Given SelectionMode and a node at world origin (40, 60)
    When pointer-down on that node travels ≤1 mm and lifts
    Then the node is selected
    And the node's world origin is still (40, 60) within ±1 world unit
