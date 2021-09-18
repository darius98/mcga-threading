#include <iostream>

#include <mcga/threading.hpp>

using mcga::threading::constructs::EventLoopThreadConstruct;
using mcga::threading::constructs::EventLoopThreadPoolConstruct;

class Processor {
  public:
    using Task = std::unique_ptr<int>;

    std::mutex printMutex;

    void executeTask(const Task& task) {
        std::lock_guard guard(printMutex);
        std::cout << "Processing " << *task << " on thread "
                  << std::hash<std::thread::id>()(std::this_thread::get_id())
                  << std::endl;
    }
};
using OwnEventLoopThread = EventLoopThreadConstruct<Processor>;
using OwnEventLoopThreadPool = EventLoopThreadPoolConstruct<Processor>;

int main() {
    OwnEventLoopThread loop;

    loop.start();
    loop.enqueueDelayed(std::make_unique<int>(300),
                        std::chrono::milliseconds{300});
    for (int i = 1; i <= 100; ++i) {
        loop.enqueue(std::make_unique<int>(i));
    }

    std::this_thread::sleep_for(std::chrono::seconds{1});

    loop.stop();

    OwnEventLoopThreadPool pool;

    pool.start();
    pool.enqueueDelayed(std::make_unique<int>(300),
                        std::chrono::milliseconds{300});
    for (int i = 1; i <= 100; ++i) {
        pool.enqueue(std::make_unique<int>(i));
    }

    std::this_thread::sleep_for(std::chrono::seconds{1});

    pool.stop();

    return 0;
}
