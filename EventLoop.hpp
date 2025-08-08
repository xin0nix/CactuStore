#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <exception>
#include <expected>
#include <functional>
#include <queue>
#include <span>
#include <unordered_map>

namespace cactus {
using TaskCallback = std::function<void(std::expected<size_t, std::exception>)>;
struct Task {
  TaskCallback mCallback;
  size_t mIndex;
  std::expected<size_t, std::exception> mPayload;
};

struct TcpSocket;

struct EventLoop : std::enable_shared_from_this<EventLoop> {
  void handleTask(size_t index, size_t payload = 0UL);

  void onTimeout(std::chrono::nanoseconds duration, TaskCallback callback);

  void onAcceptConnection(std::shared_ptr<TcpSocket> socket,
                          TaskCallback callback);

  void onTcpSocketRead(std::shared_ptr<TcpSocket> socket,
                       std::span<uint8_t> buffer, TaskCallback callback);

  void run();

  std::atomic<size_t> mTaskCount{0};
  std::mutex mMutex;
  std::unordered_map<size_t, Task> mPendingTasks;
  std::queue<Task> mReadyTasks;
  std::condition_variable mCondVar;

  ~EventLoop();
};
} // namespace cactus