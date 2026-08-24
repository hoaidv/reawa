#include "ui_stall.hpp"

#include <QCoreApplication>
#include <QThread>
#include <QTimer>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace epaper {
namespace {

constexpr int kBeatMs = 32;
constexpr int kDefaultLogMs = 250;

QTimer *g_beat = nullptr;
std::uint64_t g_lastMs = 0;
char g_section[64] = {};

/** A hitch the hand notices is well under a second. EPAPER_UI_STALL_MS to tune. */
int sectionThresholdMs()
{
    static const int ms = []() {
        if (const char *p = std::getenv("EPAPER_UI_STALL_MS")) {
            const int v = std::atoi(p);
            if (v > 0)
                return v;
        }
        return kDefaultLogMs;
    }();
    return ms;
}

std::uint64_t nowMs()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

const char *logPath()
{
    if (const char *p = std::getenv("EPAPER_UI_STALL_LOG")) {
        if (p[0])
            return p;
    }
    return "/tmp/epaper-ui-stall.log";
}

void appendLine(const char *line)
{
    const int fd = ::open(logPath(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0)
        return;
    const std::size_t n = std::strlen(line);
    ::write(fd, line, n);
    ::close(fd);
}

void logStall(int stallMs, const char *kind, const char *tag)
{
    char buf[256];
    std::snprintf(buf, sizeof(buf), "stall_ms=%d kind=%s tag=%s\n", stallMs, kind,
                  tag && tag[0] ? tag : "-");
    appendLine(buf);
    // qWarning would recurse into this if logging itself stalls; keep stderr only.
    std::fprintf(stderr, "[ui-stall] %s", buf);
}

void onBeat()
{
    const std::uint64_t t = nowMs();
    if (g_lastMs != 0) {
        const int gap = static_cast<int>(t - g_lastMs);
        const int stall = gap - kBeatMs;
        // Heartbeat stays coarser than sections: it fires on every e-ink refresh.
        if (stall >= sectionThresholdMs() * 4)
            logStall(stall, "heartbeat", g_section);
    }
    g_lastMs = t;
}

} // namespace

UiStallSection::UiStallSection(const char *tag)
    : m_tag(tag)
    , m_startMs(nowMs())
{
    if (tag && tag[0]) {
        std::strncpy(g_section, tag, sizeof(g_section) - 1);
        g_section[sizeof(g_section) - 1] = 0;
    }
}

UiStallSection::~UiStallSection()
{
    const int ms = static_cast<int>(nowMs() - m_startMs);
    if (ms >= sectionThresholdMs())
        logStall(ms, "section", m_tag ? m_tag : "-");
    g_section[0] = 0;
}

void startUiStallWatchdog()
{
    if (const char *off = std::getenv("EPAPER_UI_STALL")) {
        if (off[0] == '0' || off[0] == 'n' || off[0] == 'N')
            return;
    }
    if (g_beat)
        return;
    auto *app = QCoreApplication::instance();
    if (!app)
        return;
    g_beat = new QTimer(app);
    g_beat->setInterval(kBeatMs);
    g_beat->setTimerType(Qt::PreciseTimer);
    QObject::connect(g_beat, &QTimer::timeout, app, []() { onBeat(); });
    g_lastMs = nowMs();
    g_beat->start();
    char boot[128];
    std::snprintf(boot, sizeof(boot), "stall_ms=0 kind=boot tag=ui-stall path=%s\n", logPath());
    appendLine(boot);
}

} // namespace epaper
