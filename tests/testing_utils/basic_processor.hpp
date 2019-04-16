#pragma once

#include <set>
#include <thread>

namespace mcga::threading::testing {

template<class T>
struct BasicProcessor {
    using Task = T;

    static inline std::mutex processMutex;
    static inline std::set<std::size_t> threadIds{};
    static inline std::vector<Task> objects{};

    static inline std::function<void()> afterHandle;

    static std::size_t numProcessed() {
        return objects.size();
    }

    static void reset() {
        threadIds.clear();
        objects.clear();
        afterHandle = nullptr;
    }

    void executeTask(const Task& task) {
        std::lock_guard lock(processMutex);

        objects.push_back(task);
        threadIds.insert(
          std::hash<std::thread::id>()(std::this_thread::get_id()));
        if (afterHandle) {
            afterHandle();
        }
    }
};

}  // namespace mcga::threading::testing
