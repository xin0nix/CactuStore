#pragma once

#include <chrono>
#include <coroutine>
#include <memory>

using namespace std::chrono_literals;

namespace cactus {

struct TimerPromise;
struct EventLoop;

struct TimerTask : std::coroutine_handle<TimerPromise> {
  using promise_type = TimerPromise;
};

struct TimerPromise {
  // То что будет возвращено вызывающей стороне
  TimerTask get_return_object();
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
  void await_suspend(std::coroutine_handle<TimerPromise> handle) noexcept;

  // здесь мы вернем вызывающей стороне результат операции, ну или void если не
  // хотим ничего возвращать
  void await_resume() const noexcept;

private:
  std::chrono::system_clock::duration mDuration;
  std::shared_ptr<EventLoop> mEventLoop;
};
} // namespace cactus