@SRS-IN-20 @SRS-IN-21 @SRS-IN-22
Feature: Infini applies tablet viewport only while following
  As the creator on Infini desktop
  I apply inbound tablet viewport only while Infini follow is on
  So that cameras stay independent unless I opt in, and my local pan does not drive Epaper

  # STORY-IN-033 — SRS-IN-20 / SRS-IN-21 / SRS-IN-22. Gate SRS-IN-26 (enum only).
  # ADR-0029: apply only while epaper_to_infini; 0 apply while none or infini_to_epaper.
  # Toggle chrome is ../tablet-sync/bdd/viewport-follow-infini.feature (STORY-IN-037).
  # Peer STORY-EP-039 publishes viewport up only if Infini follow is on; this file is Infini apply.
  # Last-writer ADR-0023 is withdrawn — do not steal a token.

  @SRS-IN-20 @SRS-IN-21 @SRS-IN-22 @SRS-IN-26
  Scenario: Tablet two-finger pan while Infini following matches after settle
    Given a live session between Infini and Epaper
    And follow.direction is epaper_to_infini
    And Infini WorldLayer is at translate (0, 0) scale 1
    When the tablet two-finger pans and Infini receives viewport { source: "epaper", translate: (-160, 72), scale: 1, drawingRegion: { x: -160, y: 72, w: 1404, h: 1872 }, orientation: "gutToLeft", settle: true, seq: 14 }
    Then Infini WorldLayer matches translate (-160, 72) scale 1
    And Infini drawingRegion equals { x: -160, y: 72, w: 1404, h: 1872 }
    And 0 divergent viewports remain after settle
    And Infini emits 0 competing viewport down
    And Infini emits 0 doc_load, 0 doc_change, and 0 doc_snapshot from that apply
    And follow.direction stays epaper_to_infini

  @SRS-IN-20 @SRS-IN-21 @SRS-IN-22 @SRS-IN-26
  Scenario: Tablet two-finger pinch while Infini following matches uniform scale after settle
    Given a live session between Infini and Epaper
    And follow.direction is epaper_to_infini
    And Infini WorldLayer is at translate (-160, 72) scale 1
    When the tablet two-finger pinches and Infini receives viewport { source: "epaper", translate: (-160, 72), scale: 1.25, drawingRegion: { x: -160, y: 72, w: 1123.2, h: 1497.6 }, orientation: "gutToLeft", settle: true, seq: 15 }
    Then Infini WorldLayer matches translate (-160, 72) scale 1.25
    And scale remains uniform (scale_x equals scale_y)
    And 0 rotation or skew is applied
    And Infini drawingRegion equals { x: -160, y: 72, w: 1123.2, h: 1497.6 }
    And 0 divergent viewports remain after settle
    And Infini emits 0 competing viewport down
    And follow.direction stays epaper_to_infini

  @SRS-IN-20 @SRS-IN-21 @SRS-IN-22 @SRS-IN-26
  Scenario: Follow off leaves Infini camera unchanged when the tablet pans
    Given a live session between Infini and Epaper
    And follow.direction is none
    And Infini WorldLayer is at translate (12, -8) scale 1.1
    When the creator two-finger pans the tablet to translate (-200, 90) uniform scale 0.8
    Then Infini WorldLayer stays at translate (12, -8) scale 1.1
    And Infini applies 0 inbound tablet viewport
    And Infini emits 0 viewport either way
    And follow.direction stays none
    And a late inbound viewport { source: "epaper", translate: (-200, 90), scale: 0.8, settle: true, seq: 40 } is logged and applied 0 times
    And Infini does not treat that arrival as implicit follow-on

  @SRS-IN-20 @SRS-IN-21 @SRS-IN-22 @SRS-IN-26
  Scenario: Infini as leader ignores inbound tablet viewport
    Given a live session between Infini and Epaper
    And follow.direction is infini_to_epaper
    And Infini WorldLayer is at translate (20, -10) scale 1.2
    When Infini receives viewport { source: "epaper", translate: (-200, 90), scale: 0.8, drawingRegion: { x: -200, y: 90, w: 1404, h: 1872 }, settle: true, seq: 99 }
    Then Infini WorldLayer stays at translate (20, -10) scale 1.2
    And Infini applies 0 inbound tablet viewport
    And Infini logs the ignore
    And follow.direction stays infini_to_epaper
    And Infini does not treat that arrival as implicit follow-on
    And Infini emits 0 doc_load, 0 doc_change, and 0 doc_snapshot from that inbound

  @SRS-IN-20 @SRS-IN-21 @SRS-IN-22 @SRS-IN-26
  Scenario: Infini local pan while following turns follow off and leaves Epaper camera unchanged
    Given a live session between Infini and Epaper
    And follow.direction is epaper_to_infini
    And Infini WorldLayer matches the tablet at translate (-160, 72) scale 1
    And Epaper camera is at translate (-160, 72) scale 1
    When the creator pans Infini locally on CanvasStage by trackpad or mouse by (48, -24)
    Then follow.direction is none before that pan applies
    And Infini WorldLayer stays at the crop the gesture left
    And Infini applies 0 further tablet viewport after that gesture starts
    And Epaper camera stays at translate (-160, 72) scale 1
    And Infini emits 0 viewport down from that local-nav
    And Infini emits 0 doc_load, 0 doc_change, and 0 doc_snapshot from that pan

  @SRS-IN-20 @SRS-IN-21 @SRS-IN-22 @SRS-IN-26
  Scenario: Infini local pinch while following turns follow off and leaves Epaper camera unchanged
    Given a live session between Infini and Epaper
    And follow.direction is epaper_to_infini
    And Infini WorldLayer matches the tablet at translate (-160, 72) scale 1
    And Epaper camera is at translate (-160, 72) scale 1
    When the creator pinches Infini locally on CanvasStage from scale 1 to scale 0.75 about the window center
    Then follow.direction is none before that pinch applies
    And Infini WorldLayer scale is 0.75 uniform (scale_x equals scale_y)
    And Infini WorldLayer stays at the crop the pinch left
    And Infini applies 0 further tablet viewport after that gesture starts
    And Epaper camera stays at translate (-160, 72) scale 1
    And Infini emits 0 viewport down from that local-nav
