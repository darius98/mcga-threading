#pragma ide diagnostic ignored "readability-magic-numbers"

#include <vector>

#include <kktest.hpp>
#include <kktest_ext/matchers.hpp>

#include <mcga/threading.hpp>

#include "../testing_utils/rand_utils.hpp"

using kktest::multiRunTest;
using kktest::test;
using kktest::TestConfig;
using kktest::matchers::expect;
using kktest::matchers::isFalse;
using kktest::matchers::isTrue;
using mcga::threading::base::ThreadPoolWrapper;
using mcga::threading::base::ThreadWrapper;
using mcga::threading::testing::randomBool;
using std::atomic_bool;
using std::operator""ns;
using std::size_t;
using std::thread;
using std::vector;

namespace {

struct BasicWorker {
    using Processor = int;
    using Task = int;

    volatile int numSpins = 0;

    size_t size() const {
        return 0;
    }

    void start(volatile atomic_bool* running, Processor* /*processor*/) {
        while (running->load()) {
            numSpins += 1;
            std::this_thread::sleep_for(20ns);
        }
    }
};

}  // namespace

TEST_CASE(ThreadPoolWrapper, "ThreadPoolWrapper") {
    test("Starting and stopping ThreadPoolWrapper", [&] {
        auto loop = new ThreadPoolWrapper<BasicWorker, std::atomic_size_t>(8ul);
        expect(loop->isRunning(), isFalse);
        loop->start();
        expect(loop->isRunning(), isTrue);
        loop->stop();
        expect(loop->isRunning(), isFalse);
        delete loop;
    });

    multiRunTest(TestConfig("Concurrent starts and stops do not break the "
                            "ThreadPoolWrapper")
                 .setTimeTicksLimit(10), 10, [&] {
        constexpr int numWorkers = 100;
        constexpr int numOps = 100;

        auto loop = new ThreadPoolWrapper<BasicWorker, std::atomic_size_t>(8ul);

        vector<thread*> workers(numWorkers, nullptr);
        for (int i = 0; i < numWorkers; ++ i) {
            workers[i] = new thread([&] {
                for (int j = 0; j < numOps; ++ j) {
                    if (randomBool()) {
                        loop->start();
                    } else {
                        loop->stop();
                    }
                }
            });
        }
        for (int i = 0; i < numWorkers; ++ i) {
            workers[i]->join();
            delete workers[i];
        }

        delete loop;
    });
}
