#pragma ide diagnostic ignored "readability-magic-numbers"

#include <mutex>
#include <set>
#include <vector>

#include <kktest.hpp>
#include <kktest_ext/matchers.hpp>

#include <mcga/threading/base/worker.hpp>
#include <mcga/threading/constructs/worker_thread_construct.hpp>
#include <mcga/threading/constructs/worker_thread_pool_construct.hpp>

#include "../testing_utils/basic_processor.hpp"

using kktest::setUp;
using kktest::tearDown;
using kktest::multiRunTest;
using kktest::matchers::eachElement;
using kktest::matchers::hasSize;
using kktest::matchers::isEqualTo;
using kktest::matchers::isNotEqualTo;
using mcga::threading::base::Worker;
using mcga::threading::constructs::WorkerThreadConstruct;
using mcga::threading::constructs::WorkerThreadPoolConstruct;
using mcga::threading::testing::BasicProcessor;
using std::atomic_int;
using std::hash;
using std::operator""ms;
using std::thread;
using std::vector;
namespace this_thread = std::this_thread;

using TestingProcessor = BasicProcessor<int>;
using WorkerThread = WorkerThreadConstruct<Worker<TestingProcessor>>;
using WorkerThreadPool = WorkerThreadPoolConstruct<WorkerThread>;

TEST_CASE(WorkerThreadPool, "WorkerThreadPool") {
    WorkerThreadPool* pool = nullptr;

    setUp([&] {
        pool = new WorkerThreadPool(3ul);
        pool->start();
    });

    tearDown([&] {
        pool->stop();
        delete pool;
        pool = nullptr;
    });

    multiRunTest("Tasks enqueued in a WorkerThreadPool are executed on "
                 "multiple threads, different from the main thread", 10, [&] {
        constexpr int task = 1;
        constexpr int numTasks = 1000000;

        for (int i = 0; i < numTasks; ++ i) {
            pool->enqueue(task);
        }

        while (TestingProcessor::numProcessed() != numTasks) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(10ms);

        expect(TestingProcessor::numProcessed(), isEqualTo(numTasks));
        expect(TestingProcessor::threadIds, hasSize(3));
        expect(TestingProcessor::threadIds, eachElement(isNotEqualTo(
                hash<thread::id>()(this_thread::get_id()))));
    });

    multiRunTest("Tasks enqueued from multiple threads in a WorkerThreadPool"
                 " are executed on multiple threads, different from the main "
                 "thread", 10, [&] {
        constexpr int task = 1;
        constexpr int numWorkers = 100;
        constexpr int numWorkerJobs = 1000;

        vector<thread*> workers(numWorkers, nullptr);
        for (int i = 0; i < numWorkers; ++ i) {
            workers[i] = new thread([&] {
                for (int j = 0; j < numWorkerJobs; ++ j) {
                    pool->enqueue(task);
                }
            });
        }
        for (int i = 0; i < numWorkers; ++ i) {
            workers[i]->join();
            delete workers[i];
        }

        while (TestingProcessor::numProcessed() != numWorkers * numWorkerJobs) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(10ms);

        expect(TestingProcessor::numProcessed(),
               isEqualTo(numWorkers * numWorkerJobs));
        expect(TestingProcessor::threadIds, hasSize(3));
        expect(TestingProcessor::threadIds, eachElement(isNotEqualTo(
                hash<thread::id>()(this_thread::get_id()))));
    });
}
