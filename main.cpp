#include <chrono>
#include <coroutine>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

using namespace std::chrono_literals;

struct Task {
  std::chrono::time_point<std::chrono::system_clock> mTimeout;
  std::function<void(void)> mCallback;
};

bool operator<(const Task &lhs, const Task &rhs) {
  // 0 priority is higher that say 100
  return lhs.mTimeout > rhs.mTimeout;
}

struct EventLoop : std::enable_shared_from_this<EventLoop> {
  void onTimeout(std::chrono::system_clock::duration duration,
                 std::function<void(void)> callback) {
    auto self = weak_from_this().lock();
    auto until = std::chrono::system_clock::now() + duration;
    Task task{
        .mTimeout = until,
        .mCallback = std::move(callback),
    };
    std::lock_guard lg(self->mMutex);
    self->mTasks.push(std::move(task));
  }

  void run() {
    auto self = weak_from_this().lock();
    while (true) {
      // FIXME: use OS capabilities directly to avoid stalling
      std::this_thread::sleep_for(100ms);
      const auto currentTime = std::chrono::system_clock::now();
      std::lock_guard lg(mMutex);
      if (self->mTasks.empty()) {
        return;
      }
      const auto &top = self->mTasks.top();
      if (top.mTimeout <= currentTime) {
        top.mCallback();
        self->mTasks.pop();
      }
    }
  }

  std::mutex mMutex;
  std::priority_queue<Task> mTasks;

  ~EventLoop() { std::cerr << "EventLoop was destroyed...\n"; }
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
      : mEventLoop(std::move(eventLoop)), mDuration(duration) {}
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

TimerTask task10(std::shared_ptr<EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await AvaitableTimer(eventLoop, 10s);
  std::cout << "10s\n";
}

int main() {
  auto eventLoop = std::make_shared<EventLoop>();
  task10(eventLoop);
  tick2(eventLoop);
  tick1(eventLoop);
  std::cout << "hmm.\n";
  eventLoop->run();
}
