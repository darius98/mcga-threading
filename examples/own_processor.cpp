#pragma ide diagnostic ignored "readability-magic-numbers"

#include <iostream>

#include <mcga/threading.hpp>

using mcga::threading::constructs::EventLoopThreadConstruct;
using std::operator""ms;
using std::cout;
using std::make_unique;
using std::unique_ptr;

class Processor {
 public:
    using Task = unique_ptr<int>;

    static void executeTask(Task task) {
        cout << "Processing " << *task << "\n";
    }
};
using OwnEventLoopThread = EventLoopThreadConstruct<Processor>;

int main() {
    OwnEventLoopThread loop;

    loop.start();
    loop.enqueueDelayed(make_unique<int>(300), 300ms);
    for (int i = 1; i <= 100; ++ i) {
        loop.enqueue(make_unique<int>(i));
    }

    std::this_thread::sleep_for(1000ms);

    loop.stop();

    return 0;
}
