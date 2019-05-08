#pragma ide diagnostic ignored "readability-magic-numbers"

#include <iostream>

#include <mcga/threading.hpp>

using mcga::threading::ObjectEventLoopThread;
using std::operator""ms;
using std::cout;
using std::make_unique;
using std::unique_ptr;

int main() {
    ObjectEventLoopThread<unique_ptr<int>> loop(
      [](unique_ptr<int>& obj) { cout << "Processing " << *obj << "\n"; });

    loop.start();
    loop.enqueueDelayed(make_unique<int>(300), 300ms);
    for (int i = 1; i <= 100; ++i) {
        loop.enqueue(make_unique<int>(i));
    }

    std::this_thread::sleep_for(1000ms);

    loop.stop();

    return 0;
}
