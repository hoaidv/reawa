@SRS-IN-04
Feature: Document tree model and idempotent ops
  As Infini session SoT
  I need a materialised tree that applies ops once and flattens for paint
  So that peers and WorldLayer share one coherent document

  # STORY-IN-007 — SRS-IN-04

  @SRS-IN-04
  Scenario: Tree invariants hold after structure ops
    Given an empty Infini document
    When ops create Frame, Ink, Text, Primitive, Group, and Connector per SRS-IN-04
    Then every node id in the materialised tree is unique
    And Frame nodes appear only in Document.rootChildren
    And Group children exclude Frame
    And connector from.nodeId and to.nodeId resolve or the connector is marked invalid

  @SRS-IN-04
  Scenario: Op apply is idempotent by opId
    Given a valid op with opId "op_100"
    And the op has been applied once to the tree
    When the same opId "op_100" is applied a second time
    Then the materialised tree is byte-for-byte unchanged from after the first apply

  @SRS-IN-04
  Scenario: Connector anchors re-resolve when endpoint moves
    Given a connector with from.port "east" on rect "rect_1"
    And to.boundary on ellipse "ell_1" with angle 0.785
    When node "rect_1" is moved by translate (50, 0)
    Then the connector from world point equals the live east midpoint of "rect_1"
    And the connector to world point equals the live boundary of "ell_1" at that angle

  @SRS-IN-04
  Scenario: flattenDrawables projects leaves for WorldLayer
    Given a materialised tree with at least one Ink under a Frame
    And optionally a SmartGroup with a non-identity transform
    When flattenDrawables runs
    Then the drawable list includes Ink (and other leaves) with SmartGroup transforms applied when present
    And WorldLayer can cull and paint those drawables without crash
