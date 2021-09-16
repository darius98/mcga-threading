#pragma ide diagnostic ignored "readability-magic-numbers"

#include <vector>

#include <mcga/test.hpp>
#include <mcga/test_ext/matchers.hpp>

#include <mcga/threading.hpp>

#include "../testing_utils/basic_processor.hpp"
#include "../testing_utils/rand_utils.hpp"

using mcga::matchers::eachElement;
using mcga::matchers::hasSize;
using mcga::matchers::isEqualTo;
using mcga::matchers::isNotEqualTo;
using mcga::test::expect;
using mcga::test::multiRunTest;
using mcga::test::setUp;
using mcga::test::tearDown;
using mcga::threading::constructs::EventLoopThreadPoolConstruct;
using mcga::threading::testing::BasicProcessor;
using mcga::threading::testing::randomBool;
using mcga::threading::testing::randomDelay;
using std::atomic_int;
using std::hash;
using std::operator""ms;
using std::thread;
using std::vector;
namespace this_thread = std::this_thread;

using TestingProcessor = BasicProcessor<int>;
using EventLoopThreadPool = EventLoopThreadPoolConstruct<TestingProcessor>;

TEST_CASE(EventLoopThreadPool, "EventLoopThreadPool") {
    multiRunTest(
      "Tasks enqueued in a EventLoopThreadPool are executed on "
      "multiple threads, different from the main thread",
      10,
      [&] {
          constexpr int numTasks = 100000;

          EventLoopThreadPool pool(EventLoopThreadPool::NumThreads(3));
          pool.start();

          for (int i = 0; i < numTasks; ++i) {
              pool.enqueue(1);
              pool.enqueueDelayed(1, 3ms);
          }

          while (TestingProcessor::numProcessed() != 2 * numTasks) {
              this_thread::sleep_for(1ms);
          }
          this_thread::sleep_for(100ms);

          expect(TestingProcessor::numProcessed(), isEqualTo(2 * numTasks));
          expect(TestingProcessor::threadIds, hasSize(3));
          expect(TestingProcessor::threadIds,
                 eachElement(
                   isNotEqualTo(hash<thread::id>()(this_thread::get_id()))));
          TestingProcessor::reset();
          pool.stop();
      });

    multiRunTest(
      "Tasks enqueued from multiple threads in a EventLoopThreadPool"
      " are executed on multiple threads, different from the main "
      "thread",
      10,
      [&] {
          constexpr int numWorkers = 100;
          constexpr int numWorkerJobs = 1000;

          EventLoopThreadPool pool(EventLoopThreadPool::NumThreads(3));
          pool.start();

          vector<thread> workers;
          workers.reserve(numWorkers);
          for (int i = 0; i < numWorkers; ++i) {
              workers.emplace_back([&] {
                  for (int j = 0; j < numWorkerJobs; ++j) {
                      if (randomBool()) {
                          pool.enqueueDelayed(1, randomDelay());
                      } else {
                          pool.enqueue(1);
                      }
                  }
              });
          }
          for (int i = 0; i < numWorkers; ++i) {
              workers[i].join();
          }

          while (TestingProcessor::numProcessed()
                 != numWorkers * numWorkerJobs) {
              this_thread::sleep_for(1ms);
          }
          this_thread::sleep_for(100ms);

          expect(TestingProcessor::numProcessed(),
                 isEqualTo(numWorkers * numWorkerJobs));
          expect(TestingProcessor::threadIds, hasSize(3));
          expect(TestingProcessor::threadIds,
                 eachElement(
                   isNotEqualTo(hash<thread::id>()(this_thread::get_id()))));
          TestingProcessor::reset();
          pool.stop();
      });
}
