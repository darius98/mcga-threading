#pragma ide diagnostic ignored "readability-magic-numbers"

#include <iostream>

#include <mcga/threading.hpp>

using mcga::threading::EventLoopThread;
using std::operator""ms;
using std::operator""s;
using std::cout;

int main() {
    EventLoopThread loop;

    loop.start();
    loop.enqueueDelayed([]() {
        cout << "This message appears after 2 seconds.\n";
    }, 2s);
    loop.enqueueInterval([&loop]() {
        cout << "This message appears every second.\n";
        loop.enqueue([]() {
            cout << "This message appears right after the every-second one.\n";
        });
    }, 1s);
    loop.enqueueInterval([]() {
        cout << "This message appears every 750ms.\n";
    }, 750ms);

    std::this_thread::sleep_for(30s);

    loop.stop();

    return 0;
}
