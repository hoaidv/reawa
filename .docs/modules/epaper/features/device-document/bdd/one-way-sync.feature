@SRS-EP-08
Feature: One-way document sync from the device
  As the device that owns the working document
  I need exactly one document input and an ordered outbound change stream
  So that no peer can overwrite the creator's work mid-session

  # STORY-EP-020 — one scenario per AC. Wire ids and op.type are the SRS-IN-09
  # transmit set (ADR-0015 closed ids). Do not invent a third alias (CHL-0009).
  # Handshake: hello, drain_ack, doc_change, queue_empty, doc_load, load_ack.
  # Change envelope: doc_change { seq, opId, op, baseSeq } with
  # op { opId, type, payload } — type in append_ink | create_smart_group |
  # set_smart_transform | set_ink_scale_mode | reparent | remove_node |
  # restore_snapshot. Preview: stroke_begin | stroke_point | stroke_end
  # (no stroke_begin.intent). Retired inbound: doc_snapshot, pickables, tool_intent.

  @SRS-EP-08
  Scenario: After the initial load, only viewport comes down
    Given a session that has completed its initial load
    When the whole session lifetime is traced
    Then 0 inbound document-bearing messages are observed
    And viewport messages continue to apply

  @SRS-EP-08
  Scenario: Unsolicited document inbound is rejected
    Given a live session with a local document
    When an unsolicited doc_load, doc_snapshot, pickables, or tool_intent arrives
    Then it is not applied
    And it is logged
    And the local document is unchanged

  @SRS-EP-08
  Scenario: Load handshake drains the queue then starts a new epoch
    Given 5 queued changes
    When the desktop offers a doc_load
    Then all 5 publish as doc_change in seq order
    And queue_empty is sent
    And then the doc_load is accepted
    And 0 queued changes are discarded
    And seq resets to 0
    And the undo ring is cleared
    And the selection is cleared
    And load_ack is sent

  @SRS-EP-08
  Scenario: Editing continues with the session down
    Given the session is down
    When the creator performs 10 document operations
    Then all 10 apply locally
    And all 10 queue in order
    And 0 tools were unavailable

  @SRS-EP-08
  Scenario: Reconnect publishes the queue; duplicate opId is a no-op
    Given a restored link and queued ops
    When drain_ack arrives
    Then queued ops publish as doc_change in seq order
    And 0 are lost
    And 0 are reordered
    And a duplicate opId apply is a no-op

  @SRS-EP-08
  Scenario: One doc_change per committed structural op
    Given a committed structural op whose op.type is an SRS-IN-09 transmit name
    When it publishes
    Then exactly one doc_change {seq, opId, op, baseSeq} is emitted
    And the mirror updates p95 <= 300 ms

  @SRS-EP-08
  Scenario: Preview stroke_* is not a document change
    Given a stroke in progress
    When stroke_begin, stroke_point, and stroke_end stream
    Then no stroke_begin carries intent
    And the node arrives only in the pen-up doc_change
    And 0 preview paths are written to the mirror

  @SRS-EP-08
  Scenario: A load offered mid-gesture is deferred
    Given a gesture in progress
    When a doc_load is offered
    Then the gesture commits first
    And the change publishes as doc_change before the load is accepted
