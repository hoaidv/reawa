/**
 * LatestJob: one in-flight + one pending; newer submit replaces waiting.
 */
#include "util/latest_job.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

int main()
{
    using namespace std::chrono_literals;
    std::mutex mu;
    std::condition_variable cv;
    std::vector<int> got;
    std::atomic<int> started{0};

    epaper::LatestJob<int, int> job;
    job.start(
        [&](const int &x, const std::atomic<bool> &cancel) {
            ++started;
            for (int i = 0; i < 20; ++i) {
                if (cancel.load())
                    return -1;
                std::this_thread::sleep_for(5ms);
            }
            return x * 10;
        },
        [&](int r) {
            std::lock_guard<std::mutex> lock(mu);
            got.push_back(r);
            cv.notify_all();
        });

    job.submit(1);
    {
        std::unique_lock<std::mutex> lock(mu);
        cv.wait_for(lock, 1s, [&] { return started.load() >= 1; });
    }
    job.submit(2);
    job.submit(3);

    {
        std::unique_lock<std::mutex> lock(mu);
        cv.wait_for(lock, 2s, [&] { return got.size() >= 1 && got.back() == 30; });
    }
    job.stop();

    CHECK(!got.empty());
    CHECK(got.back() == 30);
    for (int v : got)
        CHECK(v != 20);

    // Keep in-flight: first result must deliver, then the pending latest.
    {
        std::mutex mu2;
        std::condition_variable cv2;
        std::vector<int> got2;
        std::atomic<int> started2{0};
        epaper::LatestJob<int, int> job2;
        job2.start(
            [&](const int &x, const std::atomic<bool> &cancel) {
                (void)cancel;
                ++started2;
                cv2.notify_all();
                std::this_thread::sleep_for(40ms);
                return x * 10;
            },
            [&](int r) {
                std::lock_guard<std::mutex> lock(mu2);
                got2.push_back(r);
                cv2.notify_all();
            });
        job2.submit(1);
        {
            std::unique_lock<std::mutex> lock(mu2);
            cv2.wait_for(lock, 1s, [&] { return started2.load() >= 1; });
        }
        job2.submit(2, false);
        {
            std::unique_lock<std::mutex> lock(mu2);
            cv2.wait_for(lock, 2s, [&] { return got2.size() >= 2; });
        }
        job2.stop();
        CHECK(got2.size() >= 2);
        CHECK(got2[0] == 10);
        CHECK(got2.back() == 20);
    }

    if (g_fails) {
        std::cerr << g_fails << " failure(s) got=";
        for (int v : got)
            std::cerr << v << " ";
        std::cerr << "\n";
        return 1;
    }
    std::cout << "latest_job_test: OK\n";
    return 0;
}
