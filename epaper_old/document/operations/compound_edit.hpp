#pragma once
/**
 * @implements [SRS-EP-08] compound of counterpart edits
 */

#include "doc_edit.hpp"
#include "edit_helpers.hpp"
#include "reparent_edit.hpp"

#include <algorithm>
#include <stdexcept>
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

inline void collectCapturedRestores(const DeviceDocument &doc, const std::vector<std::string> &ids,
                                    CompoundEdit &out, const std::string &forwardOpId)
{
    struct Cap {
        std::string id;
        DeviceDocument::NodePlace place;
        DocNode body;
    };
    std::vector<Cap> caps;
    auto consider = [&](const std::string &cid) {
        const DocNode *n = doc.find(cid);
        DeviceDocument::NodePlace pl;
        if (!n || !doc.findPlace(cid, &pl))
            return;
        for (const auto &c : caps) {
            if (c.id == cid)
                return;
        }
        caps.push_back({cid, pl, *n});
    };
    for (const auto &cid : ids)
        consider(cid);
    std::sort(caps.begin(), caps.end(),
              [](const Cap &a, const Cap &b) { return a.place.index < b.place.index; });
    for (const auto &c : caps) {
        auto r = std::make_unique<ReparentEdit>(
            ReparentEdit::restore(c.id, c.place.parentId, c.place.index, c.body));
        r->setId(forwardOpId);
        r->setUndo(true);
        out.addPart(std::move(r));
    }
}

inline ApplyResult CompoundEdit::doApply(DeviceDocument &doc)
{
    for (auto &p : m_parts) {
        if (!p)
            continue;
        const ApplyResult r = p->doApply(doc);
        if (!r.applied)
            throw std::runtime_error(r.reason.empty() ? "compound_part_failed" : r.reason);
    }
    return {true, {}};
}

inline std::unique_ptr<DocEdit> CompoundEdit::generateUndo(const DeviceDocument &doc) const
{
    auto u = std::make_unique<CompoundEdit>();
    u->setId(m_id);
    u->setUndo(true);
    for (auto it = m_parts.rbegin(); it != m_parts.rend(); ++it) {
        if (*it)
            u->addPart((*it)->generateUndo(doc));
    }
    return u;
}

inline JsonValue CompoundEdit::serialize() const
{
    JsonValue::Array ops;
    for (const auto &p : m_parts) {
        if (p)
            ops.push_back(p->serialize());
    }
    return envelope(JsonValue::object({{"ops", JsonValue::array(std::move(ops))}}));
}

inline std::unique_ptr<DocEdit> CompoundEdit::clone() const
{
    auto e = std::make_unique<CompoundEdit>();
    copyMetaTo(*e);
    for (const auto &p : m_parts) {
        if (p)
            e->addPart(p->clone());
    }
    return e;
}

inline std::vector<std::string> CompoundEdit::targets() const
{
    std::vector<std::string> ts;
    for (const auto &p : m_parts) {
        if (!p)
            continue;
        for (const auto &id : p->targets())
            addTargetId(ts, id);
    }
    return ts;
}

inline std::unique_ptr<CompoundEdit> CompoundEdit::fromPayload(const JsonValue &envelope,
                                                              const JsonValue &payload)
{
    auto e = std::make_unique<CompoundEdit>();
    fillMeta(*e, envelope);
    if (const JsonValue *ops = payload.get("ops"); ops && ops->isArray()) {
        for (const auto &j : ops->asArray())
            e->addPart(DocEdit::fromJson(j));
    }
    return e;
}

} // namespace document
} // namespace epaper
