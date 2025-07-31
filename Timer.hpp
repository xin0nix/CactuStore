#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

namespace cactus {

struct EventLoop;
struct TimerSingleton {
  TimerSingleton() = default;
  static void setEvent(int eventIndex, std::chrono::nanoseconds duration,
                       std::weak_ptr<EventLoop> loop);
  static void releaseEvent(int eventIndex);
  static void fire(int eventIndex);
  static TimerSingleton &get();

private:
  static void setEvent(int eventIndex, std::weak_ptr<EventLoop> loop);
  static std::unique_ptr<TimerSingleton> mEntity;
  static std::mutex mMutex;
  std::unordered_map<int, std::weak_ptr<EventLoop>> mListeners;
};

} // namespace cactus