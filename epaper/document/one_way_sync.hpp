#pragma once
/**
 * Device one-way sync: inbound classification, load handshake, publish queue, preview.
 * @implements [SRS-EP-08] one-way sync handshake and publish queue
 */

#include "device_document.hpp"

#include <chrono>
#include <set>
#include <string>
#include <vector>

namespace epaper {
namespace document {

inline bool isDocumentBearingType(const std::string &type)
{
    return type == "doc_load" || type == "doc_snapshot" || type == "pickables"
        || type == "tool_intent" || type == "doc_change";
}

inline bool isDebugFamilyType(const std::string &type)
{
    return type == "debug_request" || type == "debug_start" || type == "debug_stop"
        || type == "debug_log";
}

inline bool isPreviewStrokeType(const std::string &type)
{
    return type == "stroke_begin" || type == "stroke_point" || type == "stroke_end";
}

/**
 * Host-side apply of a published change (test analog of the Infini mirror).
 * @implements [SRS-EP-08] duplicate opId apply is a no-op
 */
struct MirrorApplier {
    std::set<std::string> appliedOpIds;
    int previewPathsWritten = 0;
    int appliedCount = 0;
    int noopDuplicates = 0;
    std::vector<int> appliedSeqs;

    bool applyDocChange(const DocChange &ch)
    {
        if (appliedOpIds.count(ch.opId)) {
            ++noopDuplicates;
            return false;
        }
        appliedOpIds.insert(ch.opId);
        appliedSeqs.push_back(ch.seq);
        ++appliedCount;
        return true;
    }

    void notePreviewPath() { ++previewPathsWritten; }
};

/**
 * Session protocol over JSON-lines. Transport-agnostic (no Qt).
 * @implements [SRS-EP-08] inbound classification, handshake, publish, preview
 */
class OneWaySyncSession {
public:
    explicit OneWaySyncSession(DeviceDocument &doc) : m_doc(doc) {}

    bool linked() const { return m_linked; }
    bool epochLive() const { return m_epochLive; }
    bool handshakeInFlight() const { return m_handshakeInFlight; }
    bool loadLegal() const { return m_loadLegal; }
    int inboundDocumentBearingAfterLoad() const { return m_inboundDocAfterLoad; }
    int viewportApplied() const { return m_viewportApplied; }
    int toolsUnavailable() const { return m_toolsUnavailable; }
    bool previewStrokeActive() const { return m_previewActive; }
    const std::vector<std::string> &rejectLogs() const { return m_rejectLogs; }
    const std::vector<std::string> &statusLine() const { return m_statusLine; }
    const std::vector<std::string> &outbound() const { return m_outbound; }
    MirrorApplier &mirror() { return m_mirror; }
    const MirrorApplier &mirror() const { return m_mirror; }

    std::vector<std::string> takeOutbound()
    {
        std::vector<std::string> out;
        out.swap(m_outbound);
        return out;
    }

    void onLinkDown()
    {
        m_linked = false;
        m_handshakeInFlight = false;
        m_awaitingDrainAck = false;
        m_loadLegal = false;
        m_wireCursor = 0;
    }

    /**
     * @implements [SRS-EP-08] hello { lastSeq, queued }
     */
    void onLinkUp()
    {
        m_linked = true;
        m_wireCursor = 0;
        const int k = static_cast<int>(m_doc.publishQueue().size());
        emitHello(k);
        m_handshakeInFlight = true;
        if (k == 0) {
            m_loadLegal = true;
            m_awaitingDrainAck = false;
        } else {
            m_loadLegal = false;
            m_awaitingDrainAck = true;
        }
    }

    /**
     * Re-send hello while waiting for drain_ack / doc_load (renderer remount or missed hello).
     * @implements [SRS-EP-08] hello { lastSeq, queued }
     */
    void retransmitHelloIfWaiting()
    {
        if (!m_linked || m_epochLive)
            return;
        emitHello(static_cast<int>(m_doc.publishQueue().size()));
    }

    /**
     * After a local commit/undo. Publishes immediately only while live and linked
     * (not waiting on drain). Session-down commits stay queued — 0 tools gated.
     * @implements [SRS-EP-08] link down editing continues
     */
    void onLocalCommit()
    {
        maybeAcceptDeferredLoad();
        if (m_linked && m_epochLive && !m_handshakeInFlight && !m_awaitingDrainAck)
            publishFromCursor();
    }

    /**
     * @implements [SRS-EP-08] preview stroke_* is not a document change
     */
    void beginPreviewStroke(const std::string &id)
    {
        m_previewActive = true;
        if (!m_linked)
            return;
        JsonValue::Object o;
        o.emplace_back("type", JsonValue::string("stroke_begin"));
        o.emplace_back("id", JsonValue::string(id));
        emitLine(JsonValue::object(std::move(o)));
    }

    void previewStrokePoint(const std::string &id, double x, double y, double p)
    {
        if (!m_linked)
            return;
        JsonValue::Object o;
        o.emplace_back("type", JsonValue::string("stroke_point"));
        o.emplace_back("id", JsonValue::string(id));
        o.emplace_back("x", JsonValue::number(x));
        o.emplace_back("y", JsonValue::number(y));
        o.emplace_back("p", JsonValue::number(p));
        emitLine(JsonValue::object(std::move(o)));
    }

    void endPreviewStroke(const std::string &id)
    {
        if (m_linked) {
            JsonValue::Object o;
            o.emplace_back("type", JsonValue::string("stroke_end"));
            o.emplace_back("id", JsonValue::string(id));
            emitLine(JsonValue::object(std::move(o)));
        }
        m_previewActive = false;
        maybeAcceptDeferredLoad();
    }

    void handleInboundLine(const std::string &line)
    {
        JsonValue msg;
        try {
            msg = parseJson(line);
        } catch (...) {
            logReject("malformed_json", false);
            return;
        }
        if (!msg.isObject()) {
            logReject("malformed_json", false);
            return;
        }
        handleInbound(msg);
    }

    void handleInbound(const JsonValue &msg)
    {
        const std::string type = msg.getString("type");
        const bool wasLive = m_epochLive && !m_handshakeInFlight;
        if (wasLive && isDocumentBearingType(type))
            ++m_inboundDocAfterLoad;

        if (type == "viewport") {
            ++m_viewportApplied;
            m_lastViewport = msg;
            return;
        }
        if (type == "drain_ack") {
            onDrainAck();
            return;
        }
        if (type == "doc_load") {
            onDocLoad(msg);
            return;
        }
        if (type == "doc_snapshot" || type == "pickables" || type == "tool_intent") {
            logReject(std::string("retired:") + type, true);
            return;
        }
        if (type == "region_refresh") {
            logOnce("region_refresh", "ignore region_refresh; paint stays on local tree");
            return;
        }
        if (isDebugFamilyType(type)) {
            logReject(std::string("debug_on_doc_socket:") + type, true);
            return;
        }
        logReject(std::string("unknown_type:") + type, true);
    }

    const JsonValue &lastViewport() const { return m_lastViewport; }

    /**
     * Host analog: commit→emit latency samples for published structural ops.
     * @implements [SRS-EP-08] mirror p95 <= 300 ms
     */
    const std::vector<std::int64_t> &publishLatencyNs() const { return m_publishLatencyNs; }

private:
    DeviceDocument &m_doc;
    MirrorApplier m_mirror;
    bool m_linked = false;
    bool m_epochLive = false;
    bool m_handshakeInFlight = false;
    bool m_awaitingDrainAck = false;
    bool m_loadLegal = false;
    bool m_previewActive = false;
    bool m_hasDeferredLoad = false;
    JsonValue m_deferredLoad;
    int m_wireCursor = 0;
    int m_inboundDocAfterLoad = 0;
    int m_viewportApplied = 0;
    int m_toolsUnavailable = 0;
    std::vector<std::string> m_outbound;
    std::vector<std::string> m_rejectLogs;
    std::vector<std::string> m_statusLine;
    std::set<std::string> m_loggedOnce;
    JsonValue m_lastViewport;
    std::vector<std::int64_t> m_publishLatencyNs;

    void emitLine(const JsonValue &j) { m_outbound.push_back(stringify(j)); }

    void emitHello(int queued)
    {
        JsonValue::Object o;
        o.emplace_back("type", JsonValue::string("hello"));
        o.emplace_back("lastSeq", JsonValue::number(m_doc.lastSeq()));
        o.emplace_back("queued", JsonValue::number(queued));
        if (queued == 0 && !m_doc.rootChildren.empty())
            o.emplace_back("document", m_doc.toJSON());
        emitLine(JsonValue::object(std::move(o)));
    }

    void drainQueueThenMarkLegal()
    {
        const int before = m_wireCursor;
        publishFromCursor();
        if (m_wireCursor > before) {
            JsonValue::Object qe;
            qe.emplace_back("type", JsonValue::string("queue_empty"));
            emitLine(JsonValue::object(std::move(qe)));
        }
        m_awaitingDrainAck = false;
        m_loadLegal = true;
    }

    void onDrainAck()
    {
        drainQueueThenMarkLegal();
        maybeAcceptDeferredLoad();
    }

    /**
     * @implements [SRS-EP-08] drain before accept; 0 queued discarded
     */
    void onDocLoad(const JsonValue &msg)
    {
        if (m_epochLive && !m_handshakeInFlight) {
            logReject("unsolicited_doc_load", true);
            return;
        }
        if (!docLoadWellFormed(msg)) {
            logReject("malformed_doc_load", false);
            return;
        }
        if (m_doc.gestureInFlight() || m_previewActive) {
            m_hasDeferredLoad = true;
            m_deferredLoad = msg;
            return;
        }
        if (!m_loadLegal && !m_handshakeInFlight && m_doc.publishQueue().empty()) {
            logReject("unsolicited_doc_load", true);
            return;
        }
        drainQueueThenMarkLegal();
        acceptLoad(msg);
    }

    static bool incomingDocumentEmpty(const JsonValue &msg)
    {
        const JsonValue *document = msg.get("document");
        if (!document || !document->isObject())
            return true;
        const JsonValue *kids = document->get("rootChildren");
        return !kids || !kids->isArray() || kids->asArray().empty();
    }

    static bool docLoadWellFormed(const JsonValue &msg)
    {
        const JsonValue *document = msg.get("document");
        if (!document || !document->isObject())
            return false;
        if (msg.has("seq") && msg.getNumber("seq") != 0)
            return false;
        return true;
    }

    void maybeAcceptDeferredLoad()
    {
        if (!m_hasDeferredLoad)
            return;
        if (m_doc.gestureInFlight() || m_previewActive)
            return;
        JsonValue load = m_deferredLoad;
        m_hasDeferredLoad = false;
        m_deferredLoad = JsonValue::null();
        drainQueueThenMarkLegal();
        acceptLoad(load);
    }

    /**
     * @implements [SRS-EP-08] accepted load is a new epoch
     */
    void acceptLoad(const JsonValue &msg)
    {
        const JsonValue *document = msg.get("document");
        if (!document)
            return;
        if (incomingDocumentEmpty(msg) && !m_doc.rootChildren.empty()) {
            logReject("empty_doc_load_keep_local", false);
            m_wireCursor = 0;
            m_epochLive = true;
            m_handshakeInFlight = false;
            m_loadLegal = false;
            m_awaitingDrainAck = false;
            JsonValue::Object ack;
            ack.emplace_back("type", JsonValue::string("load_ack"));
            emitLine(JsonValue::object(std::move(ack)));
            return;
        }
        m_doc.onAcceptedDocLoad(*document);
        m_wireCursor = 0;
        m_epochLive = true;
        m_handshakeInFlight = false;
        m_loadLegal = false;
        m_awaitingDrainAck = false;
        JsonValue::Object ack;
        ack.emplace_back("type", JsonValue::string("load_ack"));
        emitLine(JsonValue::object(std::move(ack)));
    }

    void publishFromCursor()
    {
        const auto &q = m_doc.publishQueue();
        const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();
        while (m_wireCursor < static_cast<int>(q.size())) {
            const DocChange &ch = q[static_cast<std::size_t>(m_wireCursor)];
            emitLine(docChangeToJson(ch));
            m_mirror.applyDocChange(ch);
            if (ch.committedAtMs > 0)
                m_publishLatencyNs.push_back((nowMs - ch.committedAtMs) * 1000000);
            ++m_wireCursor;
        }
    }

    void logOnce(const std::string &key, const std::string &msg)
    {
        if (m_loggedOnce.count(key))
            return;
        m_loggedOnce.insert(key);
        m_rejectLogs.push_back(msg);
        m_statusLine.push_back(msg);
    }

    void logReject(const std::string &key, bool oncePerSession)
    {
        if (oncePerSession && m_loggedOnce.count(key))
            return;
        if (oncePerSession)
            m_loggedOnce.insert(key);
        m_rejectLogs.push_back(key);
        m_statusLine.push_back(key);
    }
};

} // namespace document
} // namespace epaper
