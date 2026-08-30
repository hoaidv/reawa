/**
 * STORY-EP-065 / STORY-EP-066 — area polygon clip + object 80% table.
 */
#include "document/device_document.hpp"
#include "document/erase_area.hpp"
#include "document/erase_object.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace epaper::document;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static JsonValue makeInk(const std::string &opId, const std::string &id,
                         const std::vector<std::pair<double, double>> &xy)
{
    JsonValue::Array samples;
    for (const auto &p : xy) {
        JsonValue::Object s;
        s.emplace_back("x", JsonValue::number(p.first));
        s.emplace_back("y", JsonValue::number(p.second));
        samples.push_back(JsonValue::object(std::move(s)));
    }
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("samples", JsonValue::array(std::move(samples)));
    payload.emplace_back("style", parseJson(R"({"stroke":"#1C2430","strokeWidth":2})"));
    return opEnvelope(opId, "append_ink", JsonValue::object(std::move(payload)));
}

static JsonValue makePrimitive(const std::string &opId, const std::string &id, double x, double y,
                               double w, double h)
{
    JsonValue::Object geom;
    geom.emplace_back("kind", JsonValue::string("rect"));
    geom.emplace_back("x", JsonValue::number(x));
    geom.emplace_back("y", JsonValue::number(y));
    geom.emplace_back("w", JsonValue::number(w));
    geom.emplace_back("h", JsonValue::number(h));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("geom", JsonValue::object(std::move(geom)));
    payload.emplace_back("style", parseJson(R"({"stroke":"#1C2430","strokeWidth":2})"));
    return opEnvelope(opId, "create_primitive", JsonValue::object(std::move(payload)));
}

static JsonValue makeFrame(const std::string &opId, const std::string &id)
{
    JsonValue::Object b;
    b.emplace_back("minX", JsonValue::number(0));
    b.emplace_back("minY", JsonValue::number(0));
    b.emplace_back("maxX", JsonValue::number(40));
    b.emplace_back("maxY", JsonValue::number(40));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    return opEnvelope(opId, "create_frame", JsonValue::object(std::move(payload)));
}

static std::vector<ErasePt> boxPoly(double x0, double y0, double x1, double y1)
{
    return {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}, {x0, y0}};
}

static void test_area_polygon_clips_ink()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeInk("ink-1", "I1", {{0, 0}, {100, 0}})).applied);
    CHECK(commitAreaErase(doc, "area-1", boxPoly(40, -10, 60, 10)).applied);
    CHECK(doc.find("I1"));
    CHECK(doc.inkCount() == 2);
    CHECK(doc.undo().restored);
    CHECK(doc.inkCount() == 1);
}

static void test_area_primitive_fully_inside_removed()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makePrimitive("p-1", "P1", 0, 0, 10, 10)).applied);
    CHECK(commitAreaErase(doc, "area-p", boxPoly(-1, -1, 11, 11)).applied);
    CHECK(!doc.find("P1"));
    CHECK(doc.undo().restored);
    CHECK(doc.find("P1"));
}

static void test_area_frame_never_removed()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeFrame("f-1", "F1")).applied);
    CHECK(commitAreaErase(doc, "area-f", boxPoly(-10, -10, 50, 50)).applied);
    CHECK(doc.find("F1"));
}

static void test_area_smartgroup_fully_inside()
{
    DeviceDocument doc;
    JsonValue::Object b;
    b.emplace_back("x", JsonValue::number(0));
    b.emplace_back("y", JsonValue::number(0));
    b.emplace_back("width", JsonValue::number(20));
    b.emplace_back("height", JsonValue::number(20));
    JsonValue::Object xf;
    xf.emplace_back("x", JsonValue::number(0));
    xf.emplace_back("y", JsonValue::number(0));
    xf.emplace_back("rotation", JsonValue::number(0));
    xf.emplace_back("scaleX", JsonValue::number(1));
    xf.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Object child;
    child.emplace_back("id", JsonValue::string("B1"));
    child.emplace_back("kind", JsonValue::string("ink"));
    JsonValue::Array samples;
    JsonValue::Object a, z;
    a.emplace_back("x", JsonValue::number(0));
    a.emplace_back("y", JsonValue::number(0));
    z.emplace_back("x", JsonValue::number(20));
    z.emplace_back("y", JsonValue::number(0));
    samples.push_back(JsonValue::object(std::move(a)));
    samples.push_back(JsonValue::object(std::move(z)));
    child.emplace_back("samples", JsonValue::array(std::move(samples)));
    child.emplace_back("style", parseJson(R"({"stroke":"#1C2430","strokeWidth":2})"));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string("SG1"));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(xf)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("children", JsonValue::array({JsonValue::object(std::move(child))}));
    CHECK(doc.commitJson(opEnvelope("sg-1", "create_smart_group", JsonValue::object(std::move(payload))))
              .applied);
    CHECK(commitAreaErase(doc, "area-sg", boxPoly(-5, -5, 25, 25)).applied);
    CHECK(!doc.find("SG1"));
    CHECK(!doc.find("B1"));
}

static void test_object_ink_80_percent()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeInk("ink-1", "I1", {{0, 0}, {100, 0}})).applied);
    CHECK(commitObjectErase(doc, "obj-1", boxPoly(-1, -10, 90, 10)).applied);
    CHECK(!doc.find("I1"));
    CHECK(doc.undo().restored);
    CHECK(doc.find("I1"));
}

static void test_object_ink_below_80_left()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeInk("ink-1", "I1", {{0, 0}, {100, 0}})).applied);
    CHECK(commitObjectErase(doc, "obj-miss", boxPoly(-1, -10, 10, 10)).applied);
    CHECK(doc.find("I1"));
}

static void test_object_frame_never()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeFrame("f-1", "F1")).applied);
    CHECK(commitObjectErase(doc, "obj-f", boxPoly(-10, -10, 50, 50)).applied);
    CHECK(doc.find("F1"));
}

int main()
{
    test_area_polygon_clips_ink();
    test_area_primitive_fully_inside_removed();
    test_area_frame_never_removed();
    test_area_smartgroup_fully_inside();
    test_object_ink_80_percent();
    test_object_ink_below_80_left();
    test_object_frame_never();
    if (g_fails) {
        std::cerr << g_fails << " check(s) failed\n";
        return 1;
    }
    std::cout << "erase_area_object_test: checks passed\n";
    return 0;
}
