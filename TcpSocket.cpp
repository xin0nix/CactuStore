#include "TcpSocket.hpp"

#include "IpAddress.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <exception>
#include <netinet/in.h>
#include <sys/socket.h>
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
  mFileDescriptor = ::socket(AF_INET, SOCK_STREAM, 0);
  if (mFileDescriptor == -1) {
    throw Exception(errno);
  }
}

TcpSocket::~TcpSocket() { ::close(mFileDescriptor); }

void TcpSocket::bindTo(const IpAddress &ipAddress) {
  auto [addr, port] = ipAddress.getFullHostAddress();

  struct sockaddr_in serverAddress;
  // Set up the server address structure
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = inet_addr(addr.c_str());

  // Bind the socket to the address
  if (::bind(mFileDescriptor, (struct sockaddr *)&serverAddress,
             sizeof(serverAddress)) == -1) {
    throw Exception(errno);
  }
}

void TcpSocket::read(std::span<uint8_t> buffer,
                     std::function<void(size_t)> callback) {
  // TODO:
  callback(42);
}

void TcpSocket::write(std::span<uint8_t> buffer,
                      std::function<void(size_t)> callback) {
  // TODO:
  callback(42);
}

} // namespace cactus