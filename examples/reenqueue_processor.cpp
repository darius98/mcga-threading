#pragma ide diagnostic ignored "readability-magic-numbers"

#include <iostream>

#include <mcga/threading.hpp>

using mcga::threading::constructs::EventLoopThreadConstruct;
using mcga::threading::constructs::EventLoopThreadPoolConstruct;
using mcga::threading::constructs::WorkerThreadConstruct;
using mcga::threading::OpaqueEventLoopEnqueuer;
using std::operator""ms;
using std::cout;
using std::endl;
using std::mutex;
using std::make_unique;
using std::unique_ptr;

class Processor {
public:
    using Task = unique_ptr<int>;

    mutex printMutex;

    void executeTask(Task task, OpaqueEventLoopEnqueuer<Task>* enqueuer) {
        if (*task <= 100) {
            enqueuer->enqueueDelayed(make_unique<int>(300 + *task), 300ms);
        }

        std::lock_guard guard(printMutex);
        cout << "Processing " << *task << " on thread "
             << std::hash<std::thread::id>()(std::this_thread::get_id())
             << endl;
    }
};
using OwnEventLoopThread = EventLoopThreadConstruct<Processor>;
using OwnEventLoopThreadPool = EventLoopThreadPoolConstruct<Processor>;
using OwnWorkerThread = WorkerThreadConstruct<Processor>;

int main() {
    OwnEventLoopThread loop;

    loop.start();
    for (int i = 1; i <= 100; ++ i) {
        loop.enqueue(make_unique<int>(i));
    }

    std::this_thread::sleep_for(1000ms);

    loop.stop();

    OwnEventLoopThreadPool pool;

    pool.start();
    pool.enqueueDelayed(make_unique<int>(300), 300ms);
    for (int i = 1; i <= 100; ++ i) {
        pool.enqueue(make_unique<int>(i));
    }

    std::this_thread::sleep_for(1000ms);

    pool.stop();

//    OwnWorkerThread worker; <- DOESN'T COMPILE!

    return 0;
}
