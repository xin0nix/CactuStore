#include <chrono>
#include <coroutine>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

using namespace std::chrono_literals;

using SigVal = union ::sigval;
using SigEvent = struct ::sigevent;
using ITimerSpec = struct ::itimerspec;

struct EventLoop;

struct Task {
  void onNotification();

  std::function<void(void)> mCallback;
  std::weak_ptr<EventLoop> mEventLoop;
  size_t mIndex;
};

void signal_handler(int sig, siginfo_t *si, void *uc) {
  Task *task = static_cast<Task *>(si->si_value.sival_ptr);
  if (!task) {
    return;
  }
  task->onNotification();
}

struct EventLoop : std::enable_shared_from_this<EventLoop> {

  void handleTask(size_t index) {
    auto weak = weak_from_this();
    auto self = weak.lock();
    if (!self) {
      return;
    }
    std::lock_guard guard(self->mMutex);
    auto task = self->mPendingTasks.at(index);
    self->mReadyTasks.push(std::move(task));
    self->mPendingTasks.erase(index);
  }

  void onTimeout(std::chrono::nanoseconds duration,
                 std::function<void(void)> callback) {
    auto weak = weak_from_this();
    auto self = weak.lock();
    if (!self) {
      return;
    }
    std::lock_guard guard(self->mMutex);
    self->mPendingTasks[mTaskCount] = Task{
        .mCallback = std::move(callback),
        .mEventLoop = weak_from_this(),
        .mIndex = mTaskCount,
    };

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
    sev.sigev_value.sival_ptr = &self->mPendingTasks.at(mTaskCount);
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

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
      perror("timer_settime");
      exit(EXIT_FAILURE);
    }
    mTaskCount += 1;
  }

  void run() {
    auto weak = weak_from_this();
    auto self = weak.lock();
    if (!self) {
      return;
    }
    while (true) {
      // FIXME: wait on CV capabilities directly to avoid stalling
      std::this_thread::sleep_for(100ms);
      const auto currentTime = std::chrono::system_clock::now();
      std::lock_guard guard(self->mMutex);
      if (self->mReadyTasks.empty()) {
        if (self->mPendingTasks.empty()) {
          return;
        }
        continue;
      }
      const auto &top = self->mReadyTasks.front();
      top.mCallback();
      self->mReadyTasks.pop();
    }
  }

  std::atomic<size_t> mTaskCount{0};
  std::mutex mMutex;
  std::unordered_map<size_t, Task> mPendingTasks;
  std::queue<Task> mReadyTasks;

  ~EventLoop() { std::cerr << "EventLoop was destroyed...\n"; }
};

void Task::onNotification() {
  auto loop = mEventLoop.lock();
  if (!loop) {
    return;
  }
  loop->handleTask(mIndex);
}

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
