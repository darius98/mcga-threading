#include <iostream>

#include <mcga/threading.hpp>

using mcga::threading::EventLoopThread;

int main() {
    EventLoopThread loop;

    loop.start();
    loop.enqueueDelayed(
      []() {
          std::cout << "This message appears after 2 seconds.\n";
      },
      std::chrono::seconds{2});
    loop.enqueueInterval(
      [&loop]() {
          std::cout << "This message appears every second.\n";
          loop.enqueue([]() {
              std::cout
                << "This message appears right after the every-second one.\n";
          });
      },
      std::chrono::seconds{1});
    loop.enqueueInterval(
      []() {
          std::cout << "This message appears every 750ms.\n";
      },
      std::chrono::milliseconds{750});

    std::this_thread::sleep_for(std::chrono::seconds{30});

    loop.stop();

    return 0;
}
