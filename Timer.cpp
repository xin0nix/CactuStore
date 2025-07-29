#include "Timer.hpp"
#include "EventLoop.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>


namespace cactus {

void TimerSingleton::setEvent(int timer, std::weak_ptr<EventLoop> loop) {
  std::lock_guard lg(mMutex);
  get().mListeners[timer] = std::move(loop);
}

void TimerSingleton::fire(int timer) {
  std::unique_lock lock(mMutex);
  auto loop = get().mListeners[timer].lock();
  if (!loop) {
    return;
  }
  lock.unlock();
  loop->handleTask(timer);
}

void TimerSingleton::releaseEvent(int timer) {
  std::lock_guard lg(mMutex);
  get().mListeners.erase(timer);
}

TimerSingleton &TimerSingleton::get() {
  if (!mEntity) {
    mEntity = std::make_unique<TimerSingleton>();
  }
  return *mEntity;
}

std::unique_ptr<TimerSingleton> TimerSingleton::mEntity;
std::mutex TimerSingleton::mMutex;
} // namespace cactus