#pragma once
/**
 * @implements [SRS-EP-07] create_primitive
 */

#include "doc_edit.hpp"

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

} // namespace document
} // namespace epaper
