@SRS-EP-17
Feature: On-device connector recognition
  As an RM2 creator drawing between two Smart Groups
  I need an open stroke to become a connector when guards pass
  So that the line stays my ink and is attached without a fifth tool

  # STORY-EP-030 — SRS-EP-17 / SRS-EP-19. Chrome matches UI-EP-05.
  # Constants named in SRS: MIN_CONNECTOR_WORLD 48 u; body ≤20% in A, ≤20% in B, ≥60% outside.
  # R_SNAP and R_JOIN stay named; do not invent numeric values here.

  @SRS-EP-17
  Scenario: UX1 open stroke A to C commits create_connector
    Given two distinct Smart Groups A and C
    And exclusive tool pen and tgl.recog.connector are armed at pen-down
    And an open stroke whose first sample is within R_SNAP of A bounds and last within R_SNAP of C bounds
    And the stroke arc is at least MIN_CONNECTOR_WORLD 48 world units
    And at most 20 percent of samples lie in A, at most 20 percent in C, and at least 60 percent lie outside every box
    When pen-up runs connector recognition
    Then create_connector commits with body, rest shape, both anchors, and warpStyle
    And the connector is visible on the panel within p95 500 ms
    And zero peer messages were required to create it
    And one undo restores the pre-create ink tree
    And [recog] outcome equals connector

  @SRS-EP-17
  Scenario: UX2 chained strokes merge into one connector and one undo
    Given two distinct Smart Groups A and C
    And tgl.recog.connector is armed
    When three successive free top-level inks join by R_JOIN and tangent continuity
    And the last stroke lands on C while the far end binds A
    Then exactly one create_connector exists
    And body children are those strokes in draw order
    And warpStyle is picked from the merged rest spine S: at most one inflection yields cubic else morph
    And exactly one undo entry restores the chain

  @SRS-EP-17
  Scenario: Failed guards or disarmed toggle leave ordinary ink with no banner
    Given two Smart Groups
    And either tgl.recog.connector is disarmed at pen-down or a connector guard fails
    When pen-up runs
    Then the stroke stays ordinary ink
    And region ConnBlink is absent
    And no banner or toast is shown
    And state conn.rejected has no copy

  @SRS-EP-19
  Scenario: Successful create plays ovl.conn_blink once without naming style
    Given a successful create_connector
    When chrome runs
    Then ovl.conn_blink inverts the connector and both bound nodes once
    And the pulse is one Mono refresh about 250 ms then off
    And Ink and Curve labels are absent during the blink
    And ToolChip exclusive tools remain three with no connector tool
    And only a partial region refreshes
