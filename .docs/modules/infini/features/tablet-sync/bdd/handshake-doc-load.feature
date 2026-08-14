@SRS-IN-07
Feature: Infini handshake-gated doc_load
  As Infini under ADR-0015
  I drain the device queue then send one doc_load per epoch
  So that replace-on-load is safe and the old doc_snapshot reflex is gone

  # STORY-IN-028 — SRS-IN-07 / SRS-IN-08. Wire names are the SRS-IN-09 / ADR-0015
  # transmit set. Do not invent aliases. Home for this story; do not rewrite
  # session-channels.feature.

  @SRS-IN-07
  Scenario: Drain then doc_load when queued is greater than zero
    Given Epaper connects and sends hello with lastSeq 4 and queued 2
    When Infini handshakes
    Then Infini sends drain_ack before any doc_load
    And Infini applies inbound doc_change messages in seq order
    And the two doc_change envelopes use type "doc_change" with op envelopes { opId, type, payload }
    And those ops use SRS-IN-09 transmit types such as append_ink
    And Infini sends doc_load { type: "doc_load", document, seq: 0 } only after queue_empty
    And Infini does not send doc_load while queued is greater than 0

  @SRS-IN-07 @SRS-IN-08
  Scenario: Zero outbound document messages after the load
    Given a session whose handshake completed with one doc_load
    When the rest of the session is traced
    Then 0 further outbound document messages are observed
    And viewport messages may still flow downward
    And the count is by message type: 0 of doc_load and 0 of any other document type

  @SRS-IN-07 @SRS-IN-08
  Scenario: Reconnect runs the handshake instead of a reflexive document push
    Given a prior epoch that completed load_ack
    When the session returns on reconnect
    Then Infini does not push a document reflexively
    And Infini waits for hello { lastSeq, queued }
    And if queued is greater than 0 Infini sends drain_ack, applies doc_change in seq order, and waits for queue_empty
    And only then Infini sends one doc_load { type: "doc_load", document, seq: 0 }

  @SRS-IN-07
  Scenario: Orientation change and Infini-side actions send zero doc_load
    Given a live session after the epoch doc_load
    When the user cycles gut orientation
    Then Infini sends 0 doc_load messages
    And Infini sends 0 doc_snapshot messages
    When an Infini-side canvas action is observed
    Then Infini still sends 0 doc_load and 0 doc_snapshot messages
    And only viewport may flow down for that action

  @SRS-IN-07
  Scenario: Retired snapshot pickables and tool_intent are not emitted
    Given Infini would historically have emitted doc_snapshot, pickables, or tool_intent
    When outbound messages on TCP 9877 are observed for a session epoch
    Then Infini emits 0 messages with type "doc_snapshot"
    And Infini emits 0 pickables arrays
    And Infini emits 0 messages with type "tool_intent"
    And Infini emits 0 stroke_begin.intent fields
    And the load message type is "doc_load" per SRS-IN-09
