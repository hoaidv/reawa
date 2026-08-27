#pragma once
/**
 * @implements [SRS-EP-10] join membership
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"
#include "reparent_edit.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

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

inline ApplyResult JoinSmartGroupEdit::doApply(DeviceDocument &doc)
{
    DocNode *sg = doc.mutableFind(m_smartGroupId);
    if (!sg || sg->kind != NodeKind::SmartGroup)
        throw std::runtime_error(std::string("not_smart_group:") + m_smartGroupId);
    DocNode detached;
    if (!doc.detachInk(m_inkId, &detached))
        throw std::runtime_error(std::string("join_missing:") + m_inkId);
    const SmartTransform &t = sg->transform;
    const bool fixedInk = sg->inkScaleMode == "fixedInk";
    const double sx = t.scaleX != 0 ? t.scaleX : 1.0;
    const double sy = t.scaleY != 0 ? t.scaleY : 1.0;
    for (auto &s : detached.samples) {
        if (fixedInk) {
            s.x = s.x - t.x;
            s.y = s.y - t.y;
        } else {
            s.x = (s.x - t.x) / sx;
            s.y = (s.y - t.y) / sy;
        }
    }
    detached.role = "content";
    const double w = sg->smartBounds.width != 0 ? sg->smartBounds.width : 1.0;
    const double h = sg->smartBounds.height != 0 ? sg->smartBounds.height : 1.0;
    double minX = std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    for (const auto &s : detached.samples) {
        minX = std::min(minX, s.x);
        minY = std::min(minY, s.y);
        maxX = std::max(maxX, s.x);
        maxY = std::max(maxY, s.y);
    }
    const double cx = std::isfinite(minX) ? (minX + maxX) / 2.0 : 0.0;
    const double cy = std::isfinite(minY) ? (minY + maxY) / 2.0 : 0.0;
    detached.layoutOffset = {(cx - sg->smartBounds.x) / w, (cy - sg->smartBounds.y) / h};
    sg->children.push_back(std::move(detached));
    return {true, {}};
}

inline std::unique_ptr<DocEdit> JoinSmartGroupEdit::generateUndo(const DeviceDocument &doc) const
{
    auto e = makeRestoreEdit(doc, m_inkId, m_id);
    if (e)
        return e;
    return clone();
}

inline JsonValue JoinSmartGroupEdit::serialize() const
{
    return envelope(JsonValue::object({
        {"inkId", JsonValue::string(m_inkId)},
        {"smartGroupId", JsonValue::string(m_smartGroupId)},
    }));
}

inline std::unique_ptr<DocEdit> JoinSmartGroupEdit::clone() const
{
    return std::make_unique<JoinSmartGroupEdit>(*this);
}

inline std::unique_ptr<JoinSmartGroupEdit>
JoinSmartGroupEdit::fromPayload(const JsonValue &envelope, const JsonValue &payload)
{
    auto e = std::make_unique<JoinSmartGroupEdit>(payload.getString("inkId"),
                                                  payload.getString("smartGroupId"));
    fillMeta(*e, envelope);
    return e;
}

} // namespace document
} // namespace epaper
