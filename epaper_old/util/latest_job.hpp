#pragma once

/**
 * One in-flight compute + at most one pending request.
 * A newer submit replaces the waiting request. By default it also cooperatively
 * cancels in-flight so the worker starts the latest input next (object-erase
 * overlay: a half-finished 80% test is worthless). Camera sharpen passes
 * cancelRunning=false so a finished buffer can be warped toward the latest
 * camera instead of being aborted every 200 ms nav tick.
 */

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

#if defined(__APPLE__)
#include <pthread.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

namespace epaper {

template <typename Request, typename Result>
class LatestJob {
public:
    using Compute = std::function<Result(const Request &, const std::atomic<bool> &cancel)>;
    using Deliver = std::function<void(Result)>;

    LatestJob() = default;
    ~LatestJob() { stop(); }

    LatestJob(const LatestJob &) = delete;
    LatestJob &operator=(const LatestJob &) = delete;

    void start(Compute compute, Deliver deliver)
    {
        stop();
        m_compute = std::move(compute);
        m_deliver = std::move(deliver);
        m_stop = false;
        m_cancel = false;
        m_thread = std::thread([this] { loop(); });
    }

    /** Queue `req` as the single pending job.
     *  @param cancelRunning  true (default): abort in-flight so this input runs next.
     *                        false: let in-flight finish and deliver; then run pending. */
    void submit(Request req, bool cancelRunning = true)
    {
        std::lock_guard<std::mutex> lock(m_mu);
        if (m_stop)
            return;
        m_pending = std::move(req);
        if (cancelRunning && m_inFlight)
            m_cancel = true;
        m_cv.notify_one();
    }

    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(m_mu);
            m_stop = true;
            m_cancel = true;
            m_pending.reset();
            m_cv.notify_one();
        }
        if (m_thread.joinable())
            m_thread.join();
        m_compute = {};
        m_deliver = {};
    }

private:
    void loop()
    {
        lowerThisThread();
        for (;;) {
            Request req;
            {
                std::unique_lock<std::mutex> lock(m_mu);
                m_cv.wait(lock, [&] { return m_stop || m_pending.has_value(); });
                if (m_stop && !m_pending.has_value())
                    return;
                if (!m_pending.has_value())
                    continue;
                req = std::move(*m_pending);
                m_pending.reset();
                m_inFlight = true;
            }
            m_cancel = false;
            if (!m_compute) {
                m_inFlight = false;
                continue;
            }
            Result r = m_compute(req, m_cancel);
            m_inFlight = false;
            if (m_stop)
                return;
            if (m_cancel.load())
                continue;
            if (m_deliver)
                m_deliver(std::move(r));
        }
    }

    static void lowerThisThread()
    {
#if defined(__APPLE__)
        pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0);
#elif defined(__linux__)
        struct sched_param sp {};
#if defined(SCHED_IDLE)
        pthread_setschedparam(pthread_self(), SCHED_IDLE, &sp);
#elif defined(SCHED_BATCH)
        pthread_setschedparam(pthread_self(), SCHED_BATCH, &sp);
#endif
#endif
    }

    Compute m_compute;
    Deliver m_deliver;
    std::mutex m_mu;
    std::condition_variable m_cv;
    std::optional<Request> m_pending;
    std::atomic<bool> m_stop{true};
    std::atomic<bool> m_cancel{false};
    std::atomic<bool> m_inFlight{false};
    std::thread m_thread;
};

} // namespace epaper
