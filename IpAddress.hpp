#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace cactus {
struct IpAddress {
  IpAddress(std::array<uint8_t, 4> address, uint16_t port) noexcept;
  ~IpAddress() = default;
  std::tuple<std::string, uint16_t> getFullHostAddress() const;

private:
  std::array<uint8_t, 4> mAddress;
  uint16_t mPort;
};
} // namespace cactus