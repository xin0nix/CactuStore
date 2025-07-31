#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <queue>
#include <span>
#include <unordered_map>

namespace cactus {
struct Task {
  std::function<void(void)> mCallback;
  size_t mIndex;
};

struct TcpSocket;

struct EventLoop : std::enable_shared_from_this<EventLoop> {
  void handleTask(size_t index);

  void onTimeout(std::chrono::nanoseconds duration,
                 std::function<void(void)> callback);

  void onTcpSocketRead(std::shared_ptr<TcpSocket> socket,
                       std::span<uint8_t> buffer,
                       std::function<void(size_t)> callback);

  void run();

  std::atomic<size_t> mTaskCount{0};
  std::mutex mMutex;
  std::unordered_map<size_t, Task> mPendingTasks;
  std::queue<Task> mReadyTasks;
  std::condition_variable mCondVar;

  ~EventLoop();
};
} // namespace cactus