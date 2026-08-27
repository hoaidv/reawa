#pragma once
/**
 * @implements [SRS-EP-08] set_ink_samples
 */

#include "doc_edit.hpp"

namespace epaper {
namespace document {

class SetInkSamplesEdit final : public DocEdit {
public:
    SetInkSamplesEdit() = default;
    SetInkSamplesEdit(std::string nodeId, std::vector<InkSample> samples)
        : m_nodeId(std::move(nodeId))
        , m_samples(std::move(samples))
    {
    }

    const char *kind() const override { return edit_kind::kSetInkSamples; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_nodeId}; }

    void setNodeId(std::string id) { m_nodeId = std::move(id); }
    void setSamples(std::vector<InkSample> s) { m_samples = std::move(s); }
    const std::string &nodeId() const { return m_nodeId; }

    static std::unique_ptr<SetInkSamplesEdit> fromPayload(const JsonValue &envelope,
                                                          const JsonValue &payload);

private:
    std::string m_nodeId;
    std::vector<InkSample> m_samples;
};

} // namespace document
} // namespace epaper
