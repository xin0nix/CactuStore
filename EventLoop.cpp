#include "EventLoop.hpp"
#include "TcpSocket.hpp"
#include "Timer.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>

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

  TimerSingleton::setEvent(mTaskCount, duration, weak_from_this());
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