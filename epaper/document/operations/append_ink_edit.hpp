#pragma once
/**
 * @implements [SRS-EP-07] append_ink
 */

#include "doc_edit.hpp"

namespace epaper {
namespace document {

class AppendInkEdit final : public DocEdit {
public:
    AppendInkEdit() = default;
    AppendInkEdit(std::string nodeId, std::vector<InkSample> samples, Style style = {})
        : m_nodeId(std::move(nodeId))
        , m_style(std::move(style))
        , m_samples(std::move(samples))
    {
    }

    const char *kind() const override { return edit_kind::kAppendInk; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setParentId(std::optional<std::string> p) { m_parentId = std::move(p); }
    void setStyle(Style s) { m_style = std::move(s); }
    void setRole(std::optional<std::string> r) { m_role = std::move(r); }
    void setSamples(std::vector<InkSample> s) { m_samples = std::move(s); }

    const std::string &nodeId() const { return m_nodeId; }
    const std::optional<std::string> &parentId() const { return m_parentId; }

    static std::unique_ptr<AppendInkEdit> fromPayload(const JsonValue &envelope,
                                                      const JsonValue &payload);

private:
    std::string m_nodeId;
    std::optional<std::string> m_parentId;
    Style m_style;
    std::optional<std::string> m_role;
    std::vector<InkSample> m_samples;
};

} // namespace document
} // namespace epaper
