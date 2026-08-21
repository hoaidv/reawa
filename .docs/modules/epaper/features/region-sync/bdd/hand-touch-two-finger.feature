@SRS-EP-24
Feature: Two-finger local pan pinch and viewport publish
  As an RM2 creator on Epaper
  I need two fingers to pan and pinch the tablet camera locally
  So that the next pen sample maps correctly and Infini follows only when Infini follow is on

  # STORY-EP-039 — SRS-EP-24 / SRS-EP-26. UI-EP-06 states
  # hand.two_finger_pan, hand.pinch, hand.pan_vs_move, hand.link_down_local_view.
  # Publish only if Infini follow is on (ADR-0029). Infini apply is STORY-IN-033.

  @SRS-EP-24 @SRS-EP-26
  Scenario: Two-finger pan for 5 s applies the local map before the next pen sample
    Given two fingers on empty canvas
    And 0 box-move or resize in flight
    And follow.direction is none
    When the creator pans for 5 s or longer
    Then drawingRegion translates locally
    And the next pen sample uses the new local map with p95 map apply at most 100 ms
    And Infini's camera is unchanged

  @SRS-EP-24 @SRS-EP-26
  Scenario: Two-finger pinch scales uniformly with the same map-apply bar
    Given two fingers on empty canvas
    And 0 box-move or resize in flight
    And follow.direction is none
    When the creator pinches
    Then scale changes uniformly (scale_x equals scale_y)
    And 0 rotation or skew is applied
    And the next pen sample uses the new local map with p95 map apply at most 100 ms
    And Infini's camera is unchanged

  @SRS-EP-24 @SRS-EP-26
  Scenario: Infini follow off leaves Infini's camera unchanged
    Given follow.direction is none
    When the creator two-finger pans on the tablet
    Then Infini's camera is unchanged
    And Epaper emits 0 viewport up messages

  @SRS-EP-24 @SRS-EP-26
  Scenario: Infini follow on publishes the local viewport from the tablet
    Given follow.direction is epaper_to_infini
    When the creator two-finger pans on the tablet
    Then Epaper publishes viewport up with source epaper
    And drawingRegion in that message equals the local region after the pan
    And settle is true when the two-finger gesture ends
    And 0 viewport down is emitted from that gesture

  @SRS-EP-24
  Scenario: Second finger does not start pan while box-move is in flight
    Given exclusive tool sel_freeform is armed
    And a one-finger box-move is in flight on SmartGroup "sg_1"
    When a second finger lands
    Then two-finger pan does not start
    And 0 viewport pan occurs until the box-move ends
    And the in-flight move continues

  @SRS-EP-24
  Scenario: Second finger does not start pan while resize is in flight
    Given exclusive tool sel_freeform is armed
    And a one-finger resize is in flight on a knob of SmartGroup "sg_1"
    When a second finger lands
    Then two-finger pan does not start
    And 0 viewport pan occurs until the resize ends
    And the in-flight resize continues

  @SRS-EP-24 @SRS-EP-26
  Scenario: Link down still changes the local viewport
    Given the session to the desktop is down
    And follow.direction is none
    When the creator two-finger pans
    Then the local viewport still changes
    And region ToolChip shows publish queued
    And 0 viewport up is published

  @SRS-EP-24 @SRS-EP-26
  Scenario: Two-finger pan while following Infini is ignored
    Given follow.direction is infini_to_epaper
    When the creator two-finger pans on Epaper
    Then follow.direction stays infini_to_epaper
    And 0 local pan or pinch applies
    And inbound Infini viewport still applies
