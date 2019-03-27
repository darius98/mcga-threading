#include <random>
#include <set>

#include <kktest.hpp>
#include <kktest_ext/matchers.hpp>

#include "mcga/threading/event_loop_thread.hpp"

using namespace kktest;
using namespace kktest::matchers;
using namespace mcga::threading;
using namespace std;

bool randomBool() {
    static random_device rd;
    static mt19937 generator(rd());
    static uniform_int_distribution<> distribution(0, 1);
    return distribution(generator) == 1;
}

TEST_CASE(EventLoopThread, "EventLoopThread") {
    EventLoopThread* loop = nullptr;

    setUp([&] {
        loop = new EventLoopThread();
    });

    tearDown([&] {
        delete loop;
        loop = nullptr;
    });

    test("EventLoopThread does not start by default", [&] {
        int x = 0;
        loop->enqueue([&] { x = 1; });

        this_thread::sleep_for(10ms);

        expect(x, isZero);
    });

    test("Starting an EventLoopThread executes enqueued tasks", [&] {
        constexpr int numTasks = 10000;
        int x = 0;
        for (int i = 0; i < numTasks; ++ i) {
            loop->enqueue([&] { x += 1; });
        }

        loop->start();

        this_thread::sleep_for(10ms);

        expect(x, isEqualTo(numTasks));
    });

    multiRunTest("Multiple concurrent starts and stops do not break the "
                 "EventLoopThread", 1000, [&] {
        constexpr int numWorkers = 10;
        constexpr int numOps = 500;

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
    });

    test("All tasks enqueued in an EventLoopThread are executed on the same "
         "thread, different from the main thread", [&] {
        constexpr int numTasks = 10000;

        loop->start();

        set<size_t> threadIds;
        int numTasksExecuted = 0;

        auto task = [&] {
            numTasksExecuted += 1;
            threadIds.insert(hash<thread::id>()(this_thread::get_id()));
        };

        for (int i = 0; i < numTasks; ++ i) {
            loop->enqueue(task);
            loop->enqueueDelayed(task, chrono::nanoseconds(numTasks - i));
        }

        while (loop->size() > 0) {
            this_thread::sleep_for(50ns);
        }

        expect(numTasksExecuted, isEqualTo(2 * numTasks));
        expect(threadIds, hasSize(1));
        expect(*threadIds.begin(),
               isNotEqualTo(hash<thread::id>()(this_thread::get_id())));
    });
}