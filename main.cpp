#include <chrono>
#include <coroutine>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>

#include "EventLoop.hpp"
#include "IpAddress.hpp"
#include "TcpSocket.hpp"

using namespace std::chrono_literals;

namespace cactus {

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
} // namespace cactus

// сопрограмма, которая через каждую секунду будет выводить текст на экран
cactus::TimerTask tick1(std::shared_ptr<cactus::EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await cactus::AvaitableTimer(eventLoop, 1s);
  std::cout << "1s\n";
}

cactus::TimerTask tick2(std::shared_ptr<cactus::EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await cactus::AvaitableTimer(eventLoop, 2s);
  std::cout << "2s\n";
}

cactus::TimerTask task5(std::shared_ptr<cactus::EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await cactus::AvaitableTimer(eventLoop, 5s);
  std::cout << "5s\n";
}

cactus::TimerTask task6(std::shared_ptr<cactus::EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await cactus::AvaitableTimer(eventLoop, 6s);
  std::cout << "6s\n";
}

int main() {
  cactus::IpAddress ipAddress({127, 0, 0, 1}, 8080);
  cactus::TcpSocket tcpSocket;
  tcpSocket.bindTo(ipAddress);
  auto eventLoop = std::make_shared<cactus::EventLoop>();
  task5(eventLoop);
  tick2(eventLoop);
  task6(eventLoop);
  tick1(eventLoop);
  std::cout << "hmm.\n";
  eventLoop->run();
}
