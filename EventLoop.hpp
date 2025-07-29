#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <queue>
#include <unordered_map>

namespace cactus {
struct Task {
  std::function<void(void)> mCallback;
  size_t mIndex;
};

struct EventLoop : std::enable_shared_from_this<EventLoop> {
  void handleTask(size_t index);

  void onTimeout(std::chrono::nanoseconds duration,
                 std::function<void(void)> callback);

  void run();

  std::atomic<size_t> mTaskCount{0};
  std::mutex mMutex;
  std::unordered_map<size_t, Task> mPendingTasks;
  std::queue<Task> mReadyTasks;
  std::condition_variable mCondVar;

  ~EventLoop();
};
} // namespace cactus