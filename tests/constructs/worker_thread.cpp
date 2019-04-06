#pragma ide diagnostic ignored "readability-magic-numbers"

#include <vector>

#include <kktest.hpp>
#include <kktest_ext/matchers.hpp>

#include <mcga/threading/constructs/worker_construct.hpp>
#include <mcga/threading/constructs/worker_thread_construct.hpp>

#include "../testing_utils/basic_processor.hpp"

using kktest::multiRunTest;
using kktest::setUp;
using kktest::tearDown;
using kktest::TestConfig;
using kktest::matchers::isEqualTo;
using mcga::threading::constructs::WorkerConstruct;
using mcga::threading::constructs::WorkerThreadConstruct;
using mcga::threading::testing::BasicProcessor;
using std::operator""ms;
using std::thread;
using std::vector;
namespace this_thread = std::this_thread;

using TestingProcessor = BasicProcessor<int>;
using WorkerThread = WorkerThreadConstruct<WorkerConstruct<TestingProcessor>>;

TEST_CASE(Worker, "Worker") {
    WorkerThread* worker = nullptr;

    setUp([&] {
        worker = new WorkerThread();
        worker->start();
    });

    tearDown([&] {
        worker->stop();
        delete worker;
        worker = nullptr;
    });

    multiRunTest(TestConfig("Enqueueing executables from different threads")
                 .setTimeTicksLimit(10), 10, [&] {
        constexpr int task = 1;
        constexpr int numWorkers = 100;
        constexpr int numWorkerJobs = 100000;

        vector<thread*> workers(numWorkers, nullptr);
        for (int i = 0; i < numWorkers; ++ i) {
            workers[i] = new thread([&] {
                for (int j = 0; j < numWorkerJobs; ++ j) {
                    worker->enqueue(task);
                }
            });
        }
        for (int i = 0; i < numWorkers; ++ i) {
            workers[i]->join();
            delete workers[i];
        }
        while (TestingProcessor::numProcessed() < numWorkers * numWorkerJobs) {
            this_thread::sleep_for(1ms);
        }
        this_thread::sleep_for(100ms);
        expect(TestingProcessor::numProcessed(),
               isEqualTo(numWorkers * numWorkerJobs));
    });
}