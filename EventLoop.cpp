#include "EventLoop.hpp"
#include "TcpSocket.hpp"
#include "Timer.hpp"

#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>

namespace cactus {

void EventLoop::handleTask(size_t index, size_t payload) {
  auto weak = weak_from_this();
  auto self = weak.lock();
  if (!self) {
    return;
  }
  std::unique_lock lock(self->mMutex);
  auto task = self->mPendingTasks.at(index);
  task.mPayload = payload;
  self->mReadyTasks.push(std::move(task));
  mCondVar.notify_one();
}

void EventLoop::onTimeout(std::chrono::nanoseconds duration,
                          std::function<void(size_t)> callback) {
  std::unique_lock lock(mMutex);
  mPendingTasks[mTaskCount] = Task{
      .mCallback = std::move(callback),
      .mIndex = mTaskCount,
      .mPayload = 0UL,
  };
  lock.unlock();

  TimerSingleton::setEvent(mTaskCount, duration,
                           [weak = weak_from_this()](size_t index) {
                             auto self = weak.lock();
                             if (!self) {
                               return;
                             }
                             self->handleTask(index);
                           });
  mTaskCount += 1;
}

void EventLoop::onTcpSocketRead(std::shared_ptr<TcpSocket> socket,
                                std::span<uint8_t> buffer,
                                std::function<void(size_t)> callback) {
  std::unique_lock lock(mMutex);
  mPendingTasks[mTaskCount] = Task{
      .mCallback = std::move(callback),
      .mIndex = mTaskCount,
      .mPayload = 0UL,
  };
  lock.unlock();

  // TODO: go through the event loop!
  socket->read(buffer, [weak = weak_from_this(),
                        index = mTaskCount.load()](size_t bytesRead) {
    auto self = weak.lock();
    if (!self) {
      return;
    }
    self->handleTask(index, bytesRead);
  });
  mTaskCount += 1;
}

void EventLoop::run() {
  bool done = false;
  while (!done) {
    const auto currentTime = std::chrono::system_clock::now();
    std::unique_lock lock(mMutex);
    mCondVar.wait(lock, [this]() { return !mReadyTasks.empty(); });
    const auto &top = mReadyTasks.front();
    auto callback = std::move(top.mCallback);
    auto index = top.mIndex;
    auto payload = top.mPayload;
    mPendingTasks.erase(index);
    mReadyTasks.pop();
    done = mPendingTasks.empty() && mReadyTasks.empty();
    lock.unlock();
    TimerSingleton::releaseEvent(index);
    // FIXME: use variant, probably
    callback(payload);
  }
}

EventLoop::~EventLoop() { std::cerr << "EventLoop was destroyed...\n"; }
} // namespace cactus