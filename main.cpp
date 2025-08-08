#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <thread>

#include "Coro.hpp"
#include "EventLoop.hpp"
#include "IpAddress.hpp"
#include "TcpSocket.hpp"

using namespace std::chrono_literals;

// сопрограмма, которая через каждую секунду будет выводить текст на экран
cactus::CoroTask tick1(std::shared_ptr<cactus::EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await cactus::AvaitableTimer(eventLoop, 1s);
  std::cout << "1s\n";
}

cactus::CoroTask tick2(std::shared_ptr<cactus::EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await cactus::AvaitableTimer(eventLoop, 2s);
  std::cout << "2s\n";
}

cactus::CoroTask tick5(std::shared_ptr<cactus::EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await cactus::AvaitableTimer(eventLoop, 5s);
  std::cout << "5s\n";
}

cactus::CoroTask tick6(std::shared_ptr<cactus::EventLoop> eventLoop) {
  using namespace std::chrono_literals;
  co_await cactus::AvaitableTimer(eventLoop, 6s);
  std::cout << "6s\n";
}

cactus::CoroTask read1(std::shared_ptr<cactus::EventLoop> eventLoop,
                       std::shared_ptr<cactus::TcpSocket> socket) {
  auto connected =
      co_await cactus::AvaitableTcpSocketAcceptor(eventLoop, socket);
  std::vector<uint8_t> buffer(128, 0);
  auto bytesRead = co_await cactus::AvaitableTcpSocketReader(
      eventLoop, connected.value(), buffer);
  std::cout << "Bytes read: " << bytesRead.value() << std::endl;
}

int main() {
  cactus::IpAddress ipAddress({127, 0, 0, 1}, 8080);
  auto tcpSocket = std::make_shared<cactus::TcpSocket>();
  tcpSocket->bindTo(ipAddress);
  auto eventLoop = std::make_shared<cactus::EventLoop>();
  tick5(eventLoop);
  read1(eventLoop, tcpSocket);
  tick2(eventLoop);
  tick6(eventLoop);
  tick1(eventLoop);
  std::cout << "hmm.\n";
  std::thread t([eventLoop]() { eventLoop->run(); });
  t.join();
}
