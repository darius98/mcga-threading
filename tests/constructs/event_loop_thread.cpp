#include <algorithm>

#include <mcga/test.hpp>
#include <mcga/test_ext/matchers.hpp>

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

using TestingProcessor = BasicProcessor<int>;
using EventLoopThread = EventLoopThreadConstruct<TestingProcessor>;

std::ostream& operator<<(std::ostream& os,
                         const std::chrono::milliseconds& ms) {
    os << ms.count() << "ms";
    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const std::chrono::microseconds& us) {
    os << us.count() << "us";
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::chrono::nanoseconds& ns) {
    os << ns.count() << "ns";
    return os;
}

TEST_CASE("EventLoopThread") {
    constexpr int task = 1;

    std::unique_ptr<EventLoopThread> loop;

    setUp([&] {
        loop = std::make_unique<EventLoopThread>();
        loop->start();
    });

    tearDown([&] {
        loop->stop();
        loop.reset();
        TestingProcessor::reset();
    });

    test("All tasks enqueued in an EventLoopThread are executed on the "
         "same thread, different from the main thread",
         [&] {
             constexpr int numTasks = 10000;

             for (int i = 0; i < numTasks; ++i) {
                 loop->enqueue(task);
                 loop->enqueueDelayed(task,
                                      std::chrono::nanoseconds(numTasks - i));
             }

             while (loop->sizeApprox() > 0) {
                 std::this_thread::sleep_for(std::chrono::milliseconds{1});
             }
             std::this_thread::sleep_for(std::chrono::milliseconds{10});

             expect(TestingProcessor::numProcessed(), isEqualTo(2 * numTasks));
             expect(TestingProcessor::objects, eachElement(isEqualTo(task)));
             expect(TestingProcessor::threadIds, hasSize(1));
             expect(*TestingProcessor::threadIds.begin(),
                    isNotEqualTo(std::hash<std::thread::id>()(
                      std::this_thread::get_id())));
         });

    test("Enqueueing an executable executes it", [&] {
        loop->enqueue(task);
        while (TestingProcessor::numProcessed() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        expect(TestingProcessor::numProcessed(), isEqualTo(1));
        expect(TestingProcessor::objects[0], isEqualTo(task));
    });

    test("Enqueueing an executable delayed executes it", [&] {
        loop->enqueueDelayed(task, std::chrono::milliseconds{1});
        while (TestingProcessor::numProcessed() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        expect(TestingProcessor::numProcessed(), isEqualTo(1));
        expect(TestingProcessor::objects[0], isEqualTo(task));
    });

    test("Enqueueing an executable interval executes it multiple times", [&] {
        std::vector<std::chrono::nanoseconds> executionDelays;
        auto startTime = std::chrono::steady_clock::now();
        loop->enqueueInterval(task, std::chrono::milliseconds{10});
        TestingProcessor::afterHandle = [&] {
            executionDelays.push_back(duration_cast<std::chrono::nanoseconds>(
              std::chrono::steady_clock::now() - startTime));
        };

        while (executionDelays.size() < 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
        }
        expect(executionDelays, hasSize(10));
        expect(TestingProcessor::numProcessed(), isEqualTo(10));
        expect(TestingProcessor::objects, eachElement(isEqualTo(task)));
        for (size_t i = 1; i < executionDelays.size(); ++i) {
            expect(executionDelays[i] - executionDelays[i - 1],
                   isGreaterThanEqual(std::chrono::milliseconds{10}));
        }
    });

    test("Delayed executions are executed in the expected order", [&] {
        loop->enqueueDelayed(1, std::chrono::milliseconds{100});
        loop->enqueueDelayed(2, std::chrono::milliseconds{500});
        loop->enqueueDelayed(3, std::chrono::milliseconds{400});
        loop->enqueueDelayed(4, std::chrono::milliseconds{200});
        loop->enqueueDelayed(5, std::chrono::milliseconds{300});

        while (TestingProcessor::objects.size() < 5) {
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        expect(TestingProcessor::objects,
               isEqualTo(std::vector<int>{1, 4, 5, 3, 2}));
    });

    test("Cancelling a delayed invocation", [&] {
        auto invocation
          = loop->enqueueDelayed(task, std::chrono::milliseconds{1});

        invocation->cancel();

        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        expect(TestingProcessor::numProcessed(), isZero);
    });

    test("Cancelling an interval", [&] {
        auto invocation
          = loop->enqueueInterval(task, std::chrono::milliseconds{50});

        while (TestingProcessor::numProcessed() < 3) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }

        invocation->cancel();

        std::this_thread::sleep_for(std::chrono::milliseconds{200});
        expect(TestingProcessor::numProcessed(), isEqualTo(3));
    });

    test("Cancelling an interval within the interval", [&] {
        auto invocation
          = loop->enqueueInterval(task, std::chrono::milliseconds{10});
        TestingProcessor::afterHandle = [&] { invocation->cancel(); };

        while (TestingProcessor::numProcessed() < 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        expect(TestingProcessor::numProcessed(), isEqualTo(1));
    });

    test("A delayed invocation will still execute even if immediate "
         "invocations keep getting added to the queue",
         [&] {
             auto limit = std::chrono::steady_clock::now()
               + std::chrono::milliseconds{100};

             loop->enqueue(1);
             TestingProcessor::afterHandle = [&] {
                 if (std::chrono::steady_clock::now() <= limit) {
                     loop->enqueue(1);
                 }
             };
             loop->enqueueDelayed(2, std::chrono::milliseconds{5});

             std::this_thread::sleep_for(std::chrono::milliseconds{500});

             expect(TestingProcessor::objects, anyElement(isEqualTo(2)));
         });

    multiRunTest(TestConfig("Enqueueing delayed executables from different "
                            "threads")
                   .setTimeTicksLimit(10),
                 10,
                 [&] {
                     constexpr int numWorkers = 100;
                     constexpr int numWorkerJobs = 1000;

                     std::vector<std::thread> workers;
                     workers.reserve(numWorkers);
                     for (int i = 0; i < numWorkers; ++i) {
                         workers.emplace_back([&loop, i] {
                             for (int j = 0; j < numWorkerJobs; ++j) {
                                 loop->enqueueDelayed(i * numWorkerJobs + j,
                                                      randomDelay());
                             }
                         });
                     }
                     for (int i = 0; i < numWorkers; ++i) {
                         workers[i].join();
                     }
                     while (TestingProcessor::numProcessed()
                            < numWorkers * numWorkerJobs) {
                         std::this_thread::sleep_for(
                           std::chrono::milliseconds{1});
                     }
                     std::this_thread::sleep_for(std::chrono::milliseconds{50});
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
           .setTimeTicksLimit(10),
         [&] {
             constexpr int numSamples = 200;
             for (int i = 0; i < numSamples; ++i) {
                 const auto expected = std::chrono::milliseconds{2};
                 auto startTime = std::chrono::steady_clock::now();
                 std::chrono::nanoseconds actual(0);
                 loop->enqueueDelayed(task, std::chrono::milliseconds{2});
                 TestingProcessor::afterHandle = [&] {
                     actual = std::chrono::steady_clock::now() - startTime;
                 };

                 while (actual.count() == 0) {
                     std::this_thread::sleep_for(std::chrono::microseconds{10});
                 }

                 expect(actual, isGreaterThanEqual(expected));
             }
         });
}