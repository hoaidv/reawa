#pragma once
/**
 * @implements [SRS-EP-11] set_ink_scale_mode
 */

#include "doc_edit.hpp"

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

} // namespace document
} // namespace epaper
