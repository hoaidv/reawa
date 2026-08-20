@SRS-IN-26 @SRS-IN-28
Feature: Infini viewport-follow Epaper
  As the creator on Infini desktop
  I opt in to matching the connected tablet drawing region with an icon toggle
  So that cameras stay independent by default and follow is one-way exclusive

  # STORY-IN-037 — SRS-IN-26 / SRS-IN-27 / SRS-IN-28. ADR-0029 mutually exclusive
  # one-way follow. Last-writer ADR-0023 is not the product model.
  # Apply-while-following depth is STORY-IN-033 —
  # ../infinity-canvas/bdd/viewport-follow-apply.feature
  # This file only asserts apply as a Then while follow is on.

  @SRS-IN-26 @SRS-IN-28 @SRS-IN-27
  Scenario: Creator turns Infini follow on from both-off
    Given a live session between Infini and Epaper
    And follow.direction is none
    And Infini FollowToggle btn.viewport_follow is in WindowFrame with aria-pressed false and caption "Follow off"
    And btn.viewport_follow is a desktop icon toggle not a ToolChip and not a child of WorldLayer
    And the tablet viewport is translate (-80, 40) uniform scale 0.87
    And Infini WorldLayer is at translate (0, 0) scale 1
    When the creator clicks btn.viewport_follow
    Then Infini emits viewport_follow { direction: "epaper_to_infini" } on TCP 9877
    And follow.direction is epaper_to_infini
    And Epaper follow stays off
    And Infini applies the tablet viewport after settle with 0 divergent viewports
    And Infini WorldLayer matches translate (-80, 40) scale 0.87 within p95 300 ms
    And FollowToggle is follow.following_epaper with aria-pressed true and caption "Following Epaper"
    And Infini emits 0 doc_load, 0 doc_change, and 0 doc_snapshot from that toggle

  @SRS-IN-26 @SRS-IN-28 @SRS-IN-27
  Scenario: Creator tap on Infini follow takes over from Epaper follow
    Given a live session and Epaper follow is on
    And follow.direction is infini_to_epaper
    And Infini FollowToggle is follow.peer_following_you with aria-pressed false and caption "Epaper is following you"
    And btn.viewport_follow is still tappable
    When the creator clicks btn.viewport_follow
    Then Infini emits viewport_follow { direction: "epaper_to_infini" } on TCP 9877
    And follow.direction is epaper_to_infini
    And Epaper follow turns off within p95 300 ms
    And Infini follow is on
    And 0 intervals exist where both follows are on
    And FollowToggle is follow.following_epaper with aria-pressed true
    And Infini applies the tablet viewport after settle

  @SRS-IN-26 @SRS-IN-28 @SRS-IN-27
  Scenario: Connection lost forces Infini follow off
    Given a live session and Infini is following
    And follow.direction is epaper_to_infini
    When the session drops
    Then follow.direction is none before the next gesture
    And FollowToggle is follow.connection_lost
    And btn.viewport_follow is unavailable with aria-pressed false and caption "No session — follow off"
    And Infini does not apply inbound tablet viewport
    And Infini emits 0 doc_load, 0 doc_change, and 0 doc_snapshot from the drop

  @SRS-IN-26 @SRS-IN-28 @SRS-IN-27
  Scenario: Reconnect does not restore Infini follow
    Given Infini follow was on then the session dropped
    And follow.direction is none
    When the session returns on reconnect with hello
    Then hello does not carry last follow.direction
    And follow.direction stays none
    And FollowToggle is follow.reconnect_stays_off
    And btn.viewport_follow is enabled with aria-pressed false and caption "Reconnected — follow stays off"
    And Infini does not apply inbound tablet viewport until the creator clicks follow again

  @SRS-IN-26 @SRS-IN-28 @SRS-IN-27
  Scenario: Creator pan on Infini while following turns follow off
    Given a live session and Infini is following
    And follow.direction is epaper_to_infini
    And FollowToggle is follow.following_epaper with aria-pressed true
    When the creator pans Infini locally on CanvasStage by trackpad or mouse by (40, -20)
    Then follow.direction is none before that pan applies
    And Infini WorldLayer stays at the crop the gesture left
    And Infini applies 0 further tablet viewport after that gesture starts
    And FollowToggle is follow.local_nav_turns_off with aria-pressed false and caption "Local pan turned follow off"

  @SRS-IN-26 @SRS-IN-28 @SRS-IN-27
  Scenario: No session leaves the Infini follow toggle unavailable
    Given Infini has no live session
    When the creator looks at FollowToggle
    Then FollowToggle is follow.connection_lost
    And btn.viewport_follow is unavailable with aria-pressed false
    And 0 follow-on states persist
    And follow.direction is none
