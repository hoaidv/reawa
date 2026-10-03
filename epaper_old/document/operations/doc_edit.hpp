#pragma once
/**
 * Typed document edit: apply, inverse, clone. JSON only at the wire edge.
 * @implements [SRS-EP-07] inverse-op undo as counterpart edits
 *
 * serialize()/fromJson() exist for the publish queue and fixture JSON.
 * targets() lists nodes this edit versions — F20 lastOpId capture/skip needs
 * that set before apply, which generateUndo() cannot supply (it is the inverse,
 * not the forward node set). collectSerialized() is gone: the undo ring stores
 * DocEdit clones; CompoundEdit::parts() is the flatten.
 */

#include "../doc_model.hpp"
#include "edit_kinds.hpp"

#include <memory>
#include <string>
#include <vector>

namespace epaper {
namespace document {

class DeviceDocument;

/**
 * One structural mutation. Tools construct a subclass and send it to DocContext.
 * @implements [SRS-EP-07] DocEdit apply / generateUndo / serialize
 */
class DocEdit {
public:
    virtual ~DocEdit() = default;

    const std::string &id() const { return m_id; }
    void setId(std::string id) { m_id = std::move(id); }

    const std::string &source() const { return m_source; }
    void setSource(std::string s) { m_source = std::move(s); }

    std::optional<double> timestamp() const { return m_ts; }
    void setTimestamp(std::optional<double> ts) { m_ts = ts; }

    bool isUndo() const { return m_isUndo; }
    void setUndo(bool v) { m_isUndo = v; }

    virtual const char *kind() const = 0;
    virtual ApplyResult doApply(DeviceDocument &doc) = 0;
    virtual std::unique_ptr<DocEdit> generateUndo(const DeviceDocument &doc) const = 0;
    /** Wire envelope { opId, type, source, payload, ts? }. Not used on the tool path. */
    virtual JsonValue serialize() const = 0;
    virtual std::unique_ptr<DocEdit> clone() const = 0;
    /** Node ids this forward edit versions (F20 / lastOpId). */
    virtual std::vector<std::string> targets() const = 0;

    static std::unique_ptr<DocEdit> fromJson(const JsonValue &j);

protected:
    JsonValue envelope(JsonValue payload) const
    {
        JsonValue::Object o;
        o.emplace_back("opId", JsonValue::string(m_id));
        o.emplace_back("type", JsonValue::string(kind()));
        if (!m_source.empty())
            o.emplace_back("source", JsonValue::string(m_source));
        o.emplace_back("payload", std::move(payload));
        if (m_ts)
            o.emplace_back("ts", JsonValue::number(*m_ts));
        return JsonValue::object(std::move(o));
    }

    void copyMetaTo(DocEdit &dst) const
    {
        dst.m_id = m_id;
        dst.m_source = m_source;
        dst.m_ts = m_ts;
        dst.m_isUndo = m_isUndo;
    }

    std::string m_id;
    std::string m_source = "epaper";
    std::optional<double> m_ts;
    bool m_isUndo = false;
};

} // namespace document
} // namespace epaper
