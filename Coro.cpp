#include "Coro.hpp"

#include "EventLoop.hpp"

namespace cactus {

// То что будет возвращено вызывающей стороне
CoroTask CoroPromise::get_return_object() {
  return {CoroTask::from_promise(*this)};
}
std::suspend_never CoroPromise::initial_suspend() noexcept { return {}; }
std::suspend_never CoroPromise::final_suspend() noexcept { return {}; }
void CoroPromise::return_void() {}
void CoroPromise::unhandled_exception() {}

//-----------------------------------------------------------------------------
// Timer

AvaitableTimer::AvaitableTimer(std::shared_ptr<EventLoop> eventLoop,
                               std::chrono::system_clock::duration duration)
    : mEventLoop(std::move(eventLoop)), mDuration(duration) {}

bool AvaitableTimer::await_ready() const noexcept {
  return mDuration.count() <= 0;
}

void AvaitableTimer::await_suspend(
    std::coroutine_handle<CoroPromise> handle) noexcept {
  mEventLoop->onTimeout(mDuration, [handle](size_t) { handle.resume(); });
}

void AvaitableTimer::await_resume() const noexcept {}

//-----------------------------------------------------------------------------
// Socket Reader

AvaitableTcpSocketReader::AvaitableTcpSocketReader(
    std::shared_ptr<EventLoop> eventLoop, std::shared_ptr<TcpSocket> socket,
    std::span<uint8_t> buffer)
    : mEventLoop(std::move(eventLoop)), mTcpSocket(std::move(socket)),
      mBuffer(buffer) {}

bool AvaitableTcpSocketReader::await_ready() const noexcept { return false; }

void AvaitableTcpSocketReader::AvaitableTcpSocketReader::await_suspend(
    std::coroutine_handle<CoroPromise> handle) noexcept {
  mEventLoop->onTcpSocketRead(mTcpSocket, mBuffer,
                              [this, handle](size_t bytesRead) {
                                this->mBytesRead = bytesRead;
                                handle.resume();
                              });
}

size_t AvaitableTcpSocketReader::await_resume() const noexcept {
  return mBytesRead;
}

} // namespace cactus