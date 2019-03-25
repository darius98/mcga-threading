#include <random>
#include <thread>
#include <vector>

#include <kktest.hpp>
#include <kktest_ext/matchers.hpp>

#include "mcga/threading/event_loop.hpp"

using namespace kktest;
using namespace kktest::matchers;
using namespace mcga::threading;
using namespace std;

chrono::milliseconds randomDelay() {
    static random_device rd;
    static mt19937 generator(rd());
    static uniform_int_distribution<> distribution(1, 10);
    return chrono::milliseconds(distribution(generator));
}

ostream& operator<<(ostream& os, const chrono::milliseconds& ms) {
    os << ms.count() << "ms";
    return os;
}

ostream& operator<<(ostream& os, const chrono::microseconds& us) {
    os << us.count() << "us";
    return os;
}

ostream& operator<<(ostream& os, const chrono::nanoseconds& ns) {
    os << ns.count() << "ns";
    return os;
}

TEST_CASE(EventLoop, "EventLoop") {
    thread* eventLoopThread = nullptr;
    EventLoop* loop = nullptr;

    setUp([&] {
        loop = new EventLoop();
        eventLoopThread = new thread([&] {
            loop->start();
        });
        // This sleep is to make sure the event loop thread's start() call is
        // executed before the testing thread's stop() call.
        this_thread::sleep_for(1ms);
    });

    tearDown([&] {
        loop->stop();
        eventLoopThread->join();
        delete eventLoopThread;
        delete loop;
    });

    test("Starting and stopping loop works", [&] {
        // This is done in set-up, tear-down. The only condition is time-out.
    });

    test("getNumPendingJobs() is zero initially", [&] {
        expect(loop->getNumPendingJobs(), isZero);
    });

    test("Enqueueing an executable executes it", [&] {
        int x = 0;
        loop->enqueue([&]{ x += 1; });
        while (loop->getNumPendingJobs() > 0) {
            this_thread::sleep_for(1ms);
        }
        expect(x, isEqualTo(1));
    });

    test("Enqueueing an executable delayed executes it", [&] {
        int x = 0;
        loop->enqueueDelayed([&]{ x += 1; }, 1ms);
        while (loop->getNumPendingJobs() > 0) {
            this_thread::sleep_for(1ms);
        }
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

    multiRunTest("Enqueueing executables from different threads", 1000, [&] {
        int x = 0;

        constexpr int numWorkers = 30;
        constexpr int numJobsPerWorker = 500;

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
        while (loop->getNumPendingJobs() > 0) {
            this_thread::sleep_for(1ms);
        }
        expect(x, isEqualTo(numWorkers * numJobsPerWorker));
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

        auto invocation = loop->enqueueInterval([&] { x += 1; }, 1000us);

        this_thread::sleep_for(3050us);

        invocation->cancel();

        this_thread::sleep_for(10000us);
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

    multiRunTest("A delayed invocation is never executed before "
                 "a period at least equal to its delay has passed", 1000, [&] {
        auto expected = chrono::steady_clock::now() + 3ms;

        chrono::steady_clock::time_point actual;
        loop->enqueueDelayed([&] {
            actual = chrono::steady_clock::now();
        }, 3ms);

        this_thread::sleep_for(5ms);

        expect(actual, isGreaterThanEqual(expected));
    });

    test(TestConfig("Delay error is within 1ms 99% of the time")
         .setAttempts(1000)
         .setRequiredPassedAttempts(990), [&] {
        auto expected = chrono::steady_clock::now() + 10ms;

        chrono::steady_clock::time_point actual;
        loop->enqueueDelayed([&] {
            actual = chrono::steady_clock::now();
        }, 10ms);

        this_thread::sleep_for(15ms);

        expect(actual - expected, isLessThan(1ms));
    });

    test(TestConfig("Delay error is within 50us 90% of the time")
         .setAttempts(1000)
         .setRequiredPassedAttempts(900), [&] {
        auto expected = chrono::steady_clock::now() + 3ms;

        chrono::steady_clock::time_point actual;
        loop->enqueueDelayed([&] {
            actual = chrono::steady_clock::now();
        }, 3ms);

        this_thread::sleep_for(5ms);

        expect(actual - expected, isLessThan(50us));
    });

    test(TestConfig("Delay error is within 20us 50% of the time")
         .setAttempts(1000)
         .setRequiredPassedAttempts(500), [&] {
        auto expected = chrono::steady_clock::now() + 3ms;

        chrono::steady_clock::time_point actual;
        loop->enqueueDelayed([&] {
            actual = chrono::steady_clock::now();
        }, 3ms);

        this_thread::sleep_for(4ms);

        expect(actual - expected, isLessThan(20us));
    });

    multiRunTest("A delayed invocation will still execute even if immediate "
                 "invocations keep getting added to the queue", 1000, [&] {
        int x = 0;
        int y = 0;

        auto limit = chrono::steady_clock::now() + 7ms;
        Executable func = [&x, &loop, &func, limit] {
            x += 1;
            if (chrono::steady_clock::now() <= limit) {
                loop->enqueue(func);
            }
        };
        loop->enqueue(func);

        loop->enqueueDelayed([&] { y = 1; }, 5ms);

        this_thread::sleep_for(10ms);
        while (loop->getNumPendingJobs() > 0) {
            this_thread::sleep_for(1ms);
        }

        expect(y, isEqualTo(1));
    });
}
