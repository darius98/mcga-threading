#pragma ide diagnostic ignored "readability-magic-numbers"

#include <random>
#include <thread>
#include <vector>

#include <kktest.hpp>
#include <kktest_ext/matchers.hpp>

#include "mcga/threading/event_loop.hpp"

using kktest::setUp;
using kktest::tearDown;
using kktest::test;
using kktest::TestConfig;
using kktest::expect;
using kktest::matchers::expect;
using kktest::matchers::isEqualTo;
using kktest::matchers::isGreaterThanEqual;
using kktest::matchers::isZero;
using mcga::threading::DelayedInvocationPtr;
using mcga::threading::EventLoop;
using mcga::threading::Executable;
using std::atomic_bool;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using std::chrono::steady_clock;
using std::literals::chrono_literals::operator""ms;
using std::literals::chrono_literals::operator""ns;
using std::literals::chrono_literals::operator""us;
using std::ostream;
using std::thread;
using std::vector;
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
    thread* eventLoopThread = nullptr;
    EventLoop* loop = nullptr;

    setUp([&] {
        loop = new EventLoop();
        expect(!loop->isRunning(), "Loop is running right after instantiation");

        atomic_bool started;
        eventLoopThread = new thread([&] {
            started = true;
            loop->start();
        });
        while (!started) {
            this_thread::sleep_for(1ns);
        }
        // This sleep is to make sure the event loop thread's start() call is
        // executed before the testing thread's stop() call.
        this_thread::sleep_for(1ms);
        expect(loop->isRunning(), "Loop is not running");
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

    test("size() is zero initially", [&] {
        expect(loop->size(), isZero);
    });

    test("Enqueueing an executable executes it", [&] {
        int x = 0;
        loop->enqueue([&]{ x += 1; });
        while (loop->size() > 0) {
            this_thread::sleep_for(1ms);
        }
        expect(x, isEqualTo(1));
    });

    test("Enqueueing an executable delayed executes it", [&] {
        int x = 0;
        loop->enqueueDelayed([&]{ x += 1; }, 1ms);
        while (loop->size() > 0) {
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
        Executable func = [&x, &loop, &func, limit] {
            x += 1;
            if (steady_clock::now() <= limit) {
                loop->enqueue(func);
            }
        };
        loop->enqueue(func);

        loop->enqueueDelayed([&] { y = 1; }, 5ms);

        this_thread::sleep_for(10ms);
        while (loop->size() > 0) {
            this_thread::sleep_for(1ms);
        }

        expect(y, isEqualTo(1));
    });

    test(TestConfig("Enqueueing executables from different threads")
         .setTimeTicksLimit(10), [&] {
        constexpr int numWorkers = 30;
        constexpr int numJobsPerWorker = 50000;

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
        while (loop->size() > 0) {
            this_thread::sleep_for(1ms);
        }
        expect(x, isEqualTo(numWorkers * numJobsPerWorker));
    });

    test(TestConfig("A delayed invocation is never executed before "
                    "a period at least equal to its delay has passed")
         .setTimeTicksLimit(10), [&] {
        constexpr int numSamples = 1000;
        for (int i = 0; i < numSamples; ++ i) {
            nanoseconds expected = 3ms;
            auto startTime = steady_clock::now();
            nanoseconds actual;
            loop->enqueueDelayed([startTime, &actual] {
                actual = steady_clock::now() - startTime;
            }, 3ms);

            while (loop->size() > 0) {
                this_thread::sleep_for(4us);
            }

            expect(actual, isGreaterThanEqual(expected));
        }
    });
}
