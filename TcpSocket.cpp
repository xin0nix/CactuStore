#include "TcpSocket.hpp"

#include "IpAddress.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace cactus {
struct TcpSocket::Exception : std::exception {
  Exception(int errorNumber) : mErrorNumber(errorNumber) {}
  const char *what() const noexcept override { return strerror(mErrorNumber); }

private:
  int mErrorNumber;
};

TcpSocket::TcpSocket() {
  // FIXME: handle SIGPIPE somehow?
  mSocketFd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (mSocketFd == -1) {
    throw Exception(errno);
  }
}

TcpSocket::TcpSocket(int fd) : mSocketFd(fd) {}

int TcpSocket::getDescriptor() const { return mSocketFd; }

TcpSocket::~TcpSocket() { ::close(mSocketFd); }

void TcpSocket::bindTo(const IpAddress &ipAddress) {
  auto [addr, port] = ipAddress.getFullHostAddress();

  struct sockaddr_in serverAddress;
  // Set up the server address structure
  serverAddress.sin_family = AF_INET;
  std::cerr << "Listening to " << addr << ":" << port << std::endl;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = inet_addr(addr.c_str());

  // Bind the socket to the address
  if (::bind(mSocketFd, (struct sockaddr *)&serverAddress,
             sizeof(serverAddress)) == -1) {
    throw Exception(errno);
  }
  int opt = 1;
  if (::setsockopt(mSocketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    throw Exception(errno);
  }
  if (::listen(mSocketFd, 10) == -1) {
    throw Exception(errno);
  }
}

void TcpSocket::acceptConneciton(std::function<void(size_t)> callback) {
  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);
  int connFd =
      accept(mSocketFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
  if (connFd < 0) {
    // FIXME:
    callback(0);
    return;
  }
  callback(connFd);
}

void TcpSocket::readSome(std::span<uint8_t> buffer,
                         std::function<void(size_t)> callback) {
  std::thread t1([weak = weak_from_this(), buffer, callback]() {
    auto self = weak.lock();
    if (!self) {
      callback(0); // FIXME: error propagation
      return;
    }

    struct pollfd pfd;
    pfd.fd = self->mSocketFd;
    pfd.events = POLLIN;
    // FIXME: make it argument of the socket's constructor
    while (true) {
      int pollCount = ::poll(&pfd, 1, 30'000 /*ms*/);
      if (pollCount > 0) {
        if (self->mSocketFd < 0) {
          perror("accept failed");
          callback(0);
        }
        if (pfd.revents & POLLIN) {
          ssize_t n = ::recv(self->mSocketFd, buffer.data(), buffer.size(), 0);
          if (n > 0) {
            // Обработайте buffer c n байтами данных
            callback(n);
            return;
          } else if (n == 0) {
            callback(0);
            return;
          } else {
            perror("poll failed");
            // errno();
            callback(0);
            return;
          }
        }
      } else if (pollCount == 0) {
        callback(0);
        return;
      }
      perror("poll failed");
      // // Ошибка poll
      // callback(0);
    }
  });
  t1.detach();
}

void TcpSocket::write(std::span<uint8_t> buffer,
                      std::function<void(size_t)> callback) {
  // TODO:
  callback(42);
}

} // namespace cactus