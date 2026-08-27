#pragma once
/**
 * @implements [SRS-EP-08] set_ink_samples
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"

#include <stdexcept>

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

inline ApplyResult SetInkSamplesEdit::doApply(DeviceDocument &doc)
{
    if (m_nodeId.empty())
        throw std::runtime_error("missing_id");
    DocNode *n = doc.mutableFind(m_nodeId);
    if (!n || n->kind != NodeKind::Ink)
        throw std::runtime_error(std::string("not_ink:") + m_nodeId);
    n->samples = m_samples;
    return {true, {}};
}

inline std::unique_ptr<DocEdit> SetInkSamplesEdit::generateUndo(const DeviceDocument &doc) const
{
    auto u = std::make_unique<SetInkSamplesEdit>();
    u->setId(m_id);
    u->setUndo(true);
    u->setNodeId(m_nodeId);
    const DocNode *n = doc.find(m_nodeId);
    if (n)
        u->setSamples(n->samples);
    return u;
}

inline JsonValue SetInkSamplesEdit::serialize() const
{
    return envelope(JsonValue::object({
        {"id", JsonValue::string(m_nodeId)},
        {"samples", samplesToJsonArray(m_samples)},
    }));
}

inline std::unique_ptr<DocEdit> SetInkSamplesEdit::clone() const
{
    return std::make_unique<SetInkSamplesEdit>(*this);
}

inline std::unique_ptr<SetInkSamplesEdit>
SetInkSamplesEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<SetInkSamplesEdit>(payload.getString("id"),
                                                 samplesFromJsonArray(payload.get("samples")));
    fillMeta(*e, envelope);
    return e;
}

} // namespace document
} // namespace epaper
