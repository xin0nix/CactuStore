#pragma once

#include <cstdint>
#include <functional>
#include <span>

namespace cactus {

struct IpAddress;

struct TcpSocket {
  struct Exception;
  TcpSocket();
  ~TcpSocket();
  void bindTo(const IpAddress &ipAddress);
  void read(std::span<uint8_t> buffer, std::function<void(size_t)> callback);
  void write(std::span<uint8_t> buffer, std::function<void(size_t)> callback);

private:
  int mFileDescriptor{-1};
};
} // namespace cactus