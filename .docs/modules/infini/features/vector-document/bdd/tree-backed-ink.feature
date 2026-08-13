@SRS-IN-04
Feature: Mirror-backed ink ingestion
  As Infini holding the document mirror
  I need published device changes to land as nodes in VectorDocument
  So that the desktop paints, saves, and reopens what the creator actually made

  # Revised 2026-08-13 — CHL-0008 / ADR-0014. The desktop no longer ingests live strokes
  # into the tree: strokes are a transient preview, and doc_change carries the truth.
  # Enclose and membership moved to the device (epaper/features/ink-box/bdd/).

  # STORY-IN-012 — SRS-IN-04 (mirror path)

  @SRS-IN-04
  Scenario: Published change becomes Ink in the mirror
    Given an Infini VectorDocument as the session mirror
    And a doc_change carrying append_ink for "stroke_42" with at least 2 world samples
    When the change applies
    Then the materialised tree contains an Ink node for that stroke
    And the Ink samples match the published world points

  @SRS-IN-04
  Scenario: Preview strokes never enter the tree
    Given a live session
    When stroke_begin, stroke_point, and stroke_end arrive for a stroke
    Then a transient preview path renders
    And zero Ink nodes are created from the preview
    When the matching doc_change arrives
    Then the preview is replaced by the real node

  @SRS-IN-04
  Scenario: Mirror ink paints through WorldLayer sync
    Given a VectorDocument with at least one Ink node applied from a change
    When syncFromVectorDoc runs on the WorldLayer document
    Then WorldLayer primitives include a path whose id matches that Ink node
    And flattenDrawables lists that Ink among drawables

  @SRS-IN-04
  Scenario: Mirror remains the paint source after a rebuild
    Given Ink nodes exist in VectorDocument from applied changes
    When a rebuild paints the canvas from the tree (plus in-flight previews only)
    Then every applied Ink remains addressable in the tree by id
    And paint does not replace the tree with WorldLayer-only storage for those strokes

  @SRS-IN-04
  Scenario: Duplicate change is idempotent
    Given a doc_change with opId "op_7" already applied
    When the same opId applies again
    Then the tree is unchanged

  @SRS-IN-04
  Scenario: Device-authored Smart Groups round-trip through save and reopen
    Given a mirror built entirely from device-published changes
    When the document is saved and reopened
    Then bounds, transform, inkScaleMode, child roles, and layoutOffset values match
    And the reopened document is legal to send as a doc_load
