#include <iostream>

#include <mcga/threading.hpp>

using mcga::threading::ObjectEventLoopThread;

int main() {
    ObjectEventLoopThread<std::unique_ptr<int>> loop(
      [](std::unique_ptr<int>& obj) {
          std::cout << "Processing " << *obj << "\n";
      });

    loop.start();
    loop.enqueueDelayed(std::make_unique<int>(300),
                        std::chrono::milliseconds{300});
    for (int i = 1; i <= 100; ++i) {
        loop.enqueue(std::make_unique<int>(i));
    }

    std::this_thread::sleep_for(std::chrono::seconds{1});

    loop.stop();

    return 0;
}
