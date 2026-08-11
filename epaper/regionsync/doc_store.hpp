#pragma once
/**
 * Minimal materialised doc store for Epaper region sync (append_ink + remote ops).
 * @implements [SRS-EP-02] idempotent remote doc_op apply
 */

#include "viewport_map.hpp"

#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace epaper::regionsync {

struct InkSample {
    double x = 0;
    double y = 0;
    std::optional<double> pressure;
    std::optional<double> tiltX;
    std::optional<double> tiltY;
    std::map<std::string, double> extras;
};

struct InkNode {
    std::string id;
    std::vector<InkSample> samples;
};

struct DocOp {
    std::string opId;
    std::string type;
    std::string source; // epaper|infini
    std::string inkId;
    std::vector<InkSample> samples;
};

inline bool sampleInAabb(const InkSample &s, const Aabb &r)
{
    return s.x >= r.minX && s.x <= r.maxX && s.y >= r.minY && s.y <= r.maxY;
}

class DocStore {
public:
    int version() const { return m_version; }
    const std::map<std::string, InkNode> &inks() const { return m_inks; }
    bool hasOp(const std::string &opId) const { return m_applied.count(opId) > 0; }

    struct ApplyResult {
        bool applied = false;
        std::string reason;
    };

    ApplyResult apply(const DocOp &op)
    {
        if (m_applied.count(op.opId))
            return {false, "duplicate_opId"};

        if (op.type == "append_ink") {
            if (op.inkId.empty())
                return {false, "missing_ink_id"};
            InkNode node;
            node.id = op.inkId;
            node.samples = op.samples;
            m_inks[op.inkId] = std::move(node);
            m_applied.insert(op.opId);
            ++m_version;
            return {true, {}};
        }

        return {false, std::string("unknown_type:") + op.type};
    }

    /** Debug content fingerprint for document ∩ drawing region. */
    std::string contentHash(const Aabb &region) const
    {
        std::ostringstream oss;
        oss << "v" << m_version << ":";
        for (const auto &kv : m_inks) {
            bool any = false;
            for (const auto &s : kv.second.samples) {
                if (sampleInAabb(s, region)) {
                    any = true;
                    break;
                }
            }
            if (any)
                oss << kv.first << ",";
        }
        return oss.str();
    }

private:
    int m_version = 0;
    std::map<std::string, InkNode> m_inks;
    std::set<std::string> m_applied;
};

} // namespace epaper::regionsync
