#include "EventLoop.hpp"
#include "TcpSocket.hpp"
#include "Timer.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>

void signal_handler(int sig, siginfo_t *si, void *uc) {
  int index = si->si_value.sival_int;
  cactus::TimerSingleton::fire(index);
}

namespace cactus {

void EventLoop::handleTask(size_t index) {
  auto weak = weak_from_this();
  auto self = weak.lock();
  if (!self) {
    return;
  }
  std::lock_guard guard(self->mMutex);
  auto task = self->mPendingTasks.at(index);
  self->mReadyTasks.push(std::move(task));
  mCondVar.notify_one();
}

void EventLoop::onTimeout(std::chrono::nanoseconds duration,
                          std::function<void(void)> callback) {
  auto weak = weak_from_this();
  auto self = weak.lock();
  if (!self) {
    return;
  }
  std::lock_guard guard(self->mMutex);
  self->mPendingTasks[mTaskCount] = Task{
      .mCallback = std::move(callback),
      .mIndex = mTaskCount,
  };

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
  sev.sigev_value.sival_int = mTaskCount;
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

  TimerSingleton::setEvent(mTaskCount, weak_from_this());
  if (timer_settime(timerid, 0, &its, NULL) == -1) {
    perror("timer_settime");
    exit(EXIT_FAILURE);
  }
  mTaskCount += 1;
}

void EventLoop::onTcpSocketRead(std::shared_ptr<TcpSocket> socket,
                                std::span<uint8_t> buffer,
                                std::function<void(size_t)> callback) {
  // TODO: go through the event loop!
  socket->read(buffer, std::move(callback));
}

void EventLoop::run() {
  auto weak = weak_from_this();
  auto self = weak.lock();
  if (!self) {
    return;
  }
  bool done = false;
  while (!done) {
    const auto currentTime = std::chrono::system_clock::now();
    std::unique_lock lock(self->mMutex);
    mCondVar.wait(lock, [self]() { return !self->mReadyTasks.empty(); });
    const auto &top = self->mReadyTasks.front();
    auto callback = std::move(top.mCallback);
    auto index = top.mIndex;
    self->mPendingTasks.erase(index);
    self->mReadyTasks.pop();
    done = self->mPendingTasks.empty() && self->mReadyTasks.empty();
    lock.unlock();
    TimerSingleton::releaseEvent(index);
    callback();
  }
}

EventLoop::~EventLoop() { std::cerr << "EventLoop was destroyed...\n"; }
} // namespace cactus