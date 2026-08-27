#pragma once
/**
 * @implements [SRS-EP-10] join membership
 */

#include "doc_edit.hpp"

namespace epaper {
namespace document {

class JoinSmartGroupEdit final : public DocEdit {
public:
    JoinSmartGroupEdit() = default;
    JoinSmartGroupEdit(std::string inkId, std::string smartGroupId)
        : m_inkId(std::move(inkId))
        , m_smartGroupId(std::move(smartGroupId))
    {
    }

    const char *kind() const override { return edit_kind::kJoinSmartGroup; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override { return {m_inkId}; }

    void setInkId(std::string id) { m_inkId = std::move(id); }
    void setSmartGroupId(std::string id) { m_smartGroupId = std::move(id); }
    const std::string &inkId() const { return m_inkId; }
    const std::string &smartGroupId() const { return m_smartGroupId; }

    static std::unique_ptr<JoinSmartGroupEdit> fromPayload(const JsonValue &envelope,
                                                           const JsonValue &payload);

private:
    std::string m_inkId;
    std::string m_smartGroupId;
};

} // namespace document
} // namespace epaper
