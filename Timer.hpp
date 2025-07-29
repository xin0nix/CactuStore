#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

namespace cactus {

struct EventLoop;
struct TimerSingleton {
  TimerSingleton() = default;
  static void setEvent(int timer, std::weak_ptr<EventLoop> loop);
  static void releaseEvent(int timer);
  static void fire(int timer);
  static TimerSingleton &get();
  static std::unique_ptr<TimerSingleton> mEntity;
  static std::mutex mMutex;
  std::unordered_map<int, std::weak_ptr<EventLoop>> mListeners;
};

} // namespace cactus