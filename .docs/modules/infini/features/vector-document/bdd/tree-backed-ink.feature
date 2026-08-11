@SRS-IN-04
Feature: Tree-backed live ink ingestion
  As Infini session SoT
  I need Epaper/live strokes to land as Ink nodes in VectorDocument
  So that Smart Group enclose and membership can see ink in the tree

  # STORY-IN-012 — SRS-IN-04 (live path)

  @SRS-IN-04
  Scenario: Committed stroke becomes Ink in the tree
    Given an Infini VectorDocument as session SoT
    And a live stroke with id "stroke_42" and at least 2 world samples
    When the stroke commits via the tree-backed ingest path
    Then the materialised tree contains an Ink node for that stroke
    And the Ink samples match the committed world points

  @SRS-IN-04
  Scenario: Tree ink paints through WorldLayer sync
    Given a VectorDocument with at least one Ink node from tree-backed ingest
    When syncFromVectorDoc runs on the WorldLayer document
    Then WorldLayer primitives include a path whose id matches that Ink node
    And flattenDrawables lists that Ink among drawables

  @SRS-IN-04
  Scenario: Tree remains SoT after refresh-style rebuild
    Given tree-backed Ink nodes exist in VectorDocument
    When a rebuild paints the canvas from the tree (plus optional in-flight live strokes only)
    Then every committed Ink remains addressable in the tree by id
    And paint does not replace the tree with WorldLayer-only storage for those strokes

  @SRS-IN-04
  Scenario: Smart Group prerequisites can see tree ink
    Given tree-backed Ink nodes from committed strokes
    When a Smart Group story queries Ink nodes in the tree
    Then at least one Ink node is present for enclose or membership to operate on
