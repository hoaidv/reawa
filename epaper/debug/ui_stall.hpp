/**
 * GUI-thread stall detector. Heartbeat timer; if it fires late, the UI was blocked.
 * Nested sections wrap known heavy work so the next log names the culprit.
 * Finished sections also feed the ink-path probe (last-heavy), even when they are
 * under the ui-stall log bar (default 250 ms) — ink hitches are ~30–100 ms.
 * Log file: EPAPER_UI_STALL_LOG (default /tmp/epaper-ui-stall.log). Set EPAPER_UI_STALL=0 to disable.
 */
#pragma once

#include <cstdint>

namespace epaper {

class UiStallWatchdog;

/** RAII: records elapsed on the GUI thread when the block exceeds the threshold. */
class UiStallSection
{
public:
    explicit UiStallSection(const char *tag);
    ~UiStallSection();
    UiStallSection(const UiStallSection &) = delete;
    UiStallSection &operator=(const UiStallSection &) = delete;

private:
    const char *m_tag = nullptr;
    std::uint64_t m_startMs = 0;
};

void startUiStallWatchdog();

} // namespace epaper
