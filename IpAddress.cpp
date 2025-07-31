#include "IpAddress.hpp"

#include <arpa/inet.h>
#include <sstream>

namespace cactus {
IpAddress::IpAddress(std::array<uint8_t, 4> address, uint16_t port) noexcept
    : mAddress(address), mPort(port) {}

std::tuple<std::string, uint16_t> IpAddress::getFullHostAddress() const {
  std::stringstream addressStream;
  addressStream << mAddress[0] << '.' << mAddress[1] << '.' << mAddress[2]
                << '.' << mAddress[3];
  return {addressStream.str(), mPort};
}
} // namespace cactus