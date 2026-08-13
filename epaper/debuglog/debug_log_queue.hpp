#pragma once
/**
 * @implements [SRS-EP-15] bounded debug-log queue (cap 512, drop oldest)
 * @implements [SRS-EP-16] try-lock enqueue — caller must not block on I/O
 */

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace epaper {
namespace debuglog {

struct DebugLogRecord {
    std::int64_t ts = 0;
    std::string level;
    std::string logger;
    std::string msg;
};

class DebugLogQueue {
public:
    static constexpr std::size_t kCap = 512;

    /** Wait-free-ish: try_lock; on failure increment dropped and return false. */
    bool tryPush(DebugLogRecord rec)
    {
        if (rec.msg.size() > 4096)
            rec.msg.resize(4096);
        std::unique_lock<std::mutex> lock(m_mu, std::try_to_lock);
        if (!lock.owns_lock()) {
            m_dropped.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        if (m_q.size() >= kCap) {
            m_q.pop_front();
            m_dropped.fetch_add(1, std::memory_order_relaxed);
        }
        m_q.push_back(std::move(rec));
        return true;
    }

    /** Blocking take — worker thread only, never paint / ingestPoint. */
    std::vector<std::pair<DebugLogRecord, int>> takeAll()
    {
        std::lock_guard<std::mutex> lock(m_mu);
        std::vector<std::pair<DebugLogRecord, int>> out;
        out.reserve(m_q.size());
        const int d = m_dropped.exchange(0, std::memory_order_relaxed);
        for (std::size_t i = 0; i < m_q.size(); ++i)
            out.emplace_back(m_q[i], i == 0 ? d : 0);
        m_q.clear();
        return out;
    }

    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mu);
        return m_q.size();
    }

    int dropped() const { return m_dropped.load(std::memory_order_relaxed); }

private:
    mutable std::mutex m_mu;
    std::deque<DebugLogRecord> m_q;
    std::atomic<int> m_dropped{0};
};

} // namespace debuglog
} // namespace epaper
