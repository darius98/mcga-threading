#pragma ide diagnostic ignored "readability-magic-numbers"

#include <set>

#include <kktest.hpp>
#include <kktest_ext/matchers.hpp>

#include "mcga/threading/event_loop.hpp"

#include "rand_utils.hpp"

using kktest::setUp;
using kktest::tearDown;
using kktest::test;
using kktest::TestConfig;
using kktest::matchers::hasSize;
using kktest::matchers::isZero;
using kktest::matchers::isEqualTo;
using kktest::matchers::isNotEqualTo;
using kktest::matchers::isGreaterThanEqual;
using kktest::matchers::expect;
using mcga::threading::EventLoopThread;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using std::chrono::steady_clock;
using std::hash;
using std::ostream;
using std::set;
using std::thread;
using std::vector;
using std::operator""ms;
using std::operator""us;
namespace this_thread = std::this_thread;

ostream& operator<<(ostream& os, const milliseconds& ms) {
    os << ms.count() << "ms";
    return os;
}

ostream& operator<<(ostream& os, const microseconds& us) {
    os << us.count() << "us";
    return os;
}

ostream& operator<<(ostream& os, const nanoseconds& ns) {
    os << ns.count() << "ns";
    return os;
}

TEST_CASE(EventLoop, "EventLoop") {
    EventLoopThread* loop = nullptr;

    setUp([&] {
        loop = new EventLoopThread();
        loop->start();
    });

    tearDown([&] {
        loop->stop();
        delete loop;
        loop = nullptr;
    });

    test("All tasks enqueued in an EventLoopThread are executed on the "
         "same thread, different from the main thread", [&] {
        constexpr int numTasks = 10000;

        set<size_t> threadIds;
        int numTasksExecuted = 0;

        auto task = [&] {
            numTasksExecuted += 1;
            threadIds.insert(hash<thread::id>()(this_thread::get_id()));
        };

        for (int i = 0; i < numTasks; ++ i) {
            loop->enqueue(task);
            loop->enqueueDelayed(task, nanoseconds(numTasks - i));
        }

        while (loop->sizeApprox() > 0) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(10ms);

        expect(numTasksExecuted, isEqualTo(2 * numTasks));
        expect(threadIds, hasSize(1));
        expect(*threadIds.begin(),
               isNotEqualTo(hash<thread::id>()(this_thread::get_id())));
    });

    test("Enqueueing an executable executes it", [&] {
        int x = 0;
        loop->enqueue([&]{ x += 1; });
        while (loop->sizeApprox() > 0) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(10ms);
        expect(x, isEqualTo(1));
    });

    test("Enqueueing an executable delayed executes it", [&] {
        int x = 0;
        loop->enqueueDelayed([&]{ x += 1; }, 1ms);
        while (loop->sizeApprox() > 0) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(10ms);
        expect(x, isEqualTo(1));
    });

    test("Enqueueing an executable interval executes it multiple times", [&] {
        vector<nanoseconds> executionDelays;
        steady_clock::time_point startTime = steady_clock::now();
        loop->enqueueInterval([&]{
            executionDelays.push_back(
                duration_cast<nanoseconds>(steady_clock::now() - startTime));
        }, 10ms);

        while (executionDelays.size() < 10) {
            this_thread::sleep_for(5ms);
        }
        expect(executionDelays, hasSize(10));
        for (size_t i = 1; i < executionDelays.size(); ++ i) {
            expect(executionDelays[i] - executionDelays[i - 1],
                   isGreaterThanEqual(10ms));
        }
    });

    test("Delayed executions are executed in the expected order", [&] {
        vector<int> values;
        loop->enqueueDelayed([&] { values.push_back(1); }, 100ms);
        loop->enqueueDelayed([&] { values.push_back(2); }, 500ms);
        loop->enqueueDelayed([&] { values.push_back(3); }, 400ms);
        loop->enqueueDelayed([&] { values.push_back(4); }, 200ms);
        loop->enqueueDelayed([&] { values.push_back(5); }, 300ms);

        while (values.size() < 5) {
            this_thread::sleep_for(5ms);
        }
        this_thread::sleep_for(100ms);
        expect(values, isEqualTo(vector<int>{1, 4, 5, 3, 2}));
    });

    test("Cancelling a delayed invocation", [&] {
        int x = 0;

        auto invocation = loop->enqueueDelayed([&] { x += 1; }, 1000us);

        invocation->cancel();

        this_thread::sleep_for(10000us);
        expect(x, isZero);
    });

    test("Cancelling an interval", [&] {
        int x = 0;

        auto invocation = loop->enqueueInterval([&] { x += 1; }, 50ms);

        while (x < 3) {
            this_thread::sleep_for(1ms);
        }

        invocation->cancel();

        this_thread::sleep_for(200ms);
        expect(x, isEqualTo(3));
    });

    test("Cancelling an interval within the interval", [&] {
        int x = 0;

        EventLoopThread::DelayedInvocationPtr invocation
                = loop->enqueueInterval([&] {
            x += 1;
            invocation->cancel();
        }, 10ms);

        while (x < 1) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(100ms);
        expect(x, isEqualTo(1));
    });

    test("A delayed invocation will still execute even if immediate "
         "invocations keep getting added to the queue", [&] {
        int x = 0;
        int y = 0;

        auto limit = steady_clock::now() + 7ms;
        EventLoopThread::Object func = [&x, &loop, &func, limit] {
            x += 1;
            if (steady_clock::now() <= limit) {
                loop->enqueue(func);
            }
        };
        loop->enqueue(func);

        loop->enqueueDelayed([&] { y = 1; }, 5ms);

        while (y == 0) {
            this_thread::sleep_for(1ms);
        }

        expect(y, isEqualTo(1));
    });

    multiRunTest(TestConfig("Enqueueing delayed executables from different "
                            "threads")
                 .setTimeTicksLimit(10), 10, [&] {
        constexpr int numWorkers = 100;
        constexpr int numJobsPerWorker = 1000;

        int x = 0;

        vector<thread*> workers(numWorkers, nullptr);
        for (int i = 0; i < numWorkers; ++ i) {
            workers[i] = new thread([&] {
                for (int j = 0; j < numJobsPerWorker; ++ j) {
                    loop->enqueueDelayed([&] { x += 1; }, randomDelay());
                }
            });
        }
        for (int i = 0; i < numWorkers; ++ i) {
            workers[i]->join();
            delete workers[i];
        }
        while (x < numWorkers * numJobsPerWorker) {
            this_thread::sleep_for(1ms);
        }
        expect(x, isEqualTo(numWorkers * numJobsPerWorker));
    });

    test(TestConfig("A delayed invocation is never executed before "
                    "a period at least equal to its delay has passed")
         .setTimeTicksLimit(50), [&] {
        constexpr int numSamples = 10000;
        for (int i = 0; i < numSamples; ++ i) {
            nanoseconds expected = 3ms;
            auto startTime = steady_clock::now();
            nanoseconds actual(0);
            loop->enqueueDelayed([startTime, &actual] {
                actual = steady_clock::now() - startTime;
            }, 3ms);

            while (actual.count() == 0) {
                this_thread::sleep_for(1ms);
            }

            expect(actual, isGreaterThanEqual(expected));
        }
    });
}