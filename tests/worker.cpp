#pragma ide diagnostic ignored "readability-magic-numbers"

#include <vector>

#include <kktest.hpp>
#include <kktest_ext/matchers.hpp>

#include <mcga/threading.hpp>

using kktest::multiRunTest;
using kktest::setUp;
using kktest::tearDown;
using kktest::TestConfig;
using kktest::matchers::isEqualTo;
using mcga::threading::WorkerThread;
using std::operator""ms;
using std::thread;
using std::vector;
namespace this_thread = std::this_thread;

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
        constexpr int numWorkers = 100;
        constexpr int numJobsPerWorker = 100000;

        int x = 0; // note: Not atomic by design.

        vector<thread*> workers(numWorkers, nullptr);
        for (int i = 0; i < numWorkers; ++ i) {
            workers[i] = new thread([&] {
                for (int j = 0; j < numJobsPerWorker; ++ j) {
                    worker->enqueue([&] { x += 1; });
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
        this_thread::sleep_for(100ms);
        expect(x, isEqualTo(numWorkers * numJobsPerWorker));
    });
}
