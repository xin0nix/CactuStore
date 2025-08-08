#include "Timer.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>

void signal_handler(int sig, siginfo_t *si, void *uc) {
  int index = si->si_value.sival_int;
  cactus::TimerSingleton::fire(index);
}

namespace cactus {

void TimerSingleton::setEvent(int eventIndex, std::chrono::nanoseconds duration,
                              std::function<void(size_t)> handler) {
  // TODO: move to the timer!!!
  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = signal_handler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  struct sigevent sev;
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIGRTMIN;
  sev.sigev_value.sival_int = eventIndex;
  timer_t timerid;
  if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
    perror("timer_create");
    exit(EXIT_FAILURE);
  }

  struct itimerspec its;
  its.it_value.tv_sec = duration.count() / 1000'000'000;
  its.it_value.tv_nsec = duration.count() % 1000'000'000;
  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = 0;

  TimerSingleton::setEvent(eventIndex, handler);
  if (timer_settime(timerid, 0, &its, NULL) == -1) {
    perror("timer_settime");
    exit(EXIT_FAILURE);
  }
}

void TimerSingleton::setEvent(int timer, std::function<void(size_t)> handler) {
  std::lock_guard lg(mMutex);
  get().mListeners[timer] = std::move(handler);
}

void TimerSingleton::fire(int timer) {
  std::unique_lock lock(mMutex);
  auto handler = get().mListeners[timer];
  lock.unlock();
  handler(timer);
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