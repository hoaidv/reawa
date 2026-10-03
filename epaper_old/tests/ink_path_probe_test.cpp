/**
 * Ink-path probe: queued-behind vs slow-sample attribution.
 * Host test, no Qt.
 */
#include "debug/ink_path_probe.hpp"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
            return 1;                                                                          \
        }                                                                                      \
    } while (0)

using epaper::inkpath::Event;
using epaper::inkpath::Sample;
using epaper::inkpath::Span;

int main()
{
    epaper::inkpath::resetForTest();
    epaper::inkpath::setEnabledForTest(false);
    epaper::inkpath::setThresholdMsForTest(30);
    {
        Sample s(Event::Down, 4, 9);
        Span span("emitSegment");
    }
    CHECK(epaper::inkpath::takeLastLogForTest().empty());

    epaper::inkpath::resetForTest();
    epaper::inkpath::setEnabledForTest(true);
    epaper::inkpath::setThresholdMsForTest(30);
    epaper::inkpath::noteSection("rasterizeVectors", 108);
    {
        Sample s(Event::Down, 412, 480);
    }
    const std::string queued = epaper::inkpath::takeLastLogForTest();
    CHECK(queued.find("event=down") != std::string::npos);
    CHECK(queued.find("reason=queued") != std::string::npos);
    CHECK(queued.find("behind=rasterizeVectors") != std::string::npos);
    CHECK(queued.find("behind_ms=108") != std::string::npos);
    CHECK(queued.find("ink=412") != std::string::npos);
    CHECK(queued.find("nodes=480") != std::string::npos);
    CHECK(queued.find("i=0") != std::string::npos);

    epaper::inkpath::resetForTest();
    epaper::inkpath::setEnabledForTest(true);
    epaper::inkpath::setThresholdMsForTest(30);
    {
        Sample s(Event::Down, 8, 8);
        {
            Span span("flushPending");
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
    }
    const std::string slow = epaper::inkpath::takeLastLogForTest();
    CHECK(slow.find("reason=slow_sample") != std::string::npos
          || slow.find("reason=both") != std::string::npos);
    CHECK(slow.find("slowest=flushPending") != std::string::npos);
    CHECK(slow.find("spans=flushPending:") != std::string::npos);

    epaper::inkpath::resetForTest();
    epaper::inkpath::setEnabledForTest(true);
    epaper::inkpath::setThresholdMsForTest(30);
    {
        Sample s(Event::Move, 1, 1);
    }
    CHECK(epaper::inkpath::takeLastLogForTest().empty());

    std::printf("ink_path_probe_test: OK\n");
    return 0;
}
