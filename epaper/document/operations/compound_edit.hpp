#pragma once
/**
 * @implements [SRS-EP-08] compound of counterpart edits
 */

#include "doc_edit.hpp"

#include <vector>

namespace epaper {
namespace document {

class CompoundEdit final : public DocEdit {
public:
    const char *kind() const override { return edit_kind::kCompound; }
    ApplyResult doApply(DeviceDocument &doc) override;
    std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const override;
    JsonValue serialize() const override;
    std::unique_ptr<DocEdit> clone() const override;
    std::vector<std::string> targets() const override;

    void addPart(std::unique_ptr<DocEdit> part) { m_parts.push_back(std::move(part)); }
    const std::vector<std::unique_ptr<DocEdit>> &parts() const { return m_parts; }
    std::vector<std::unique_ptr<DocEdit>> takeParts() { return std::move(m_parts); }

    static std::unique_ptr<CompoundEdit> fromPayload(const JsonValue &envelope,
                                                     const JsonValue &payload);

private:
    std::vector<std::unique_ptr<DocEdit>> m_parts;
};

} // namespace document
} // namespace epaper
