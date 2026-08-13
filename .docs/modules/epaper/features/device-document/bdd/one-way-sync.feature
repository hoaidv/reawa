@SRS-EP-08
Feature: One-way document sync from the device
  As the device that owns the working document
  I need exactly one document input and an ordered outbound change stream
  So that no peer can overwrite the creator's work mid-session

  # New 2026-08-13 — CHL-0008 / ADR-0014 / ADR-0015.
  # Replaces the device half of infini SRS-IN-13 (retired).

  @SRS-EP-08
  Scenario: Only viewport and one load ever come down
    Given a session that has completed its initial load
    When the whole session lifetime is traced
    Then zero inbound document-bearing messages are observed
    And viewport messages continue to be applied

  @SRS-EP-08
  Scenario: Unsolicited doc_load mid-session is rejected
    Given a live session with a local document
    When a doc_load arrives without a completed handshake
    Then the local document is unchanged
    And the message is logged as a protocol defect
    And the status line reflects it

  @SRS-EP-08
  Scenario: Retired message types are rejected, not tolerated
    Given a live session
    When a doc_snapshot or pickables or tool_intent message arrives
    Then it is not applied
    And it is logged once for the session

  @SRS-EP-08
  Scenario: Load handshake drains the queue first
    Given the device has 5 queued changes
    When the desktop offers a doc_load
    Then the device publishes all 5 changes in seq order
    And sends queue_empty
    And only then accepts the doc_load
    And zero queued changes are discarded

  @SRS-EP-08
  Scenario: Accepted load starts a new epoch
    Given an accepted doc_load
    Then the local document is replaced wholesale
    And seq resets to 0
    And the undo ring is cleared
    And the selection is cleared
    And load_ack is sent

  @SRS-EP-08
  Scenario: Editing continues with the link down
    Given the session is down
    When the creator performs 10 document operations
    Then all 10 are applied locally
    And all 10 are queued in order
    And zero tools were unavailable

  @SRS-EP-08
  Scenario: Reconnect publishes the queue in order
    Given 10 queued changes and a restored link
    When the handshake completes with drain_ack
    Then all 10 publish in seq order
    And zero are lost or reordered

  @SRS-EP-08
  Scenario: Duplicate delivery is idempotent
    Given a published change with opId "op_7"
    When the same opId is delivered again
    Then the mirror is unchanged by the second apply

  @SRS-EP-08
  Scenario: One change per committed op
    Given a committed structural op
    When it publishes
    Then exactly one doc_change is emitted
    And it carries seq, opId, op, and baseSeq
    And the mirror updates within p95 300 ms

  @SRS-EP-08
  Scenario: Preview strokes are not document changes
    Given a stroke in progress
    When stroke_begin, stroke_point, and stroke_end are streamed
    Then no stroke_begin carries an intent field
    And the authoritative node reaches the desktop only in the doc_change at pen-up
    And zero preview paths are written to the mirror

  @SRS-EP-08
  Scenario: A load offered mid-gesture is deferred
    Given a manipulation gesture in progress and an empty queue
    When a doc_load is offered
    Then the gesture completes and commits first
    And the change publishes before the load is accepted
