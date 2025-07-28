#include <chrono>
#include <coroutine>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

using namespace std::chrono_literals;

struct EventLoop : std::enable_shared_from_this<EventLoop> {
  void onTimeout(std::chrono::system_clock::duration duration,
                 std::function<void(void)> callback) {
    std::async(std::launch::async, [duration]() {
      std::this_thread::sleep_for(duration);
    }).wait();
    callback();
  }
};

struct TimerPromise;

struct TimerTask : std::coroutine_handle<TimerPromise> {
  using promise_type = TimerPromise;
};

struct TimerPromise {
  // То что будет возвращено вызывающей стороне
  TimerTask get_return_object() { return {TimerTask::from_promise(*this)}; }
  // Доходим до первого co_await
  std::suspend_never initial_suspend() noexcept { return {}; }
  // Сразу выходим
  std::suspend_never final_suspend() noexcept { return {}; }
  // co_return ничего не возвращает
  void return_void() {}
  // игнорируем исключения
  void unhandled_exception() {}
};

struct AvaitableTimer {
  explicit AvaitableTimer(std::shared_ptr<EventLoop> eventLoop,
                          std::chrono::system_clock::duration duration)
      : mDuration(duration) {}
  // а нужно ли нам вообще засыпать, может все уже и так готово и мы можем
  // продолжить сразу?
  bool await_ready() const noexcept { return mDuration.count() <= 0; }

  // здесь мы можем запустить какой-то процесс, по завершению которого нами
  // будет вызван handle.resume()
  void await_suspend(std::coroutine_handle<TimerPromise> handle) noexcept {
    mEventLoop->onTimeout(mDuration, [handle]() { handle.resume(); });
  }

  // здесь мы вернем вызывающей стороне результат операции, ну или void если не
  // хотим ничего возвращать
  void await_resume() const noexcept {}

private:
  std::chrono::system_clock::duration mDuration;
  std::shared_ptr<EventLoop> mEventLoop;
};

// сопрограмма, которая через каждую секунду будет выводить текст на экран
TimerTask tick1(std::shared_ptr<EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await AvaitableTimer(eventLoop, 1s);
  std::cout << "1s\n";
}

TimerTask tick2(std::shared_ptr<EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await AvaitableTimer(eventLoop, 2s);
  std::cout << "2s\n";
}

TimerTask tick3(std::shared_ptr<EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await AvaitableTimer(eventLoop, 2s);
  std::cout << "3s\n";
}

// TODO: https://habr.com/ru/articles/798935/
int main() {
  auto eventLoop = std::make_shared<EventLoop>();
  // tick(eventLoop);
  std::thread t1([eventLoop]() {
    tick3(eventLoop);
    tick2(eventLoop);
    tick1(eventLoop);
  });
  std::cout << "hmm.\n";
  t1.join();
}