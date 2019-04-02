#pragma ide diagnostic ignored "readability-magic-numbers"

#include <random>
#include <set>

#include <kktest.hpp>
#include <kktest_ext/matchers.hpp>

#include "mcga/threading/event_loop_thread.hpp"

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
using mcga::threading::DelayedInvocationPtr;
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

milliseconds randomDelay() {
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::uniform_int_distribution<> distribution(1, 10);
    return milliseconds(distribution(generator));
}

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
        int x = 0;
        loop->enqueueInterval([&]{ x += 1; }, 100ms);
        this_thread::sleep_for(105ms);
        expect(x, isEqualTo(1));
        this_thread::sleep_for(105ms);
        expect(x, isEqualTo(2));
        this_thread::sleep_for(105ms);
        expect(x, isEqualTo(3));
        this_thread::sleep_for(105ms);
        expect(x, isEqualTo(4));
        this_thread::sleep_for(105ms);
        expect(x, isEqualTo(5));
    });

    test("Delayed executions are executed at the given time", [&] {
        int x = 0;

        loop->enqueueDelayed([&] { x = 1; }, 100ms);
        loop->enqueueDelayed([&] { x = 2; }, 500ms);
        loop->enqueueDelayed([&] { x = 3; }, 400ms);
        loop->enqueueDelayed([&] { x = 4; }, 200ms);
        loop->enqueueDelayed([&] { x = 5; }, 300ms);

        this_thread::sleep_for(105ms);
        expect(x, isEqualTo(1));

        this_thread::sleep_for(105ms);
        expect(x, isEqualTo(4));

        this_thread::sleep_for(105ms);
        expect(x, isEqualTo(5));

        this_thread::sleep_for(105ms);
        expect(x, isEqualTo(3));

        this_thread::sleep_for(105ms);
        expect(x, isEqualTo(2));
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

        this_thread::sleep_for(180ms);

        invocation->cancel();

        this_thread::sleep_for(200ms);
        expect(x, isEqualTo(3));
    });

    test("Cancelling an interval within the interval", [&] {
        int x = 0;

        DelayedInvocationPtr invocation = loop->enqueueInterval([&] {
            x += 1;
            invocation->cancel();
        }, 1000us);

        this_thread::sleep_for(10000us);
        expect(x, isEqualTo(1));
    });

    test("A delayed invocation will still execute even if immediate "
         "invocations keep getting added to the queue", [&] {
        int x = 0;
        int y = 0;

        auto limit = steady_clock::now() + 7ms;
        EventLoopThread::Executable func = [&x, &loop, &func, limit] {
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