#include "Coro.hpp"

#include "EventLoop.hpp"

namespace cactus {

// То что будет возвращено вызывающей стороне
TimerTask TimerPromise::get_return_object() {
  return {TimerTask::from_promise(*this)};
}
// Доходим до первого co_await
std::suspend_never TimerPromise::initial_suspend() noexcept { return {}; }
// Сразу выходим
std::suspend_never TimerPromise::final_suspend() noexcept { return {}; }
// co_return ничего не возвращает
void TimerPromise::return_void() {}
// игнорируем исключения
void TimerPromise::unhandled_exception() {}

AvaitableTimer::AvaitableTimer(std::shared_ptr<EventLoop> eventLoop,
                               std::chrono::system_clock::duration duration)
    : mEventLoop(std::move(eventLoop)), mDuration(duration) {}
// а нужно ли нам вообще засыпать, может все уже и так готово и мы можем
// продолжить сразу?
bool AvaitableTimer::await_ready() const noexcept {
  return mDuration.count() <= 0;
}

// здесь мы можем запустить какой-то процесс, по завершению которого нами
// будет вызван handle.resume()
void AvaitableTimer::await_suspend(
    std::coroutine_handle<TimerPromise> handle) noexcept {
  mEventLoop->onTimeout(mDuration, [handle]() { handle.resume(); });
}

// здесь мы вернем вызывающей стороне результат операции, ну или void если не
// хотим ничего возвращать
void AvaitableTimer::await_resume() const noexcept {}

} // namespace cactus