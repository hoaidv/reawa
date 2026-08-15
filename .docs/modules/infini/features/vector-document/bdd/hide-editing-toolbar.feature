@SRS-IN-14
Feature: Infini desktop has no editing toolbar
  As a creator reviewing the canvas on desktop
  I need Infini to show no ink-box or selection tools
  So that authoring stays on the tablet this campaign

  # STORY-IN-031 — REQ-04 deprecated; remove leftover STORY-IN-013 / IN-024 chrome.

  @SRS-IN-14
  Scenario: Desktop offers 0 authoring affordances
    Given the Infini desktop window is showing a document
    When the creator looks for an ink-box or selection tool
    Then region ToolStrip is absent
    And region SelectionOverlay is absent
    And 0 transform handles are offered
    And pan zoom chrome still works
    And open or save chrome is unchanged if present
