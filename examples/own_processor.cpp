#pragma ide diagnostic ignored "readability-magic-numbers"

#include <iostream>

#include <mcga/threading.hpp>

using mcga::threading::constructs::EventLoopThreadConstruct;
using mcga::threading::constructs::EventLoopThreadPoolConstruct;
using std::operator""ms;
using std::cout;
using std::endl;
using std::make_unique;
using std::mutex;
using std::unique_ptr;

class Processor {
  public:
    using Task = unique_ptr<int>;

    mutex printMutex;

    void executeTask(const Task& task) {
        std::lock_guard guard(printMutex);
        cout << "Processing " << *task << " on thread "
             << std::hash<std::thread::id>()(std::this_thread::get_id())
             << endl;
    }
};
using OwnEventLoopThread = EventLoopThreadConstruct<Processor>;
using OwnEventLoopThreadPool = EventLoopThreadPoolConstruct<Processor>;

int main() {
    OwnEventLoopThread loop;

    loop.start();
    loop.enqueueDelayed(make_unique<int>(300), 300ms);
    for (int i = 1; i <= 100; ++i) {
        loop.enqueue(make_unique<int>(i));
    }

    std::this_thread::sleep_for(1000ms);

    loop.stop();

    OwnEventLoopThreadPool pool;

    pool.start();
    pool.enqueueDelayed(make_unique<int>(300), 300ms);
    for (int i = 1; i <= 100; ++i) {
        pool.enqueue(make_unique<int>(i));
    }

    std::this_thread::sleep_for(1000ms);

    pool.stop();

    return 0;
}
