#pragma once

#include "TcpSocket.hpp"
#include <chrono>
#include <coroutine>
#include <expected>
#include <memory>

using namespace std::chrono_literals;

namespace cactus {

struct CoroPromise;
struct EventLoop;
struct TcpSocket;

struct CoroTask : std::coroutine_handle<CoroPromise> {
  using promise_type = CoroPromise;
};

struct CoroPromise {
  // То что будет возвращено вызывающей стороне
  CoroTask get_return_object();
  // Доходим до первого co_await
  std::suspend_never initial_suspend() noexcept;
  // Сразу выходим
  std::suspend_never final_suspend() noexcept;
  // co_return ничего не возвращает
  void return_void();
  // игнорируем исключения
  void unhandled_exception();
};

struct AvaitableTimer {
  explicit AvaitableTimer(std::shared_ptr<EventLoop> eventLoop,
                          std::chrono::system_clock::duration duration);
  // а нужно ли нам вообще засыпать, может все уже и так готово и мы можем
  // продолжить сразу?
  bool await_ready() const noexcept;

  // здесь мы можем запустить какой-то процесс, по завершению которого нами
  // будет вызван handle.resume()
  void await_suspend(std::coroutine_handle<CoroPromise> handle) noexcept;

  // здесь мы вернем вызывающей стороне результат операции, ну или void если не
  // хотим ничего возвращать
  void await_resume() const noexcept;

private:
  std::chrono::system_clock::duration mDuration;
  std::shared_ptr<EventLoop> mEventLoop;
};

struct AvaitableTcpSocketAcceptor {
  explicit AvaitableTcpSocketAcceptor(std::shared_ptr<EventLoop> eventLoop,
                                      std::shared_ptr<TcpSocket> socket);
  bool await_ready() const noexcept;
  void await_suspend(std::coroutine_handle<CoroPromise> handle) noexcept;
  std::expected<std::shared_ptr<TcpSocket>, std::exception>
  await_resume() const noexcept;

private:
  std::shared_ptr<EventLoop> mEventLoop;
  std::shared_ptr<TcpSocket> mTcpSocket;
  std::expected<std::shared_ptr<TcpSocket>, std::exception> mConnectedSocket;
};

struct AvaitableTcpSocketReader {
  explicit AvaitableTcpSocketReader(std::shared_ptr<EventLoop> eventLoop,
                                    std::shared_ptr<TcpSocket> socket,
                                    std::span<uint8_t> buffer);
  // а нужно ли нам вообще засыпать, может все уже и так готово и мы можем
  // продолжить сразу?
  bool await_ready() const noexcept;

  // здесь мы можем запустить какой-то процесс, по завершению которого нами
  // будет вызван handle.resume()
  void await_suspend(std::coroutine_handle<CoroPromise> handle) noexcept;

  // здесь мы вернем вызывающей стороне результат операции
  std::expected<size_t, std::exception> await_resume() const noexcept;

private:
  std::span<uint8_t> mBuffer;
  std::shared_ptr<TcpSocket> mTcpSocket;
  std::shared_ptr<EventLoop> mEventLoop;
  std::expected<size_t, std::exception> mBytesRead{0};
};

} // namespace cactus