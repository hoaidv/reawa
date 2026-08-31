#pragma once
/**
 * Always-on GUI-thread attribution for InkStrokeOperation.
 *
 * Fast path: clock + fixed POD. File I/O only when a pointer sample itself, or
 * the work that just finished ahead of it, exceeds the ink budget.
 *
 * @implements [SRS-EP-01] pen-down → pixel attribution
 * @implements [SRS-EP-16] 0 log I/O on the fast path
 *
 * Env:
 *   EPAPER_INK_PATH=0     disable
 *   EPAPER_INK_PATH_MS    log threshold (default 30, SRS-EP-01)
 *   EPAPER_INK_PATH_LOG   path (default /tmp/epaper-ink-path.log)
 *
 * How to read a line:
 *   reason=slow_sample  — the callback itself was over budget; `slowest=` is the stage
 *   reason=queued       — GUI thread was busy (`behind=`) then this sample ran fast
 *   reason=both         — queued behind heavy work *and* the sample was slow
 *   `i=0 event=down`    — first sample of a stroke (the "lag then smooth" signature)
 */

#include <cstdint>
#include <string>

namespace epaper {
namespace inkpath {

enum class Event { Down, Move, Up };

/** One pointer callback (ToolCanvas onPointerStart/Move/End). */
class Sample
{
public:
    Sample(Event e, int inkCount, int nodeCount);
    ~Sample();
    Sample(const Sample &) = delete;
    Sample &operator=(const Sample &) = delete;

private:
    bool m_armed = false;
};

/** Named leaf stage. Nested spans flatten; prefer non-overlapping leaves. */
class Span
{
public:
    explicit Span(const char *tag);
    ~Span();
    Span(const Span &) = delete;
    Span &operator=(const Span &) = delete;

private:
    const char *m_tag = nullptr;
    std::uint64_t m_startMs = 0;
    int m_index = -1;
    bool m_armed = false;
};

/** Record a finished section (UiStallSection, rasterize, …) as last-heavy. */
void noteSection(const char *tag, int durationMs);

bool enabled();
int thresholdMs();

void resetForTest();
void setEnabledForTest(bool on);
void setThresholdMsForTest(int ms);
std::string takeLastLogForTest();

} // namespace inkpath
} // namespace epaper
