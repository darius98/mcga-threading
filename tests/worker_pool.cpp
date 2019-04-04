#pragma ide diagnostic ignored "readability-magic-numbers"

#include <mutex>
#include <set>
#include <vector>

#include <kktest.hpp>
#include <kktest_ext/matchers.hpp>

#include <mcga/threading.hpp>

using kktest::setUp;
using kktest::tearDown;
using kktest::multiRunTest;
using kktest::matchers::eachElement;
using kktest::matchers::hasSize;
using kktest::matchers::isEqualTo;
using kktest::matchers::isNotEqualTo;
using mcga::threading::WorkerThreadPool;
using std::atomic_int;
using std::hash;
using std::lock_guard;
using std::mutex;
using std::operator""ms;
using std::set;
using std::thread;
using std::vector;
namespace this_thread = std::this_thread;

TEST_CASE(WorkerThreadPool, "WorkerThreadPool") {
    WorkerThreadPool* pool = nullptr;

    setUp([&] {
        pool = new WorkerThreadPool(3);
        pool->start();
    });

    tearDown([&] {
        pool->stop();
        delete pool;
        pool = nullptr;
    });

    multiRunTest("Tasks enqueued in a WorkerThreadPool are executed on "
                 "multiple threads, different from the main thread", 10, [&] {
        constexpr int numTasks = 1000000;

        mutex threadIdsMutex;
        set<size_t> threadIds;
        atomic_int numTasksExecuted = 0;

        auto task = [&] {
            numTasksExecuted += 1;

            lock_guard guard(threadIdsMutex);
            threadIds.insert(hash<thread::id>()(this_thread::get_id()));
        };

        for (int i = 0; i < numTasks; ++ i) {
            pool->enqueue(task);
        }

        while (numTasksExecuted != numTasks) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(10ms);

        expect(numTasksExecuted, isEqualTo(numTasks));
        expect(threadIds, hasSize(3));
        expect(threadIds, eachElement(isNotEqualTo(
                hash<thread::id>()(this_thread::get_id()))));
    });

    multiRunTest("Tasks enqueued from multiple threads in a WorkerThreadPool"
                 " are executed on multiple threads, different from the main "
                 "thread", 10, [&] {
        constexpr int numWorkers = 100;
        constexpr int numJobsPerWorker = 1000;

        mutex threadIdsMutex;
        set<size_t> threadIds;
        atomic_int numTasksExecuted = 0;

        auto task = [&] {
            numTasksExecuted += 1;

            lock_guard guard(threadIdsMutex);
            threadIds.insert(hash<thread::id>()(this_thread::get_id()));
        };

        vector<thread*> workers(numWorkers, nullptr);
        for (int i = 0; i < numWorkers; ++ i) {
            workers[i] = new thread([&] {
                for (int j = 0; j < numJobsPerWorker; ++ j) {
                    pool->enqueue(task);
                }
            });
        }
        for (int i = 0; i < numWorkers; ++ i) {
            workers[i]->join();
            delete workers[i];
        }

        while (numTasksExecuted != numWorkers * numJobsPerWorker) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(100ms);

        expect(numTasksExecuted, isEqualTo(numWorkers * numJobsPerWorker));
        expect(threadIds, hasSize(3));
        expect(threadIds, eachElement(isNotEqualTo(
                hash<thread::id>()(this_thread::get_id()))));
    });
}
