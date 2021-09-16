#pragma ide diagnostic ignored "readability-magic-numbers"

#include <vector>

#include <mcga/test.hpp>
#include <mcga/test_ext/matchers.hpp>

#include <mcga/threading.hpp>

#include "../testing_utils/rand_utils.hpp"

using mcga::matchers::isFalse;
using mcga::matchers::isTrue;
using mcga::test::expect;
using mcga::test::multiRunTest;
using mcga::test::test;
using mcga::test::TestConfig;
using mcga::threading::base::ThreadPoolWrapper;
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
        ThreadPoolWrapper<BasicWorker, std::atomic_size_t> loop(8ul);
        expect(loop.isRunning(), isFalse);
        loop.start();
        expect(loop.isRunning(), isTrue);
        loop.stop();
        expect(loop.isRunning(), isFalse);
    });

    multiRunTest(TestConfig("Concurrent starts and stops do not break the "
                            "ThreadPoolWrapper")
                   .setTimeTicksLimit(10),
                 10,
                 [&] {
                     constexpr int numWorkers = 30;
                     constexpr int numOps = 70;

                     ThreadPoolWrapper<BasicWorker, std::atomic_size_t> loop(8ul);

                     vector<thread> workers;
                     workers.reserve(numWorkers);
                     for (int i = 0; i < numWorkers; ++i) {
                         workers.emplace_back([&] {
                             for (int j = 0; j < numOps; ++j) {
                                 if (randomBool()) {
                                     loop.start();
                                 } else {
                                     loop.stop();
                                 }
                             }
                         });
                     }
                     for (int i = 0; i < numWorkers; ++i) {
                         workers[i].join();
                     }
                 });
}
