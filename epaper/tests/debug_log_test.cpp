/**
 * STORY-EP-021 / @SRS-EP-15 @SRS-EP-16 — debug-log queue, env, enclose line, type filter.
 * Host test, no Qt.
 */
#include "debuglog/debug_log_format.hpp"
#include "debuglog/debug_log_queue.hpp"

#include <cstdio>
#include <string>
#include <utility>

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
            return 1;                                                                          \
        }                                                                                      \
    } while (0)

using epaper::debuglog::debugLogEnvOn;
using epaper::debuglog::debugLogPort;
using epaper::debuglog::DebugLogQueue;
using epaper::debuglog::DebugLogRecord;
using epaper::debuglog::formatEncloseLog;
using epaper::debuglog::formatInkLog;
using epaper::debuglog::isDocumentTypeOnDebugPort;
using epaper::debuglog::isDebugControlType;

int main()
{
    CHECK(!debugLogEnvOn(nullptr));
    CHECK(!debugLogEnvOn(""));
    CHECK(!debugLogEnvOn("0"));
    CHECK(!debugLogEnvOn("false"));
    CHECK(debugLogEnvOn("1"));
    CHECK(debugLogEnvOn("TRUE"));
    CHECK(debugLogEnvOn("on"));
    CHECK(debugLogEnvOn("Yes"));

    CHECK(debugLogPort(nullptr, nullptr) == 9878);
    CHECK(debugLogPort("", "") == 9878);
    CHECK(debugLogPort("9999", "9878") == 9999);
    CHECK(debugLogPort("", "1111") == 1111);

    CHECK(isDebugControlType("debug_start"));
    CHECK(isDebugControlType("debug_stop"));
    CHECK(isDebugControlType("debug_request"));
    CHECK(!isDebugControlType("debug_log"));
    CHECK(isDocumentTypeOnDebugPort("doc_change"));
    CHECK(isDocumentTypeOnDebugPort("viewport"));
    CHECK(isDocumentTypeOnDebugPort("stroke_begin"));
    CHECK(!isDocumentTypeOnDebugPort("debug_log"));

    DebugLogQueue q;
    for (int i = 0; i < 512; ++i) {
        DebugLogRecord r;
        r.ts = i;
        r.level = "info";
        r.logger = "qt";
        r.msg = "m" + std::to_string(i);
        CHECK(q.tryPush(std::move(r)));
    }
    CHECK(q.size() == 512);
    DebugLogRecord extra;
    extra.ts = 999;
    extra.level = "info";
    extra.logger = "qt";
    extra.msg = "newest";
    CHECK(q.tryPush(std::move(extra)));
    CHECK(q.size() == 512);
    const auto batch = q.takeAll();
    CHECK(batch.size() == 512);
    CHECK(batch.front().first.msg == "m1");
    CHECK(batch.back().first.msg == "newest");
    CHECK(batch.front().second >= 1);
    CHECK(q.size() == 0);

    CHECK(formatInkLog("stroke_42") == "[ink] id=stroke_42");
    CHECK(formatEncloseLog("Created", "", "sg_enclose_1", {"stroke_1", "ink_a", "ink_b"})
          == "[enclose] armed=ink_box outcome=created id=sg_enclose_1 "
             "children=[stroke_1,ink_a,ink_b] captured=2");
    CHECK(formatEncloseLog("OrdinaryInk", "too_small", "")
          == "[enclose] armed=ink_box outcome=stayed_ink guard=size captured=0");
    CHECK(epaper::debuglog::formatRecogLog("enclose", "none")
          == "[recog] outcome=enclose guard=none");
    CHECK(epaper::debuglog::formatRecogLog("membership", "no_content")
          == "[recog] outcome=membership guard=no_content");
    CHECK(epaper::debuglog::formatConnLog("n=2 ids=s-1,s-2 s-1~s-2=cross", "no_two_bindings")
          == "[conn] n=2 ids=s-1,s-2 s-1~s-2=cross fail=no_two_bindings");

    std::printf("debug_log_test OK\n");
    return 0;
}
