#pragma once
/**
 * Replace ConnectorAnchor.styleInk on one end.
 * @implements [SRS-EP-35] set_endpoint_ink
 * @implements [ADR-0038] face-frame list; inverse restores previous list
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"

#include <stdexcept>
#include <utility>

namespace epaper {
namespace document {

class SetEndpointInkEdit final : public DocEdit {
public:
    SetEndpointInkEdit() = default;
    SetEndpointInkEdit(std::string connectorId, bool fromEnd,
                       std::vector<EndpointInkStroke> strokes)
        : m_connectorId(std::move(connectorId))
        , m_fromEnd(fromEnd)
        , m_strokes(std::move(strokes))
    {
    }

    const char *kind() const override { return edit_kind::kSetEndpointInk; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_connectorId}; }

    void setConnectorId(std::string id) { m_connectorId = std::move(id); }
    void setFromEnd(bool v) { m_fromEnd = v; }
    void setStrokes(std::vector<EndpointInkStroke> s) { m_strokes = std::move(s); }

    static std::unique_ptr<SetEndpointInkEdit> fromPayload(const JsonValue &envelope,
                                                           const JsonValue &payload);

private:
    std::string m_connectorId;
    bool m_fromEnd = true;
    std::vector<EndpointInkStroke> m_strokes;
};

inline ApplyResult SetEndpointInkEdit::doApply(DeviceDocument &doc)
{
    if (m_connectorId.empty())
        throw std::runtime_error("missing_id");
    DocNode *n = doc.mutableFind(m_connectorId);
    if (!n || n->kind != NodeKind::Connector)
        throw std::runtime_error(std::string("not_connector:") + m_connectorId);
    if (m_fromEnd)
        n->fromAnchor.styleInk = m_strokes;
    else
        n->toAnchor.styleInk = m_strokes;
    return {true, {}};
}

inline std::unique_ptr<DocEdit> SetEndpointInkEdit::generateUndo(const DeviceDocument &doc) const
{
    auto u = std::make_unique<SetEndpointInkEdit>();
    u->setId(m_id);
    u->setUndo(true);
    u->setConnectorId(m_connectorId);
    u->setFromEnd(m_fromEnd);
    const DocNode *n = doc.find(m_connectorId);
    if (n && n->kind == NodeKind::Connector)
        u->setStrokes(m_fromEnd ? n->fromAnchor.styleInk : n->toAnchor.styleInk);
    return u;
}

inline JsonValue SetEndpointInkEdit::serialize() const
{
    return envelope(JsonValue::object({
        {"id", JsonValue::string(m_connectorId)},
        {"end", JsonValue::string(m_fromEnd ? "from" : "to")},
        {"styleInk", styleInkToJson(m_strokes)},
    }));
}

inline std::unique_ptr<DocEdit> SetEndpointInkEdit::clone() const
{
    return std::make_unique<SetEndpointInkEdit>(*this);
}

inline std::unique_ptr<SetEndpointInkEdit>
SetEndpointInkEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    const std::string end = payload.getString("end", "from");
    auto e = std::make_unique<SetEndpointInkEdit>(payload.getString("id"), end != "to",
                                                  styleInkFromJson(payload.get("styleInk")));
    fillMeta(*e, envelope);
    return e;
}

} // namespace document
} // namespace epaper
