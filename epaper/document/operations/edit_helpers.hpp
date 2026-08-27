#pragma once
/**
 * Shared JSON / target helpers for DocEdit method bodies.
 * @implements [SRS-EP-07] typed edit dispatch
 */

#include "doc_edit.hpp"

namespace epaper {
namespace document {

inline void addTargetId(std::vector<std::string> &ids, const std::string &nodeId)
{
    if (nodeId.empty())
        return;
    for (const auto &id : ids) {
        if (id == nodeId)
            return;
    }
    ids.push_back(nodeId);
}

inline std::optional<std::string> parentIdFromJson(const JsonValue &p)
{
    const JsonValue *x = p.get("parentId");
    if (!x || !x->isString() || x->asString().empty())
        return std::nullopt;
    return x->asString();
}

inline std::vector<InkSample> samplesFromJsonArray(const JsonValue *a)
{
    std::vector<InkSample> out;
    if (!a || !a->isArray())
        return out;
    for (const auto &s : a->asArray())
        out.push_back(sampleFromJson(s));
    return out;
}

inline JsonValue samplesToJsonArray(const std::vector<InkSample> &ss)
{
    JsonValue::Array a;
    a.reserve(ss.size());
    for (const auto &s : ss)
        a.push_back(sampleToJson(s));
    return JsonValue::array(std::move(a));
}

inline void fillMeta(DocEdit &e, const JsonValue &envelope)
{
    e.setId(envelope.getString("opId"));
    const std::string src = envelope.getString("source");
    e.setSource(src.empty() ? "epaper" : src);
    e.setTimestamp(optNumber(envelope, "ts"));
}

} // namespace document
} // namespace epaper
