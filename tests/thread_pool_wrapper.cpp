#pragma ide diagnostic ignored "readability-magic-numbers"

#include <vector>

#include <kktest.hpp>
#include <kktest_ext/matchers.hpp>

#include "mcga/threading/internal/thread_wrapper.hpp"
#include "mcga/threading/internal/thread_pool_wrapper.hpp"

#include "rand_utils.hpp"

using kktest::multiRunTest;
using kktest::test;
using kktest::matchers::expect;
using kktest::matchers::isFalse;
using kktest::matchers::isTrue;
using mcga::threading::internal::ThreadPoolWrapper;
using mcga::threading::internal::ThreadWrapper;
using std::atomic_bool;
using std::operator""ns;
using std::size_t;
using std::thread;
using std::vector;

namespace {

struct BasicWorker {
    volatile int numSpins = 0;

    size_t size() const {
        return 0;
    }

    void start(volatile atomic_bool* running) {
        while (running->load()) {
            numSpins += 1;
            std::this_thread::sleep_for(20ns);
        }
    }
};

}  // namespace

TEST_CASE(ThreadPoolWrapper, "ThreadPoolWrapper") {
    test("Starting and stopping ThreadPoolWrapper", [&] {
        auto loop = new ThreadPoolWrapper<ThreadWrapper<BasicWorker>>(8);
        expect(loop->isRunning(), isFalse);
        loop->start();
        expect(loop->isRunning(), isTrue);
        loop->stop();
        expect(loop->isRunning(), isFalse);
        delete loop;
    });

    multiRunTest("Concurrent starts and stops do not break the "
                 "ThreadPoolWrapper", 10, [&] {
        constexpr int numWorkers = 100;
        constexpr int numOps = 10000;

        auto loop = new ThreadPoolWrapper<ThreadWrapper<BasicWorker>>(8);

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
