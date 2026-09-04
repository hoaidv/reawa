@SRS-EP-35 @SRS-EP-74 @SRS-EP-37
Feature: Path B endpoint ink
  As an RM2 creator drawing ticks on a connector end
  I need those strokes to become decoration of that end
  So that arrows stay aimed at the box and I can erase the ticks without deleting the connector

  # STORY-EP-047 — SRS-EP-35 / SRS-EP-74 / SRS-EP-37. ADR-0038 face frame.
  # Steal: ≥80% arc length in eraseMmToWorld(5) circle at one end of one connector.
  # Armed with recog.connector latched at pen-down.

  @SRS-EP-35 @SRS-EP-74
  Scenario: Stroke on one end binds as endpoint ink
    Given Connector recognition is armed at pen-down
    And an existing connector between two Smart Groups
    When a stroke has at least 80 percent of its length in a 5 mm world circle at one end of that connector
    Then set_endpoint_ink appends a face-frame stroke on that ConnectorAnchor
    And the free Ink is removed
    And zero second connector is created
    And the decoration paints as drawn
    And [recog] outcome equals endpoint_ink

  @SRS-EP-35 @SRS-EP-37
  Scenario: Bound-node transform keeps stored leave and rotates paint with re-warp
    Given endpoint decoration on a connector end
    When the bound Smart Group rotates, or the peer node moves so the spine re-warps
    Then stored {n, e} on ConnectorAnchor.styleInk is unchanged
    And drawnN / drawnE / drawnBoxX / drawnBoxY are unchanged
    And world paint is the face-frame stroke rotated by α (stored leave vs re-warped leaving tangent)
    And the rest spine is not rebaked

  @SRS-EP-35 @SRS-EP-74
  Scenario: Second stolen stroke appends and one undo peels it
    Given endpoint decoration on an end
    When another stolen stroke commits on the same end
    Then the styleInk list grows by one
    And one undo removes only that last stroke
    And the earlier decoration remains

  @SRS-EP-35
  Scenario: Endpoint-ink beats draw-into membership
    Given Connector recognition is armed at pen-down
    And an existing connector between two Smart Groups
    And a stroke that has at least 80 percent of its length in a 5 mm circle at one end
    And that stroke also has at least 80 percent of its length inside the bound box's boundary ink
    When pen-up runs dispatch
    Then the stroke is bound as endpoint ink
    And it is not reparented as Smart Group content

  @SRS-EP-35
  Scenario: Spine, empty canvas, or recognition off is not stolen
    Given an existing connector
    When a stroke ends on the connector spine, on empty canvas, or with Connector recognition off
    Then the stroke is not bound as endpoint style
    And [recog] outcome is not endpoint_ink

  @SRS-EP-35 @SRS-EP-37
  Scenario: Brush erase clips ticks and keeps the connector
    Given endpoint decoration on a connector
    When the creator brush-erases those ticks
    Then styleInk is clipped in world
    And the connector spine remains

  @SRS-EP-35
  Scenario: Object-erase of ticks drops a stroke and keeps the connector
    Given endpoint decoration on a connector
    When object-erase covers at least 80 percent of a decoration stroke and not 80 percent of the spine
    Then that decoration stroke is dropped
    And the connector remains

  @SRS-EP-35
  Scenario: Object-erase of the connector takes decoration with it
    Given endpoint decoration on a connector
    When object-erase removes the connector
    Then the decoration is gone with it
    And one undo restores both
