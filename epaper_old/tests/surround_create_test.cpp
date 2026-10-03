/**
 * Host tests for STORY-EP-018 / [SRS-EP-10] selection-create surround.
 */

#include "document/device_document.hpp"
#include "document/surround_create.hpp"

#include <algorithm>
#include <cmath>
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

static std::vector<InkSample> box(double x, double y, double w, double h)
{
    std::vector<InkSample> s;
    auto add = [&](double px, double py) {
        InkSample p;
        p.x = px;
        p.y = py;
        p.t = static_cast<double>(s.size());
        s.push_back(p);
    };
    add(x, y);
    add(x + w, y);
    add(x + w, y + h);
    add(x, y + h);
    add(x, y);
    return s;
}

static void appendInk(DeviceDocument &doc, const std::string &id, const std::vector<InkSample> &samples)
{
    JsonValue::Array arr;
    for (const auto &s : samples)
        arr.push_back(sampleToJson(s));
    JsonValue::Object style;
    style.emplace_back("stroke", JsonValue::string("#000"));
    style.emplace_back("strokeWidth", JsonValue::number(2));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("samples", JsonValue::array(std::move(arr)));
    payload.emplace_back("style", JsonValue::object(std::move(style)));
        CHECK(doc.commitJson(opEnvelope("append_ink:" + id, "append_ink", JsonValue::object(std::move(payload)))).applied);
}

int main()
{
    {
        DeviceDocument doc;
        appendInk(doc, "outer", box(0, 0, 100, 100));
        appendInk(doc, "inner",
                  [] {
                      std::vector<InkSample> s;
                      for (auto [x, y] : {std::pair{40.0, 40.0}, {50.0, 45.0}, {45.0, 55.0}}) {
                          InkSample p;
                          p.x = x;
                          p.y = y;
                          s.push_back(p);
                      }
                      return s;
                  }());
        const auto r = createSmartGroupFromSelection(doc, {"outer", "inner"});
        CHECK(r.created);
        CHECK(r.boundaryId == "outer");
        const DocNode *sg = doc.find(r.smartGroupId);
        CHECK(sg && sg->kind == NodeKind::SmartGroup);
        CHECK(sg->boundaryPolyline.size() >= 2);
        CHECK(std::abs(sg->smartBounds.width - 100) < 1e-6);
        bool innerContent = false;
        for (const auto &c : sg->children) {
            if (c.id == "inner") {
                CHECK(c.role && *c.role == "content");
                CHECK(c.layoutOffset.has_value());
                innerContent = true;
            }
            if (c.id == "outer")
                CHECK(c.role && *c.role == "boundary");
        }
        CHECK(innerContent);
    }
    {
        DeviceDocument doc;
        appendInk(doc, "a", [] {
            std::vector<InkSample> s;
            InkSample a, b;
            a.x = 0;
            a.y = 0;
            b.x = 10;
            b.y = 0;
            s.push_back(a);
            s.push_back(b);
            return s;
        }());
        appendInk(doc, "b", [] {
            std::vector<InkSample> s;
            InkSample a, b;
            a.x = 50;
            a.y = 50;
            b.x = 60;
            b.y = 50;
            s.push_back(a);
            s.push_back(b);
            return s;
        }());
        const std::string before = doc.snapshotString();
        const auto r = createSmartGroupFromSelection(doc, {"a", "b"});
        CHECK(!r.created);
        CHECK(r.reason == "no_surround");
        CHECK(doc.snapshotString() == before);
    }
    {
        DeviceDocument doc;
        appendInk(doc, "earlyBox", box(10, 10, 15, 15));
        appendInk(doc, "lateBox", box(0, 0, 100, 100));
        appendInk(doc, "inner", [] {
            std::vector<InkSample> s;
            InkSample a, b;
            a.x = 40;
            a.y = 40;
            b.x = 50;
            b.y = 50;
            s.push_back(a);
            s.push_back(b);
            return s;
        }());
        const auto r = createSmartGroupFromSelection(doc, {"earlyBox", "lateBox", "inner"});
        CHECK(r.created);
        CHECK(r.boundaryId == "lateBox");
    }
    {
        DeviceDocument doc;
        appendInk(doc, "outer", box(0, 0, 80, 80));
        appendInk(doc, "inner", [] {
            std::vector<InkSample> s;
            InkSample a, b;
            a.x = 20;
            a.y = 20;
            b.x = 30;
            b.y = 30;
            s.push_back(a);
            s.push_back(b);
            return s;
        }());
        const std::string before = doc.snapshotString();
        CHECK(createSmartGroupFromSelection(doc, {"outer", "inner"}).created);
        CHECK(doc.undo().restored);
        CHECK(doc.snapshotString() == before);
    }
    {
        DeviceDocument doc;
        appendInk(doc, "outer", box(0, 0, 100, 100));
        appendInk(doc, "inner", [] {
            std::vector<InkSample> s;
            InkSample a, b;
            a.x = 40;
            a.y = 40;
            b.x = 50;
            b.y = 50;
            s.push_back(a);
            s.push_back(b);
            return s;
        }());
        JsonValue::Object b, t, payload;
        b.emplace_back("x", JsonValue::number(0));
        b.emplace_back("y", JsonValue::number(0));
        b.emplace_back("width", JsonValue::number(10));
        b.emplace_back("height", JsonValue::number(10));
        t.emplace_back("x", JsonValue::number(200));
        t.emplace_back("y", JsonValue::number(200));
        t.emplace_back("rotation", JsonValue::number(0));
        t.emplace_back("scaleX", JsonValue::number(1));
        t.emplace_back("scaleY", JsonValue::number(1));
        payload.emplace_back("id", JsonValue::string("sg_existing"));
        payload.emplace_back("bounds", JsonValue::object(std::move(b)));
        payload.emplace_back("transform", JsonValue::object(std::move(t)));
        payload.emplace_back("children", JsonValue::array({}));
                CHECK(doc.commitJson(opEnvelope("create_smart_group:sg_existing", "create_smart_group", JsonValue::object(std::move(payload)))).applied);
        const auto r = createSmartGroupFromSelection(doc, {"outer", "inner", "sg_existing"});
        CHECK(r.created);
        const DocNode *sg = doc.find(r.smartGroupId);
        CHECK(sg);
        bool nested = false;
        for (const auto &c : sg->children) {
            if (c.id == "sg_existing" && c.kind == NodeKind::SmartGroup)
                nested = true;
        }
        CHECK(nested);
        CHECK(doc.find("sg_existing"));
    }
    {
        DeviceDocument doc;
        appendInk(doc, "in", box(10, 10, 5, 5));
        appendInk(doc, "out", box(80, 80, 5, 5));
        SmartBounds rect;
        rect.x = 0;
        rect.y = 0;
        rect.width = 40;
        rect.height = 40;
        const auto ids = selectByRect(doc, rect);
        CHECK(std::find(ids.begin(), ids.end(), "in") != ids.end());
        CHECK(std::find(ids.begin(), ids.end(), "out") == ids.end());
    }
    {
        DeviceDocument doc;
        std::vector<InkSample> graze;
        for (int i = 0; i < 20; ++i) {
            InkSample p;
            p.x = double(i) * 5.0;
            p.y = 20;
            p.t = double(i);
            graze.push_back(p);
        }
        appendInk(doc, "graze", graze);
        SmartBounds rect;
        rect.x = 0;
        rect.y = 0;
        rect.width = 12;
        rect.height = 40;
        const auto ids = selectByRect(doc, rect);
        CHECK(std::find(ids.begin(), ids.end(), "graze") == ids.end());
    }
    {
        DeviceDocument doc;
        appendInk(doc, "in", box(20, 20, 4, 4));
        appendInk(doc, "out", box(90, 20, 4, 4));
        const auto poly = box(0, 0, 50, 50);
        const auto ids = selectByFreeform(doc, poly);
        CHECK(std::find(ids.begin(), ids.end(), "in") != ids.end());
        CHECK(std::find(ids.begin(), ids.end(), "out") == ids.end());
    }
    {
        CHECK(pointInPolygonEvenOdd(50, 50, closedPathForTest(box(0, 0, 100, 100))));
        auto open = box(0, 0, 100, 100);
        open.pop_back();
        const auto stored = open;
        CHECK(qualifiesAsSurround(
            [&] {
                DocNode n;
                n.kind = NodeKind::Ink;
                n.samples = stored;
                return n;
            }(),
            {}));
        CHECK(stored.size() == open.size());
    }
    {
        DeviceDocument doc;
        DocNode a;
        a.id = "A";
        a.kind = NodeKind::SmartGroup;
        a.smartBounds = {0, 0, 80, 80};
        a.transform = {0, 0, 0, 1, 1};
        DocNode b;
        b.id = "B";
        b.kind = NodeKind::SmartGroup;
        b.smartBounds = {0, 0, 80, 80};
        b.transform = {300, 0, 0, 1, 1};
        DocNode conn;
        conn.id = "C";
        conn.kind = NodeKind::Connector;
        conn.fromNodeId = "A";
        conn.toNodeId = "B";
        for (int i = 0; i < 20; ++i) {
            ConnectorRestPt p;
            p.x = 80.0 + double(i) * 11.5;
            p.y = 40.0;
            conn.warpedSamples.push_back(p);
        }
        doc.rootChildren.push_back(std::move(a));
        doc.rootChildren.push_back(std::move(b));
        doc.rootChildren.push_back(std::move(conn));

        const auto ids = selectByFreeform(doc, box(0, 0, 400, 120));
        CHECK(std::find(ids.begin(), ids.end(), "A") != ids.end());
        CHECK(std::find(ids.begin(), ids.end(), "B") != ids.end());
        CHECK(std::find(ids.begin(), ids.end(), "C") != ids.end());

        SmartBounds graze;
        graze.x = 90;
        graze.y = 30;
        graze.width = 20;
        graze.height = 20;
        const auto grazeIds = selectByRect(doc, graze);
        CHECK(std::find(grazeIds.begin(), grazeIds.end(), "C") == grazeIds.end());

        SmartBounds boxOnly;
        CHECK(nodeInvalidateAabb(doc, "A", boxOnly));
        SmartBounds hist;
        CHECK(unionHistoryRestoreAabb(doc, {"A"}, hist));
        CHECK(hist.x + hist.width >= 80.0 + 19.0 * 11.5 - 1.0);
        CHECK(boxOnly.x + boxOnly.width < 80.0 + 19.0 * 11.5 - 1.0);

        CHECK(connectorStrokeHits(*doc.find("C"), 80.0 + 10 * 11.5, 40.0, 4.0));
        CHECK(!connectorStrokeHits(*doc.find("C"), 180.0, 120.0, 4.0));
    }

    if (g_fails) {
        std::cerr << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "surround_create_test OK\n";
    return 0;
}
