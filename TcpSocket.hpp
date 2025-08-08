#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>

namespace cactus {

struct IpAddress;

struct TcpSocket : std::enable_shared_from_this<TcpSocket> {
  struct Exception;
  TcpSocket();
  TcpSocket(int fd);
  ~TcpSocket();
  void bindTo(const IpAddress &ipAddress);
  void acceptConneciton(std::function<void(size_t)> callback);
  void readSome(std::span<uint8_t> buffer,
                std::function<void(size_t)> callback);
  void write(std::span<uint8_t> buffer, std::function<void(size_t)> callback);
  int getDescriptor() const;

private:
  int mSocketFd{-1};
};
} // namespace cactus