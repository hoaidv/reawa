#pragma once
/**
 * @implements [SRS-EP-07] create_primitive
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"
#include "remove_node_edit.hpp"

#include <stdexcept>

namespace epaper {
namespace document {

class CreatePrimitiveEdit final : public DocEdit {
public:
    CreatePrimitiveEdit() = default;

    const char *kind() const override { return edit_kind::kCreatePrimitive; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setParentId(std::optional<std::string> p) { m_parentId = std::move(p); }
    void setStyle(Style s) { m_style = std::move(s); }
    void setGeomKind(PrimitiveKind k) { m_geomKind = k; }
    void setLine(double x1, double y1, double x2, double y2)
    {
        m_geomKind = PrimitiveKind::Line;
        m_x1 = x1;
        m_y1 = y1;
        m_x2 = x2;
        m_y2 = y2;
    }
    void setRect(double x, double y, double w, double h)
    {
        m_geomKind = PrimitiveKind::Rect;
        m_gx = x;
        m_gy = y;
        m_gw = w;
        m_gh = h;
    }
    void setEllipse(double cx, double cy, double rx, double ry)
    {
        m_geomKind = PrimitiveKind::Ellipse;
        m_cx = cx;
        m_cy = cy;
        m_rx = rx;
        m_ry = ry;
    }
    const std::string &nodeId() const { return m_nodeId; }

    static std::unique_ptr<CreatePrimitiveEdit> fromPayload(const JsonValue &envelope,
                                                            const JsonValue &payload);

private:
    std::string m_nodeId;
    std::optional<std::string> m_parentId;
    Style m_style;
    PrimitiveKind m_geomKind = PrimitiveKind::Rect;
    double m_x1 = 0, m_y1 = 0, m_x2 = 0, m_y2 = 0;
    double m_gx = 0, m_gy = 0, m_gw = 0, m_gh = 0;
    double m_cx = 0, m_cy = 0, m_rx = 0, m_ry = 0;
};

inline ApplyResult CreatePrimitiveEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    doc.requireUnique(m_nodeId);
    DocNode n;
    n.id = m_nodeId;
    n.kind = NodeKind::Primitive;
    n.style = m_style;
    n.geomKind = m_geomKind;
    n.x1 = m_x1;
    n.y1 = m_y1;
    n.x2 = m_x2;
    n.y2 = m_y2;
    n.gx = m_gx;
    n.gy = m_gy;
    n.gw = m_gw;
    n.gh = m_gh;
    n.cx = m_cx;
    n.cy = m_cy;
    n.rx = m_rx;
    n.ry = m_ry;
    doc.insertUnder(m_parentId, std::move(n));
    return {true, {}};
}

inline std::unique_ptr<DocEdit> CreatePrimitiveEdit::generateUndo(const DeviceDocument &) const
{
    return makeRemoveEdit(m_id, m_nodeId);
}

inline JsonValue CreatePrimitiveEdit::serialize() const
{
    JsonValue::Object geom;
    if (m_geomKind == PrimitiveKind::Line) {
        geom.emplace_back("kind", JsonValue::string("line"));
        geom.emplace_back("x1", JsonValue::number(m_x1));
        geom.emplace_back("y1", JsonValue::number(m_y1));
        geom.emplace_back("x2", JsonValue::number(m_x2));
        geom.emplace_back("y2", JsonValue::number(m_y2));
    } else if (m_geomKind == PrimitiveKind::Ellipse) {
        geom.emplace_back("kind", JsonValue::string("ellipse"));
        geom.emplace_back("cx", JsonValue::number(m_cx));
        geom.emplace_back("cy", JsonValue::number(m_cy));
        geom.emplace_back("rx", JsonValue::number(m_rx));
        geom.emplace_back("ry", JsonValue::number(m_ry));
    } else {
        geom.emplace_back("kind", JsonValue::string("rect"));
        geom.emplace_back("x", JsonValue::number(m_gx));
        geom.emplace_back("y", JsonValue::number(m_gy));
        geom.emplace_back("w", JsonValue::number(m_gw));
        geom.emplace_back("h", JsonValue::number(m_gh));
    }
    JsonValue::Object p;
    p.emplace_back("id", JsonValue::string(m_nodeId));
    if (m_parentId)
        p.emplace_back("parentId", JsonValue::string(*m_parentId));
    p.emplace_back("style", styleToJson(m_style));
    p.emplace_back("geom", JsonValue::object(std::move(geom)));
    return envelope(JsonValue::object(std::move(p)));
}

inline std::unique_ptr<DocEdit> CreatePrimitiveEdit::clone() const
{
    return std::make_unique<CreatePrimitiveEdit>(*this);
}

inline std::unique_ptr<CreatePrimitiveEdit>
CreatePrimitiveEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<CreatePrimitiveEdit>();
    fillMeta(*e, envelope);
    e->setNodeId(payload.getString("id"));
    e->setParentId(parentIdFromJson(payload));
    e->setStyle(styleFromJson(payload.get("style")));
    const JsonValue *geom = payload.get("geom");
    if (!geom || !geom->isObject())
        return e;
    const std::string gk = geom->getString("kind");
    if (gk == "line")
        e->setLine(geom->getNumber("x1"), geom->getNumber("y1"), geom->getNumber("x2"),
                   geom->getNumber("y2"));
    else if (gk == "ellipse")
        e->setEllipse(geom->getNumber("cx"), geom->getNumber("cy"), geom->getNumber("rx"),
                      geom->getNumber("ry"));
    else
        e->setRect(geom->getNumber("x"), geom->getNumber("y"), geom->getNumber("w"),
                   geom->getNumber("h"));
    return e;
}

} // namespace document
} // namespace epaper
