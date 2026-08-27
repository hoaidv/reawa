#pragma once
/**
 * @implements [SRS-EP-07] create_smart_group
 * @implements [SRS-EP-10] reparent capture into Smart Group
 */

#include "doc_edit.hpp"

namespace epaper {
namespace document {

class CreateSmartGroupEdit final : public DocEdit {
public:
    CreateSmartGroupEdit() = default;

    const char *kind() const override { return edit_kind::kCreateSmartGroup; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override;

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setParentId(std::optional<std::string> p) { m_parentId = std::move(p); }
    void setBounds(SmartBounds b) { m_bounds = b; }
    void setTransform(SmartTransform t) { m_transform = t; }
    void setInkScaleMode(std::string m) { m_inkScaleMode = std::move(m); }
    void setCaptureIds(std::vector<std::string> ids) { m_captureIds = std::move(ids); }
    void setChildren(std::vector<DocNode> ch) { m_children = std::move(ch); }

    const std::string &nodeId() const { return m_nodeId; }
    const std::vector<std::string> &captureIds() const { return m_captureIds; }
    const std::vector<DocNode> &children() const { return m_children; }

    static std::unique_ptr<CreateSmartGroupEdit> fromPayload(const JsonValue &envelope,
                                                             const JsonValue &payload);

private:
    std::string m_nodeId;
    std::optional<std::string> m_parentId;
    SmartBounds m_bounds;
    SmartTransform m_transform;
    std::string m_inkScaleMode = "fixedInk";
    std::vector<std::string> m_captureIds;
    std::vector<DocNode> m_children;
};

} // namespace document
} // namespace epaper
