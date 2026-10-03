/**
 * Host harness for STORY-EP-013 / [SRS-EP-13] — stub document + hit-test probe.
 * No Qt. Not a device p95; prints an explicit caveat.
 *
 * Build: ./tests/run_latency_probe.sh
 */

#include "debug/latency_probe.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace epaper::latencyprobe;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static void test_fixture_scale()
{
    StubDocument doc;
    doc.fill();
    CHECK(doc.nodeCount() == kNodeCount);
    CHECK(doc.sampleCount() == kSampleCount);
    CHECK(int(doc.nodes().size()) == 500);
    CHECK(int(doc.samples().size()) == 50000);
}

static void test_known_hit_and_miss()
{
    StubDocument doc;
    doc.fill();
    const Sample hit = doc.knownHitPoint(0);
    CHECK(doc.hitTest(hit.x, hit.y) == 0);
    const Sample last = doc.knownHitPoint(kNodeCount - 1);
    CHECK(doc.hitTest(last.x, last.y) == kNodeCount - 1);
    CHECK(doc.hitTest(-1000.f, -1000.f) == -1);
}

static Percentiles bench_hit_test(StubDocument &doc, int queries)
{
    std::vector<std::int64_t> us;
    us.reserve(std::size_t(queries));
    std::uint32_t rng = 0xA5A5u;
    auto rnd = [&]() -> std::uint32_t {
        rng = rng * 1664525u + 1013904223u;
        return rng;
    };
    volatile int sink = 0;
    for (int i = 0; i < queries; ++i) {
        float x, y;
        const int mode = i % 4;
        if (mode == 0) {
            const Sample p = doc.knownHitPoint(i % kNodeCount);
            x = p.x;
            y = p.y;
        } else if (mode == 1) {
            const Sample p = doc.knownHitPoint(0); // bottom-most, reverse walk
            x = p.x;
            y = p.y;
        } else if (mode == 2) {
            x = -50.f;
            y = -50.f; // miss
        } else {
            x = float(rnd() % 1404);
            y = float(rnd() % 1872);
        }
        const auto t0 = std::chrono::steady_clock::now();
        sink += doc.hitTest(x, y);
        const auto t1 = std::chrono::steady_clock::now();
        us.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    (void)sink;
    return summarizeNs(us);
}

static void test_paint_loop_hygiene()
{
    Harness h;
    h.enable();
    h.enterPaint();
    // Simulated paint() only blits; it must not call onIngest.
    std::vector<Sample> ink;
    simulatePaintSegment(ink, 1, 1, 2, 2);
    h.leavePaint();
    CHECK(h.paintLoopHits() == 0);
    CHECK(h.samplesSeen() == 0);

    h.enterPaint();
    h.onIngest(10.f, 10.f, true); // would be a spec violation if paint() did this
    h.leavePaint();
    CHECK(h.paintLoopHits() == 1);
}

static void test_probe_never_drops()
{
    Harness h;
    h.enable();
    const int n = 250;
    for (int i = 0; i < n; ++i)
        h.onIngest(float(i), float(i), i % 17 == 0);
    CHECK(h.samplesSeen() == n);
    CHECK(h.samplesDropped() == 0);
}

int main()
{
    test_fixture_scale();
    test_known_hit_and_miss();
    test_paint_loop_hygiene();
    test_probe_never_drops();

    StubDocument doc;
    doc.fill();
    {
        volatile int warm = 0;
        for (int i = 0; i < 64; ++i)
            warm += doc.hitTest(float(i), float(i));
        (void)warm;
    }
    const Percentiles hit = bench_hit_test(doc, 2000);

    const IngestSimResult baseline = simulateIngest(200, 25, nullptr);

    Harness probe;
    probe.enable();
    const IngestSimResult withProbe = simulateIngest(200, 25, &probe);
    const Percentiles ingestHit = probe.hitPercentiles();

    std::cout << "=== STORY-EP-013 host latency probe ===\n";
    std::cout << "caveat: NOT device p95. Host (this machine) only.\n";
    std::cout << "SRS-EP-13 bars: pen-down→pixel p95 ≤30ms (device); hit-test p95 ≤100ms; 0 drops.\n";
    std::cout << "SRS-EP-01 device baseline (2026-08-09): arrival→flush p95=798us n=764\n";
    std::cout << "\n";
    std::cout << "fixture: nodes=" << doc.nodeCount() << " samples=" << doc.sampleCount() << "\n";
    std::cout << "hit-test (AABB+polyline, mixed hit/miss) n=" << hit.n << " p50=" << hit.p50Ns
              << "ns p95=" << hit.p95Ns << "ns p99=" << hit.p99Ns << "ns ("
              << nsToUs(hit.p95Ns) << "us p95)\n";
    std::cout << "ingest press→first-segment BASELINE n=" << baseline.pressToFirstNs.n
              << " p50=" << baseline.pressToFirstNs.p50Ns << "ns p95=" << baseline.pressToFirstNs.p95Ns
              << "ns p99=" << baseline.pressToFirstNs.p99Ns << "ns emitted=" << baseline.samplesEmitted
              << "\n";
    std::cout << "ingest press→first-segment PROBE    n=" << withProbe.pressToFirstNs.n
              << " p50=" << withProbe.pressToFirstNs.p50Ns << "ns p95=" << withProbe.pressToFirstNs.p95Ns
              << "ns p99=" << withProbe.pressToFirstNs.p99Ns << "ns emitted=" << withProbe.samplesEmitted
              << " dropped=" << withProbe.samplesDropped << "\n";
    std::cout << "ingest-path hit-test n=" << ingestHit.n << " p50=" << ingestHit.p50Ns
              << "ns p95=" << ingestHit.p95Ns << "ns p99=" << ingestHit.p99Ns << "ns\n";
    std::cout << probe.dumpText();

    const std::int64_t deltaP95 = withProbe.pressToFirstNs.p95Ns - baseline.pressToFirstNs.p95Ns;
    std::cout << "probe-baseline p95 delta=" << deltaP95 << "ns\n";

    CHECK(withProbe.samplesDropped == 0);
    CHECK(withProbe.samplesEmitted == baseline.samplesEmitted);
    CHECK(probe.paintLoopHits() == 0);
    CHECK(hit.p95Ns >= 0);
    CHECK(hit.p95Ns <= 100000000); // 100 ms — host must beat the bar; miss ⇒ CHL on any device

    if (g_fails) {
        std::cerr << g_fails << " check(s) failed\n";
        return 1;
    }
    std::cout << "latency_probe_test: checks passed (host only; device p95 not claimed)\n";
    return 0;
}
