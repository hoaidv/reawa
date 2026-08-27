#pragma once
/**
 * @implements [SRS-EP-11] set_ink_scale_mode
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"

#include <stdexcept>

namespace epaper {
namespace document {

class SetInkScaleModeEdit final : public DocEdit {
public:
    SetInkScaleModeEdit() = default;
    SetInkScaleModeEdit(std::string nodeId, std::string mode)
        : m_nodeId(std::move(nodeId))
        , m_mode(std::move(mode))
    {
    }

    const char *kind() const override { return edit_kind::kSetInkScaleMode; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setMode(std::string m) { m_mode = std::move(m); }
    void setOldMode(std::string mode)
    {
        m_oldMode = std::move(mode);
        m_hasOld = true;
    }
    const std::string &nodeId() const { return m_nodeId; }

    static std::unique_ptr<SetInkScaleModeEdit> fromPayload(const JsonValue &envelope,
                                                            const JsonValue &payload);

private:
    std::string m_nodeId;
    std::string m_mode;
    std::string m_oldMode;
    bool m_hasOld = false;
};

inline ApplyResult SetInkScaleModeEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    DocNode *node = doc.mutableFind(m_nodeId);
    if (!node || node->kind != NodeKind::SmartGroup)
        throw std::runtime_error(std::string("not_smart_group:") + m_nodeId);
    if (m_mode != "withBounds" && m_mode != "fixedInk")
        throw std::runtime_error(std::string("bad_ink_scale_mode:") + m_mode);
    node->inkScaleMode = m_mode;
    return {true, {}};
}

inline std::unique_ptr<DocEdit> SetInkScaleModeEdit::generateUndo(const DeviceDocument &doc) const
{
    auto u = std::make_unique<SetInkScaleModeEdit>();
    u->setId(m_id);
    u->setUndo(true);
    const DocNode *n = doc.find(m_nodeId);
    const std::string old = m_hasOld ? m_oldMode : (n ? n->inkScaleMode : std::string("fixedInk"));
    u->setNodeId(m_nodeId);
    u->setMode(old);
    return u;
}

inline JsonValue SetInkScaleModeEdit::serialize() const
{
    return envelope(JsonValue::object({
        {"id", JsonValue::string(m_nodeId)},
        {"inkScaleMode", JsonValue::string(m_mode)},
    }));
}

inline std::unique_ptr<DocEdit> SetInkScaleModeEdit::clone() const
{
    return std::make_unique<SetInkScaleModeEdit>(*this);
}

inline std::unique_ptr<SetInkScaleModeEdit>
SetInkScaleModeEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<SetInkScaleModeEdit>(payload.getString("id"),
                                                   payload.getString("inkScaleMode"));
    fillMeta(*e, envelope);
    return e;
}

} // namespace document
} // namespace epaper
