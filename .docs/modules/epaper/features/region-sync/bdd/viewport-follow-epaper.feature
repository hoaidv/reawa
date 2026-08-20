@SRS-EP-49
Feature: Epaper follow Infini toggle exclusion and disconnect
  As an RM2 creator on Epaper
  I need an icon toggle to opt in to Infini's drawing region
  So that cameras stay independent by default and follow is exactly one direction

  # STORY-EP-055 — SRS-EP-49 / SRS-EP-51. UI-EP-07 states
  # follow.off, follow.following_infini, follow.peer_following_you,
  # follow.connection_lost, follow.reconnect_stays_off.
  # Mutually exclusive one-way follow (ADR-0029). Icon toggle not ToolChip.
  # Two-finger follower local-nav is in hand-touch-two-finger.feature.

  @SRS-EP-49 @SRS-EP-50 @SRS-EP-51
  Scenario: Enabling Epaper follow from both-off applies Infini viewport
    Given a live session
    And follow.direction is none
    And exclusive tool pen is armed
    And region FollowToggle is a sibling of region ToolChip
    And btn.viewport_follow is an icon toggle not a ToolChip exclusive tool
    When the creator taps btn.viewport_follow
    Then follow.direction is infini_to_epaper
    And Infini follow stays off
    And 0 intervals have both follows on
    And Epaper applies Infini's current viewport with p95 map apply at most 100 ms
    And exclusive tool remains pen
    And region FollowToggle is not inside region ToolChip
    And ToolChip still has three exclusive tools
    And btn.viewport_follow has aria-pressed true
    And Epaper emits viewport_follow
    And 0 doc_load, doc_change, or doc_snapshot messages are emitted from that toggle

  @SRS-EP-49 @SRS-EP-50 @SRS-EP-51
  Scenario: Tapping Epaper follow while Infini is following takes over
    Given a live session
    And follow.direction is epaper_to_infini
    And region FollowToggle shows btn.viewport_follow with aria-pressed false
    When the creator taps btn.viewport_follow
    Then follow.direction is infini_to_epaper
    And Infini follow turns off with p95 at most 300 ms
    And 0 intervals have both follows on
    And Epaper begins applying Infini viewport
    And btn.viewport_follow has aria-pressed true

  @SRS-EP-49 @SRS-EP-50 @SRS-EP-51
  Scenario: Connection lost while following forces follow off
    Given follow.direction is infini_to_epaper
    When the session to the desktop is lost
    Then follow.direction becomes none before the next gesture
    And btn.viewport_follow is unavailable with aria-disabled true
    And btn.viewport_follow has aria-pressed false
    And 0 follow-on states persist

  @SRS-EP-49 @SRS-EP-50 @SRS-EP-51
  Scenario: Reconnect does not restore Epaper follow
    Given the session was lost while follow.direction was infini_to_epaper
    And follow.direction is none
    When the session reconnects
    Then follow.direction remains none
    And btn.viewport_follow is off with aria-pressed false
    And the toggle is tappable
    And 0 follow restore occurs until the creator taps btn.viewport_follow

  @SRS-EP-49 @SRS-EP-51
  Scenario: Local pan while following Infini turns Epaper follow off
    Given follow.direction is infini_to_epaper
    And exclusive tool pen is armed
    And the creator finger-downs on empty canvas with hit.kind empty
    When Euclidean panel travel goes to 36 mm (past 10 mm / 89 du at 226 dpi)
    Then follow.direction becomes none before the local pan applies
    And the gesture drives the local camera
    And 0 continued Infini viewport apply occurs after that gesture starts
