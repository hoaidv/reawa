/**
 * Host tests for STORY-EP-020 / [SRS-EP-08] one-way sync.
 * Maps one-way-sync.feature. No Qt.
 *
 * Build: ./tests/run_device_document_test.sh
 */

#include "document/device_document.hpp"
#include "document/one_way_sync.hpp"
#include "debug/latency_probe.hpp"

#include <iostream>
#include <string>

using namespace epaper::document;
using epaper::latencyprobe::nsToUs;
using epaper::latencyprobe::summarizeNs;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static JsonValue makeAppendInkOp(const std::string &id, double x = 10, double y = 20)
{
    const std::string json = std::string("{\"opId\":\"op-") + id + "\",\"type\":\"append_ink\","
                                                                  "\"source\":\"epaper\",\"payload\":{\"id\":\""
                             + id + "\",\"samples\":[{\"x\":" + std::to_string(x) + ",\"y\":"
                             + std::to_string(y) + "},{\"x\":" + std::to_string(x + 2) + ",\"y\":"
                             + std::to_string(y + 2)
                             + "}],\"style\":{\"stroke\":\"#1C2430\",\"strokeWidth\":2}}}";
    return parseJson(json);
}

static JsonValue emptyDoc()
{
    return parseJson(R"({"version":1,"status":"open","rootChildren":[]})");
}

static JsonValue loadedDoc()
{
    return parseJson(R"({
      "version": 1,
      "status": "open",
      "rootChildren": [{
        "id": "loaded",
        "kind": "ink",
        "samples": [{"x": 1, "y": 1}, {"x": 2, "y": 2}],
        "style": {"stroke": "#1C2430", "strokeWidth": 2}
      }]
    })");
}

static JsonValue docLoadMsg(const JsonValue &document)
{
    JsonValue::Object o;
    o.emplace_back("type", JsonValue::string("doc_load"));
    o.emplace_back("document", document);
    o.emplace_back("seq", JsonValue::number(0));
    return JsonValue::object(std::move(o));
}

static std::string typeOf(const std::string &line)
{
    return parseJson(line).getString("type");
}

static int countType(const std::vector<std::string> &lines, const std::string &t)
{
    int n = 0;
    for (const auto &l : lines) {
        if (typeOf(l) == t)
            ++n;
    }
    return n;
}

static JsonValue firstOf(const std::vector<std::string> &lines, const std::string &t)
{
    for (const auto &l : lines) {
        JsonValue j = parseJson(l);
        if (j.getString("type") == t)
            return j;
    }
    return JsonValue::null();
}

static void completeInitialLoad(OneWaySyncSession &sync)
{
    sync.onLinkUp();
    sync.handleInbound(docLoadMsg(emptyDoc()));
}

/** After the initial load, only viewport comes down */
static void test_after_initial_load_only_viewport()
{
    DeviceDocument doc;
    OneWaySyncSession sync(doc);
    completeInitialLoad(sync);
    CHECK(sync.epochLive());
    const int docBefore = sync.inboundDocumentBearingAfterLoad();
    sync.handleInbound(parseJson(R"({"type":"viewport","seq":3})"));
    sync.handleInbound(parseJson(R"({"type":"viewport","seq":4,"settle":true})"));
    CHECK(sync.inboundDocumentBearingAfterLoad() == docBefore);
    CHECK(sync.viewportApplied() >= 2);
    CHECK(sync.lastViewport().getNumber("seq") == 4);
}

/** Unsolicited document inbound is rejected */
static void test_unsolicited_document_inbound_rejected()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeAppendInkOp("keep")).applied);
    OneWaySyncSession sync(doc);
    completeInitialLoad(sync);
    // Initial load replaced the tree; put a node back as the local document.
    CHECK(doc.commitJson(makeAppendInkOp("local")).applied);
    const std::string snap = doc.snapshotString();
    const int ink = doc.inkCount();

    sync.handleInbound(docLoadMsg(loadedDoc()));
    sync.handleInbound(parseJson(R"({"type":"doc_snapshot","document":{}})"));
    sync.handleInbound(parseJson(R"({"type":"pickables","pickables":[]})"));
    sync.handleInbound(parseJson(R"({"type":"tool_intent","intent":"ink"})"));

    CHECK(doc.snapshotString() == snap);
    CHECK(doc.inkCount() == ink);
    CHECK(doc.find("local"));
    CHECK(!doc.find("loaded"));
    CHECK(sync.rejectLogs().size() >= 4);
    CHECK(!sync.statusLine().empty());
}

/** Load handshake drains the queue then starts a new epoch */
static void test_load_handshake_drains_then_new_epoch()
{
    DeviceDocument doc;
    OneWaySyncSession sync(doc);
    for (int i = 0; i < 5; ++i)
        CHECK(doc.commitJson(makeAppendInkOp(std::string("q") + std::to_string(i), double(i), 0))
                  .applied);
    CHECK(doc.publishQueue().size() == 5);

    sync.onLinkUp();
    JsonValue hello = firstOf(sync.outbound(), "hello");
    CHECK(hello.getNumber("queued") == 5);
    CHECK(hello.getNumber("lastSeq") == 5);

    sync.handleInbound(parseJson(R"({"type":"drain_ack"})"));
    const auto afterDrain = sync.outbound();
    CHECK(countType(afterDrain, "doc_change") == 5);
    int prevSeq = 0;
    int changes = 0;
    bool sawEmpty = false;
    for (const auto &l : afterDrain) {
        const std::string t = typeOf(l);
        if (t == "doc_change") {
            CHECK(!sawEmpty);
            const int seq = static_cast<int>(parseJson(l).getNumber("seq"));
            CHECK(seq == prevSeq + 1);
            prevSeq = seq;
            ++changes;
        }
        if (t == "queue_empty")
            sawEmpty = true;
    }
    CHECK(changes == 5);
    CHECK(sawEmpty);

    const std::size_t qBefore = doc.publishQueue().size();
    CHECK(qBefore == 5);
    sync.handleInbound(docLoadMsg(loadedDoc()));
    CHECK(doc.find("loaded"));
    CHECK(!doc.find("q0"));
    CHECK(doc.publishQueue().empty());
    CHECK(doc.lastSeq() == 0);
    CHECK(doc.undoDepth() == 0);
    CHECK(countType(sync.outbound(), "load_ack") == 1);
    CHECK(doc.publishQueue().size() == 0);
}

/** Editing continues with the session down */
static void test_editing_continues_session_down()
{
    DeviceDocument doc;
    OneWaySyncSession sync(doc);
    sync.onLinkDown();
    CHECK(!sync.linked());
    for (int i = 0; i < 10; ++i)
        CHECK(doc.commitJson(makeAppendInkOp(std::string("off") + std::to_string(i), double(i), 1))
                  .applied);
    CHECK(doc.inkCount() == 10);
    CHECK(doc.publishQueue().size() == 10);
    for (int i = 0; i < 10; ++i)
        CHECK(doc.publishQueue()[static_cast<std::size_t>(i)].seq == i + 1);
    CHECK(sync.toolsUnavailable() == 0);
}

/** Reconnect publishes the queue; duplicate opId is a no-op */
static void test_reconnect_publishes_queue_duplicate_noop()
{
    DeviceDocument doc;
    OneWaySyncSession sync(doc);
    for (int i = 0; i < 3; ++i)
        CHECK(doc.commitJson(makeAppendInkOp(std::string("r") + std::to_string(i))).applied);
    sync.onLinkUp();
    sync.handleInbound(parseJson(R"({"type":"drain_ack"})"));
    CHECK(sync.mirror().appliedCount == 3);
    CHECK(sync.mirror().appliedSeqs.size() == 3);
    CHECK(sync.mirror().appliedSeqs[0] == 1);
    CHECK(sync.mirror().appliedSeqs[2] == 3);

    const DocChange dup = doc.publishQueue().front();
    CHECK(!sync.mirror().applyDocChange(dup));
    CHECK(sync.mirror().noopDuplicates == 1);
    CHECK(sync.mirror().appliedCount == 3);
}

/** One doc_change per committed structural op */
static void test_one_doc_change_per_structural_op()
{
    DeviceDocument doc;
    OneWaySyncSession sync(doc);
    completeInitialLoad(sync);
    sync.takeOutbound();

    CHECK(isClosedTransmitOp("append_ink"));
    CHECK(doc.commitJson(makeAppendInkOp("one")).applied);
    sync.onLocalCommit();
    CHECK(countType(sync.outbound(), "doc_change") == 1);
    const JsonValue ch = firstOf(sync.outbound(), "doc_change");
    CHECK(ch.getNumber("seq") == 1);
    CHECK(ch.getNumber("baseSeq") == 0);
    CHECK(ch.getString("opId") == "op-one");
    CHECK(ch.get("op") && ch.get("op")->getString("type") == "append_ink");
    CHECK(isClosedTransmitOp(ch.get("op")->getString("type")));

    const auto p = summarizeNs(sync.publishLatencyNs());
    std::cout << "doc_change commit→emit n=" << p.n << " p95=" << nsToUs(p.p95Ns)
              << "us (bar 300ms)\n";
    CHECK(!sync.publishLatencyNs().empty());
    CHECK(p.p95Ns <= 300000000);
}

/** Preview stroke_* is not a document change */
static void test_preview_stroke_not_document_change()
{
    DeviceDocument doc;
    OneWaySyncSession sync(doc);
    completeInitialLoad(sync);
    sync.takeOutbound();
    const int ink0 = doc.inkCount();
    const std::size_t q0 = doc.publishQueue().size();

    sync.beginPreviewStroke("s-1");
    sync.previewStrokePoint("s-1", 1, 2, 0.4);
    sync.endPreviewStroke("s-1");

    CHECK(doc.inkCount() == ink0);
    CHECK(doc.publishQueue().size() == q0);
    CHECK(sync.mirror().previewPathsWritten == 0);
    CHECK(countType(sync.outbound(), "stroke_begin") == 1);
    CHECK(countType(sync.outbound(), "stroke_point") == 1);
    CHECK(countType(sync.outbound(), "stroke_end") == 1);
    CHECK(countType(sync.outbound(), "doc_change") == 0);
    const JsonValue begin = firstOf(sync.outbound(), "stroke_begin");
    CHECK(!begin.has("intent"));

    CHECK(doc.commitJson(makeAppendInkOp("penup")).applied);
    sync.onLocalCommit();
    CHECK(countType(sync.outbound(), "doc_change") == 1);
    CHECK(firstOf(sync.outbound(), "doc_change").get("op")->getString("type") == "append_ink");
}

/** A load offered mid-gesture is deferred */
static void test_load_mid_gesture_deferred()
{
    DeviceDocument doc;
    OneWaySyncSession sync(doc);
    sync.onLinkUp();
    doc.beginGesture();
    CHECK(doc.gestureInFlight());
    sync.handleInbound(docLoadMsg(loadedDoc()));
    CHECK(!doc.find("loaded"));
    CHECK(countType(sync.outbound(), "load_ack") == 0);

    CHECK(doc.commitJson(makeAppendInkOp("mid")).applied);
    CHECK(!doc.gestureInFlight());
    sync.onLocalCommit();
    CHECK(doc.find("loaded"));
    CHECK(!doc.find("mid"));
    const auto lines = sync.outbound();
    int changeAt = -1;
    int ackAt = -1;
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        const std::string t = typeOf(lines[static_cast<std::size_t>(i)]);
        if (t == "doc_change" && changeAt < 0)
            changeAt = i;
        if (t == "load_ack")
            ackAt = i;
    }
    CHECK(changeAt >= 0);
    CHECK(ackAt > changeAt);
}

/** Missed hello is retransmitted until the load epoch starts */
static void test_hello_retransmit_until_load()
{
    DeviceDocument doc;
    OneWaySyncSession sync(doc);
    sync.onLinkUp();
    CHECK(countType(sync.takeOutbound(), "hello") == 1);
    CHECK(!sync.epochLive());
    sync.retransmitHelloIfWaiting();
    CHECK(countType(sync.takeOutbound(), "hello") == 1);
    sync.handleInbound(docLoadMsg(emptyDoc()));
    CHECK(sync.epochLive());
    sync.retransmitHelloIfWaiting();
    CHECK(countType(sync.takeOutbound(), "hello") == 0);
}

static void test_hello_carries_document_when_queue_empty()
{
    DeviceDocument doc;
    doc.onAcceptedDocLoad(loadedDoc());
    OneWaySyncSession sync(doc);
    sync.onLinkUp();
    auto lines = sync.takeOutbound();
    CHECK(countType(lines, "hello") == 1);
    const JsonValue hello = parseJson(lines.front());
    CHECK(hello.getNumber("queued") == 0);
    const JsonValue *document = hello.get("document");
    CHECK(document && document->isObject());
    const JsonValue *kids = document->get("rootChildren");
    CHECK(kids && kids->isArray() && !kids->asArray().empty());
}

static void test_empty_doc_load_does_not_wipe_local_tree()
{
    DeviceDocument doc;
    doc.applyJson(makeAppendInkOp("keep"));
    OneWaySyncSession sync(doc);
    sync.onLinkUp();
    sync.takeOutbound();
    sync.handleInbound(docLoadMsg(emptyDoc()));
    CHECK(sync.epochLive());
    CHECK(doc.find("keep"));
    CHECK(countType(sync.takeOutbound(), "load_ack") == 1);
}

int main()
{
    test_after_initial_load_only_viewport();
    test_unsolicited_document_inbound_rejected();
    test_load_handshake_drains_then_new_epoch();
    test_editing_continues_session_down();
    test_reconnect_publishes_queue_duplicate_noop();
    test_one_doc_change_per_structural_op();
    test_preview_stroke_not_document_change();
    test_load_mid_gesture_deferred();
    test_hello_retransmit_until_load();
    test_hello_carries_document_when_queue_empty();
    test_empty_doc_load_does_not_wipe_local_tree();

    if (g_fails) {
        std::cerr << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "one_way_sync_test OK\n";
    return 0;
}
