#include "ink_path_probe.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace epaper {
namespace inkpath {
namespace {

constexpr int kDefaultMs = 30; // SRS-EP-01 pen-down → pixel
constexpr int kNotableMs = 16; // remember last-heavy below the log bar
constexpr int kBehindWindowMs = 80;
constexpr int kMaxSpans = 12;
constexpr int kHeavyRing = 4;

int g_enabled = -1; // -1 env, 0 off, 1 on
int g_thresholdMs = -1;
int g_stroke = 0;
int g_sampleInStroke = 0;
std::uint64_t g_lastSampleEndMs = 0;
std::string g_lastLog;

struct SpanRec {
    char tag[40] = {};
    int ms = 0;
};

struct SampleRec {
    bool active = false;
    Event event = Event::Move;
    int ink = 0;
    int nodes = 0;
    int stroke = 0;
    int index = 0;
    std::uint64_t startMs = 0;
    std::uint64_t gapMs = 0;
    SpanRec spans[kMaxSpans];
    int nSpans = 0;
};

struct Heavy {
    char tag[40] = {};
    int durationMs = 0;
    std::uint64_t endMs = 0;
};

SampleRec g_sample;
Heavy g_heavy[kHeavyRing];
int g_heavyN = 0;

std::uint64_t nowMs()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

int readThreshold()
{
    if (g_thresholdMs > 0)
        return g_thresholdMs;
    if (const char *p = std::getenv("EPAPER_INK_PATH_MS")) {
        const int v = std::atoi(p);
        if (v > 0)
            return v;
    }
    return kDefaultMs;
}

const char *logPath()
{
    if (const char *p = std::getenv("EPAPER_INK_PATH_LOG")) {
        if (p[0])
            return p;
    }
    return "/tmp/epaper-ink-path.log";
}

void appendFile(const char *line)
{
    const int fd = ::open(logPath(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0)
        return;
    const std::size_t n = std::strlen(line);
    ::write(fd, line, n);
    ::close(fd);
}

const char *eventName(Event e)
{
    switch (e) {
    case Event::Down:
        return "down";
    case Event::Move:
        return "move";
    case Event::Up:
        return "up";
    }
    return "move";
}

void pushHeavy(const char *tag, int durationMs, std::uint64_t endMs)
{
    Heavy h;
    std::strncpy(h.tag, tag && tag[0] ? tag : "-", sizeof(h.tag) - 1);
    h.durationMs = durationMs;
    h.endMs = endMs;
    if (g_heavyN < kHeavyRing) {
        g_heavy[g_heavyN++] = h;
        return;
    }
    for (int i = 1; i < kHeavyRing; ++i)
        g_heavy[i - 1] = g_heavy[i];
    g_heavy[kHeavyRing - 1] = h;
}

void emitLine(const char *line)
{
    g_lastLog = line;
    appendFile(line);
    std::fprintf(stderr, "%s", line);
}

void logSample(int totalMs)
{
    const int bar = readThreshold();
    const Heavy *behind = nullptr;
    char recent[240];
    recent[0] = 0;
    int recentUsed = 0;
    for (int i = 0; i < g_heavyN; ++i) {
        const Heavy &h = g_heavy[i];
        if (g_sample.startMs < h.endMs)
            continue;
        if ((g_sample.startMs - h.endMs) > std::uint64_t(kBehindWindowMs))
            continue;
        if (h.durationMs >= bar && (!behind || h.durationMs >= behind->durationMs))
            behind = &h;
        const int n = std::snprintf(recent + recentUsed, sizeof(recent) - std::size_t(recentUsed),
                                    "%s%s:%d", recentUsed ? "," : "", h.tag, h.durationMs);
        if (n < 0 || recentUsed + n >= int(sizeof(recent)))
            break;
        recentUsed += n;
    }
    if (!recent[0])
        std::strncpy(recent, "-", sizeof(recent) - 1);

    const bool queued = behind != nullptr;
    const bool slow = totalMs >= bar;
    if (!slow && !queued)
        return;

    const char *reason = (slow && queued) ? "both" : (queued ? "queued" : "slow_sample");
    const char *slowest = "-";
    int slowestMs = 0;
    for (int i = 0; i < g_sample.nSpans; ++i) {
        if (g_sample.spans[i].ms > slowestMs) {
            slowestMs = g_sample.spans[i].ms;
            slowest = g_sample.spans[i].tag;
        }
    }

    char spans[400];
    spans[0] = 0;
    int used = 0;
    for (int i = 0; i < g_sample.nSpans; ++i) {
        const int n = std::snprintf(spans + used, sizeof(spans) - std::size_t(used),
                                    "%s%s:%d", used ? "," : "", g_sample.spans[i].tag,
                                    g_sample.spans[i].ms);
        if (n < 0 || used + n >= int(sizeof(spans)))
            break;
        used += n;
    }
    if (!spans[0])
        std::strncpy(spans, "-", sizeof(spans) - 1);

    char buf[768];
    std::snprintf(buf, sizeof(buf),
                  "[ink-path] event=%s stroke=%d i=%d total_ms=%d gap_ms=%d reason=%s "
                  "slowest=%s slowest_ms=%d behind=%s behind_ms=%d recent=%s ink=%d nodes=%d "
                  "spans=%s\n",
                  eventName(g_sample.event), g_sample.stroke, g_sample.index, totalMs,
                  int(g_sample.gapMs), reason, slowest, slowestMs,
                  queued && behind ? behind->tag : "-", queued && behind ? behind->durationMs : 0,
                  recent, g_sample.ink, g_sample.nodes, spans);
    emitLine(buf);
}

} // namespace

bool enabled()
{
    if (g_enabled >= 0)
        return g_enabled == 1;
    if (const char *off = std::getenv("EPAPER_INK_PATH")) {
        if (off[0] == '0' || off[0] == 'n' || off[0] == 'N' || off[0] == 'f' || off[0] == 'F') {
            g_enabled = 0;
            return false;
        }
    }
    g_enabled = 1;
    return true;
}

int thresholdMs()
{
    return readThreshold();
}

void noteSection(const char *tag, int durationMs)
{
    if (durationMs < kNotableMs)
        return;
    pushHeavy(tag, durationMs, nowMs());
}

Sample::Sample(Event e, int inkCount, int nodeCount)
{
    if (!enabled())
        return;
    m_armed = true;
    if (e == Event::Down) {
        ++g_stroke;
        g_sampleInStroke = 0;
    } else {
        ++g_sampleInStroke;
    }
    const std::uint64_t t = nowMs();
    g_sample.active = true;
    g_sample.event = e;
    g_sample.ink = inkCount;
    g_sample.nodes = nodeCount;
    g_sample.stroke = g_stroke;
    g_sample.index = g_sampleInStroke;
    g_sample.startMs = t;
    g_sample.gapMs = (g_lastSampleEndMs == 0) ? 0 : (t - g_lastSampleEndMs);
    g_sample.nSpans = 0;
}

Sample::~Sample()
{
    if (!m_armed)
        return;
    const std::uint64_t t = nowMs();
    const int total = static_cast<int>(t - g_sample.startMs);
    logSample(total);
    g_lastSampleEndMs = t;
    g_sample.active = false;
}

Span::Span(const char *tag)
    : m_tag(tag)
    , m_startMs(nowMs())
{
    if (!enabled())
        return;
    m_armed = true;
    if (!g_sample.active || g_sample.nSpans >= kMaxSpans)
        return;
    m_index = g_sample.nSpans;
    std::strncpy(g_sample.spans[m_index].tag, tag && tag[0] ? tag : "-",
                 sizeof(g_sample.spans[m_index].tag) - 1);
    g_sample.spans[m_index].ms = 0;
    ++g_sample.nSpans;
}

Span::~Span()
{
    if (!m_armed)
        return;
    const int ms = static_cast<int>(nowMs() - m_startMs);
    if (m_index >= 0 && m_index < g_sample.nSpans)
        g_sample.spans[m_index].ms = ms;
    if (ms >= kNotableMs)
        noteSection(m_tag, ms);
}

void resetForTest()
{
    g_enabled = -1;
    g_thresholdMs = -1;
    g_stroke = 0;
    g_sampleInStroke = 0;
    g_lastSampleEndMs = 0;
    g_lastLog.clear();
    g_sample = SampleRec{};
    g_heavyN = 0;
    for (int i = 0; i < kHeavyRing; ++i)
        g_heavy[i] = Heavy{};
}

void setEnabledForTest(bool on)
{
    g_enabled = on ? 1 : 0;
}

void setThresholdMsForTest(int ms)
{
    g_thresholdMs = ms > 0 ? ms : 1;
}

std::string takeLastLogForTest()
{
    std::string out = g_lastLog;
    g_lastLog.clear();
    return out;
}

} // namespace inkpath
} // namespace epaper
