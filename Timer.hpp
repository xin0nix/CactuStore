#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace cactus {

struct TimerSingleton {
  TimerSingleton() = default;
  static void setEvent(int eventIndex, std::chrono::nanoseconds duration,
                       std::function<void(size_t)>);
  static void releaseEvent(int eventIndex);
  static void fire(int eventIndex);
  static TimerSingleton &get();

private:
  static void setEvent(int eventIndex, std::function<void(size_t)>);
  static std::unique_ptr<TimerSingleton> mEntity;
  static std::mutex mMutex;
  std::unordered_map<int, std::function<void(size_t)>> mListeners;
};

} // namespace cactus