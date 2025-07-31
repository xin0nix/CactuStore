#pragma once

namespace cactus {

struct IpAddress;

struct TcpSocket {
  struct Exception;
  TcpSocket();
  ~TcpSocket();
  void bindTo(const IpAddress &ipAddress);

private:
  int mFileDescriptor{-1};
};
} // namespace cactus