#include <vector>

#include <mcga/test.hpp>
#include <mcga/test_ext/matchers.hpp>

#include <mcga/threading.hpp>

#include "../testing_utils/rand_utils.hpp"

using mcga::matchers::isFalse;
using mcga::matchers::isTrue;
using mcga::threading::base::ThreadPoolWrapper;
using mcga::threading::testing::randomBool;

namespace {

struct BasicWorker {
    using Processor = int;
    using Task = int;

    std::atomic_int numSpins = 0;

    size_t size() const {
        return 0;
    }

    void start(std::atomic_bool* running, Processor* /*processor*/) {
        while (running->load()) {
            numSpins += 1;
            std::this_thread::sleep_for(std::chrono::nanoseconds{20});
        }
    }
};

}  // namespace

TEST_CASE("ThreadPoolWrapper") {
    test("Starting and stopping ThreadPoolWrapper", [&] {
        ThreadPoolWrapper<BasicWorker, std::atomic_size_t> loop;
        expect(loop.isRunning(), isFalse);
        loop.start();
        expect(loop.isRunning(), isTrue);
        loop.stop();
        expect(loop.isRunning(), isFalse);
    });

    test(
      {
        .description = "Concurrent starts and stops do not break the "
                       "ThreadPoolWrapper",
        .timeTicksLimit = 10,
        .attempts = 10,
      },
      [&] {
          constexpr int numWorkers = 30;
          constexpr int numOps = 70;

          ThreadPoolWrapper<BasicWorker, std::atomic_size_t> loop;

          std::vector<std::thread> workers;
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
