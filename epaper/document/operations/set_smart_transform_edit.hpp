#pragma once
/**
 * Stores absolute from/to pose so live preview cannot poison the inverse.
 * @implements [SRS-EP-07] set_smart_transform inverse is stored old pose
 * @implements [SRS-EP-11] move / resize commit
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"

#include <stdexcept>

namespace epaper {
namespace document {

class SetSmartTransformEdit final : public DocEdit {
public:
    SetSmartTransformEdit() = default;

    SetSmartTransformEdit(std::string opId, std::string nodeId, const SmartTransform &fromT,
                          const SmartBounds &fromB, const SmartTransform &toT,
                          const SmartBounds &toB, bool applyBounds)
        : m_nodeId(std::move(nodeId))
        , m_toT(toT)
        , m_toB(toB)
        , m_hasToBounds(applyBounds)
        , m_fromT(fromT)
        , m_fromB(fromB)
        , m_hasFrom(true)
    {
        setId(std::move(opId));
    }

    const char *kind() const override { return edit_kind::kSetSmartTransform; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setTo(const SmartTransform &t, const SmartBounds *bounds)
    {
        m_toT = t;
        if (bounds) {
            m_toB = *bounds;
            m_hasToBounds = true;
        }
    }
    void setFrom(const SmartTransform &t, const SmartBounds &b)
    {
        m_fromT = t;
        m_fromB = b;
        m_hasFrom = true;
    }
    bool hasFrom() const { return m_hasFrom; }
    const std::string &nodeId() const { return m_nodeId; }
    const SmartTransform &fromT() const { return m_fromT; }
    const SmartBounds &fromB() const { return m_fromB; }
    const SmartTransform &toT() const { return m_toT; }

    static std::unique_ptr<SetSmartTransformEdit> fromPayload(const JsonValue &envelope,
                                                              const JsonValue &payload);

private:
    std::string m_nodeId;
    SmartTransform m_toT;
    SmartBounds m_toB;
    bool m_hasToBounds = false;
    SmartTransform m_fromT;
    SmartBounds m_fromB;
    bool m_hasFrom = false;
};

inline std::unique_ptr<SetSmartTransformEdit>
SetSmartTransformEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<SetSmartTransformEdit>();
    fillMeta(*e, envelope);
    e->setNodeId(payload.getString("id"));
    if (const JsonValue *t = payload.get("transform"))
        e->setTo(transformFromJson(t), nullptr);
    if (const JsonValue *b = payload.get("bounds"); b && b->isObject()) {
        const SmartBounds bb = boundsFromJson(b);
        e->setTo(e->toT(), &bb);
    }
    return e;
}

inline ApplyResult SetSmartTransformEdit::doApply(DeviceDocument &doc)
{
    DocNode *node = doc.mutableFind(m_nodeId);
    if (!node || node->kind != NodeKind::SmartGroup)
        throw std::runtime_error(std::string("not_smart_group:") + m_nodeId);
    node->transform = m_toT;
    if (m_hasToBounds)
        node->smartBounds = m_toB;
    return {true, {}};
}

inline std::unique_ptr<DocEdit> SetSmartTransformEdit::generateUndo(const DeviceDocument &doc) const
{
    auto u = std::make_unique<SetSmartTransformEdit>();
    u->setId(m_id);
    u->setUndo(true);
    u->setNodeId(m_nodeId);
    if (m_hasFrom) {
        u->setTo(m_fromT, &m_fromB);
        u->setFrom(m_toT, m_toB);
        return u;
    }
    const DocNode *n = doc.find(m_nodeId);
    if (n)
        u->setTo(n->transform, &n->smartBounds);
    return u;
}

inline JsonValue SetSmartTransformEdit::serialize() const
{
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(m_nodeId));
    payload.emplace_back("transform", transformToJson(m_toT));
    if (m_hasToBounds)
        payload.emplace_back("bounds", boundsToJson(m_toB));
    return envelope(JsonValue::object(std::move(payload)));
}

inline std::unique_ptr<DocEdit> SetSmartTransformEdit::clone() const
{
    return std::make_unique<SetSmartTransformEdit>(*this);
}

} // namespace document
} // namespace epaper
