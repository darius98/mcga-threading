#pragma ide diagnostic ignored "readability-magic-numbers"

#include <algorithm>

#include <mcga/test_ext/matchers.hpp>
#include <mcga/test.hpp>

#include <mcga/threading/base/event_loop.hpp>
#include <mcga/threading/constructs.hpp>

#include "../testing_utils/basic_processor.hpp"
#include "../testing_utils/rand_utils.hpp"

using mcga::matchers::anyElement;
using mcga::matchers::eachElement;
using mcga::matchers::hasSize;
using mcga::matchers::isEqualTo;
using mcga::matchers::isGreaterThanEqual;
using mcga::matchers::isNotEqualTo;
using mcga::matchers::isZero;
using mcga::test::expect;
using mcga::test::setUp;
using mcga::test::tearDown;
using mcga::test::test;
using mcga::test::TestConfig;
using mcga::threading::base::EventLoop;
using mcga::threading::constructs::EventLoopThreadConstruct;
using mcga::threading::testing::BasicProcessor;
using mcga::threading::testing::randomDelay;
using std::hash;
using std::ostream;
using std::sort;
using std::thread;
using std::vector;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using std::chrono::steady_clock;
using std::operator""ms;
namespace this_thread = std::this_thread;

using TestingProcessor = BasicProcessor<int>;
using EventLoopThread = EventLoopThreadConstruct<TestingProcessor>;

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

TEST_CASE(EventLoopThread, "EventLoopThread") {
    constexpr int task = 1;

    EventLoopThread* loop = nullptr;

    setUp([&] {
        loop = new EventLoopThread();
        loop->start();
    });

    tearDown([&] {
        loop->stop();
        delete loop;
        loop = nullptr;
        TestingProcessor::reset();
    });

    test("All tasks enqueued in an EventLoopThread are executed on the "
         "same thread, different from the main thread",
         [&] {
             constexpr int numTasks = 10000;

             for (int i = 0; i < numTasks; ++i) {
                 loop->enqueue(task);
                 loop->enqueueDelayed(task, nanoseconds(numTasks - i));
             }

             while (loop->sizeApprox() > 0) {
                 this_thread::sleep_for(1ms);
             }
             this_thread::sleep_for(10ms);

             expect(TestingProcessor::numProcessed(), isEqualTo(2 * numTasks));
             expect(TestingProcessor::objects, eachElement(isEqualTo(task)));
             expect(TestingProcessor::threadIds, hasSize(1));
             expect(*TestingProcessor::threadIds.begin(),
                    isNotEqualTo(hash<thread::id>()(this_thread::get_id())));
         });

    test("Enqueueing an executable executes it", [&] {
        loop->enqueue(task);
        while (TestingProcessor::numProcessed() == 0) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(10ms);
        expect(TestingProcessor::numProcessed(), isEqualTo(1));
        expect(TestingProcessor::objects[0], isEqualTo(task));
    });

    test("Enqueueing an executable delayed executes it", [&] {
        loop->enqueueDelayed(task, 1ms);
        while (TestingProcessor::numProcessed() == 0) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(10ms);
        expect(TestingProcessor::numProcessed(), isEqualTo(1));
        expect(TestingProcessor::objects[0], isEqualTo(task));
    });

    test("Enqueueing an executable interval executes it multiple times", [&] {
        vector<nanoseconds> executionDelays;
        steady_clock::time_point startTime = steady_clock::now();
        loop->enqueueInterval(task, 10ms);
        TestingProcessor::afterHandle = [&] {
            executionDelays.push_back(
              duration_cast<nanoseconds>(steady_clock::now() - startTime));
        };

        while (executionDelays.size() < 10) {
            this_thread::sleep_for(5ms);
        }
        expect(executionDelays, hasSize(10));
        expect(TestingProcessor::numProcessed(), isEqualTo(10));
        expect(TestingProcessor::objects, eachElement(isEqualTo(task)));
        for (size_t i = 1; i < executionDelays.size(); ++i) {
            expect(executionDelays[i] - executionDelays[i - 1],
                   isGreaterThanEqual(10ms));
        }
    });

    test("Delayed executions are executed in the expected order", [&] {
        loop->enqueueDelayed(1, 100ms);
        loop->enqueueDelayed(2, 500ms);
        loop->enqueueDelayed(3, 400ms);
        loop->enqueueDelayed(4, 200ms);
        loop->enqueueDelayed(5, 300ms);

        while (TestingProcessor::objects.size() < 5) {
            this_thread::sleep_for(5ms);
        }
        this_thread::sleep_for(100ms);
        expect(TestingProcessor::objects,
               isEqualTo(vector<int>{1, 4, 5, 3, 2}));
    });

    test("Cancelling a delayed invocation", [&] {
        auto invocation = loop->enqueueDelayed(task, 1ms);

        invocation->cancel();

        this_thread::sleep_for(10ms);
        expect(TestingProcessor::numProcessed(), isZero);
    });

    test("Cancelling an interval", [&] {
        auto invocation = loop->enqueueInterval(task, 50ms);

        while (TestingProcessor::numProcessed() < 3) {
            this_thread::sleep_for(1ms);
        }

        invocation->cancel();

        this_thread::sleep_for(200ms);
        expect(TestingProcessor::numProcessed(), isEqualTo(3));
    });

    test("Cancelling an interval within the interval", [&] {
        auto invocation = loop->enqueueInterval(task, 10ms);
        TestingProcessor::afterHandle = [&] { invocation->cancel(); };

        while (TestingProcessor::numProcessed() < 1) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(100ms);
        expect(TestingProcessor::numProcessed(), isEqualTo(1));
    });

    test("A delayed invocation will still execute even if immediate "
         "invocations keep getting added to the queue",
         [&] {
             auto limit = steady_clock::now() + 100ms;

             loop->enqueue(1);
             TestingProcessor::afterHandle = [&] {
                 if (steady_clock::now() <= limit) {
                     loop->enqueue(1);
                 }
             };
             loop->enqueueDelayed(2, 5ms);

             this_thread::sleep_for(500ms);

             expect(TestingProcessor::objects, anyElement(isEqualTo(2)));
         });

    multiRunTest(TestConfig("Enqueueing delayed executables from different "
                            "threads")
                   .setTimeTicksLimit(10),
                 10,
                 [&] {
                     constexpr int numWorkers = 100;
                     constexpr int numWorkerJobs = 1000;

                     vector<thread*> workers(numWorkers, nullptr);
                     for (int i = 0; i < numWorkers; ++i) {
                         workers[i] = new thread([&loop, i] {
                             for (int j = 0; j < numWorkerJobs; ++j) {
                                 loop->enqueueDelayed(i * numWorkerJobs + j,
                                                      randomDelay());
                             }
                         });
                     }
                     for (int i = 0; i < numWorkers; ++i) {
                         workers[i]->join();
                         delete workers[i];
                     }
                     while (TestingProcessor::numProcessed()
                            < numWorkers * numWorkerJobs) {
                         this_thread::sleep_for(1ms);
                     }
                     this_thread::sleep_for(50ms);
                     expect(TestingProcessor::numProcessed(),
                            isEqualTo(numWorkers * numWorkerJobs));
                     sort(TestingProcessor::objects.begin(),
                          TestingProcessor::objects.end());
                     for (int i = 0; i < numWorkers * numWorkerJobs; ++i) {
                         expect(TestingProcessor::objects[i], isEqualTo(i));
                     }
                 });

    test(TestConfig("A delayed invocation is never executed before "
                    "a period at least equal to its delay has passed")
           .setTimeTicksLimit(50),
         [&] {
             constexpr int numSamples = 10000;
             for (int i = 0; i < numSamples; ++i) {
                 nanoseconds expected = 3ms;
                 auto startTime = steady_clock::now();
                 nanoseconds actual(0);
                 loop->enqueueDelayed(task, 3ms);
                 TestingProcessor::afterHandle
                   = [&] { actual = steady_clock::now() - startTime; };

                 while (actual.count() == 0) {
                     this_thread::sleep_for(1ms);
                 }

                 expect(actual, isGreaterThanEqual(expected));
             }
         });
}