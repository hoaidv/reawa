/**
 * TransformSession host tests (no Qt). Geometry + commit moved/resized.
 */

#include "drawing/tools/transform_session.hpp"

#include <cmath>
#include <iostream>
#include <string>

using namespace epaper::tools;
using epaper::document::ResizeHandle;
using epaper::document::SmartBounds;
using epaper::document::SmartTransform;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static bool near(double a, double b, double eps = 1e-6)
{
    return std::abs(a - b) <= eps;
}

static void test_handle_from_index()
{
    CHECK(handleFromIndex(0) == ResizeHandle::Nw);
    CHECK(handleFromIndex(4) == ResizeHandle::Se);
    CHECK(handleFromIndex(-1) == ResizeHandle::None);
    CHECK(handleFromIndex(99) == ResizeHandle::None);
}

static void test_begin()
{
    TransformSession s;
    SmartTransform t;
    t.x = 10;
    t.y = 20;
    SmartBounds b;
    b.width = 40;
    b.height = 30;
    s.begin("n1", ResizeHandle::None, {100, 200}, t, b);
    CHECK(s.active);
    CHECK(s.nodeId == "n1");
    CHECK(near(s.originT.x, 10));
    CHECK(near(s.originT.y, 20));
    CHECK(near(s.originWorld.x, 100));
    CHECK(near(s.originWorld.y, 200));
}

static void test_apply_move()
{
    TransformSession s;
    SmartTransform t;
    t.x = 10;
    t.y = 20;
    SmartBounds b;
    b.x = 0;
    b.y = 0;
    b.width = 40;
    b.height = 30;
    s.begin("n1", ResizeHandle::None, {0, 0}, t, b);
    s.apply({15, 25}, "withBounds");
    CHECK(near(s.liveT.x, 25));
    CHECK(near(s.liveT.y, 45));
}

static void test_commit_no_move()
{
    TransformSession s;
    SmartTransform t;
    SmartBounds b;
    b.width = 10;
    b.height = 10;
    s.begin("n1", ResizeHandle::None, {0, 0}, t, b);
    s.apply({0.5, 0}, "withBounds"); // under 2.0 du
    const TransformResult r = s.commit();
    CHECK(!r.moved);
    CHECK(!r.resized);
    CHECK(s.active); // fields stay until gesture reset()
}

static void test_commit_move()
{
    TransformSession s;
    SmartTransform t;
    SmartBounds b;
    b.width = 10;
    b.height = 10;
    s.begin("n1", ResizeHandle::None, {0, 0}, t, b);
    s.apply({10, 0}, "withBounds");
    const TransformResult r = s.commit();
    CHECK(r.moved);
    CHECK(!r.resized);
}

static void test_reset_clears()
{
    TransformSession s;
    SmartTransform t;
    SmartBounds b;
    b.width = 10;
    b.height = 10;
    s.begin("n1", ResizeHandle::None, {0, 0}, t, b);
    s.reset();
    CHECK(!s.active);
    CHECK(s.nodeId.empty());
}

int main()
{
    test_handle_from_index();
    test_begin();
    test_apply_move();
    test_commit_no_move();
    test_commit_move();
    test_reset_clears();
    if (g_fails) {
        std::cerr << "transform_session_test: " << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "transform_session_test ok\n";
    return 0;
}
