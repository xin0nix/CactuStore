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
  mEventLoop->onTimeout(
      mDuration,
      [handle](std::expected<size_t, std::exception>) { handle.resume(); });
}

void AvaitableTimer::await_resume() const noexcept {}

//-----------------------------------------------------------------------------
// Connection Acceptor

AvaitableTcpSocketAcceptor::AvaitableTcpSocketAcceptor(
    std::shared_ptr<EventLoop> eventLoop, std::shared_ptr<TcpSocket> socket)
    : mEventLoop(std::move(eventLoop)), mTcpSocket(std::move(socket)) {}

bool AvaitableTcpSocketAcceptor::await_ready() const noexcept { return false; }

void AvaitableTcpSocketAcceptor::await_suspend(
    std::coroutine_handle<CoroPromise> handle) noexcept {
  mEventLoop->onAcceptConnection(
      mTcpSocket,
      [this, handle](std::expected<size_t, std::exception> socketFd) {
        // TODO:
        this->mConnectedSocket = std::make_shared<TcpSocket>(socketFd.value());
        handle.resume();
      });
}

std::expected<std::shared_ptr<TcpSocket>, std::exception>
AvaitableTcpSocketAcceptor::await_resume() const noexcept {
  return this->mConnectedSocket;
}

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
  mEventLoop->onTcpSocketRead(
      mTcpSocket, mBuffer,
      [this, handle](std::expected<size_t, std::exception> bytesRead) {
        this->mBytesRead = bytesRead;
        handle.resume();
      });
}

std::expected<size_t, std::exception>
AvaitableTcpSocketReader::await_resume() const noexcept {
  return mBytesRead;
}

} // namespace cactus