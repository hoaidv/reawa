@SRS-IN-01
Feature: Canvas transform and primitive figures
  As a creator
  I need world-space primitives to render under translate + uniform scale
  So that circles stay circular and empty/populated states both work

  # STORY-IN-003 — Spec states canvas.empty / canvas.populated

  @SRS-IN-01
  Scenario: Screen positions match translate then uniform scale
    Given world-space primitives exist including line, rect, ellipse, and path
    And viewport translate is (tx, ty) and uniform scale s where s > 0
    When the canvas renderer draws those primitives
    Then each point maps as screen = (world + translate) * scale
    And scale_x equals scale_y (no non-uniform scale)

  @SRS-IN-01
  Scenario: Circle remains circular under pan and zoom
    Given a circle primitive centered in world space with equal radius axes
    When translate and scale are changed programmatically (simulating pan and zoom)
    Then the rendered circle's bounding width equals its bounding height within 1 CSS px at the current zoom
    And aspect ratio remains 1:1

  @SRS-IN-01
  Scenario: Empty and populated canvas states both render
    Given document.vectors.length is 0
    When the canvas paints state canvas.empty
    Then the CanvasStage and WorldLayer regions render without error
    And the empty hint copy from the Spec may be shown
    Given document.vectors.length is at least 1
    When the canvas paints state canvas.populated
    Then the primitives are visible under WorldLayer without crash
