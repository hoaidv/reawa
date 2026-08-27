#pragma once
/**
 * Stores absolute from/to pose so live preview cannot poison the inverse.
 * @implements [SRS-EP-07] set_smart_transform inverse is stored old pose
 * @implements [SRS-EP-11] move / resize commit
 */

#include "doc_edit.hpp"

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

} // namespace document
} // namespace epaper
