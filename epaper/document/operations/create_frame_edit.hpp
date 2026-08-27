#pragma once
/**
 * @implements [SRS-EP-07] create_frame
 */

#include "doc_edit.hpp"

namespace epaper {
namespace document {

class CreateFrameEdit final : public DocEdit {
public:
    CreateFrameEdit() = default;
    CreateFrameEdit(std::string nodeId, Aabb bounds)
        : m_nodeId(std::move(nodeId))
        , m_bounds(bounds)
    {
    }

    const char *kind() const override { return edit_kind::kCreateFrame; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setBounds(Aabb b) { m_bounds = b; }
    const std::string &nodeId() const { return m_nodeId; }

    static std::unique_ptr<CreateFrameEdit> fromPayload(const JsonValue &envelope,
                                                        const JsonValue &payload);

private:
    std::string m_nodeId;
    Aabb m_bounds;
};

} // namespace document
} // namespace epaper
